/*
 * Object-oriented thread pool API
 *
 * Copyright 2014-2015 Sebastian Lackner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <limits.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(threadpool);

static inline LONG interlocked_inc( PLONG dest )
{
    return interlocked_xchg_add( dest, 1 ) + 1;
}

static inline LONG interlocked_dec( PLONG dest )
{
    return interlocked_xchg_add( dest, -1 ) - 1;
}

#define THREADPOOL_WORKER_TIMEOUT 5000

/* internal threadpool representation */
struct threadpool
{
    LONG                    refcount;
    BOOL                    shutdown;
    CRITICAL_SECTION        cs;
    /* pool of work items, locked via .cs */
    struct list             pool;
    RTL_CONDITION_VARIABLE  update_event;
    /* information about worker threads, locked via .cs */
    int                     max_workers;
    int                     min_workers;
    int                     num_workers;
    int                     num_busy_workers;
};

enum threadpool_objtype
{
    TP_OBJECT_TYPE_SIMPLE
};

/* internal threadpool object representation */
struct threadpool_object
{
    LONG                    refcount;
    BOOL                    shutdown;
    /* read-only information */
    enum threadpool_objtype type;
    struct threadpool       *pool;
    PVOID                   userdata;
    /* information about the pool, locked via .pool->cs */
    struct list             pool_entry;
    LONG                    num_pending_callbacks;
    LONG                    num_running_callbacks;
    /* arguments for callback */
    union
    {
        struct
        {
            PTP_SIMPLE_CALLBACK callback;
        } simple;
    } u;
};

static inline struct threadpool *impl_from_TP_POOL( TP_POOL *pool )
{
    return (struct threadpool *)pool;
}

static void CALLBACK threadpool_worker_proc( void *param );
static NTSTATUS tp_threadpool_alloc( struct threadpool **out );
static void tp_threadpool_shutdown( struct threadpool *pool );
static BOOL tp_threadpool_release( struct threadpool *pool );
static void tp_object_submit( struct threadpool_object *object );
static void tp_object_shutdown( struct threadpool_object *object );
static BOOL tp_object_release( struct threadpool_object *object );

static struct threadpool *default_threadpool = NULL;

/* allocates or returns the default threadpool */
static struct threadpool *get_default_threadpool( void )
{
    if (!default_threadpool)
    {
        struct threadpool *pool;

        if (tp_threadpool_alloc( &pool ) != STATUS_SUCCESS)
            return NULL;

        if (interlocked_cmpxchg_ptr( (void *)&default_threadpool, pool, NULL ) != NULL)
        {
            tp_threadpool_shutdown( pool );
            tp_threadpool_release( pool );
        }
    }
    return default_threadpool;
}

/* allocate a new threadpool (with at least one worker thread) */
static NTSTATUS tp_threadpool_alloc( struct threadpool **out )
{
    struct threadpool *pool;
    NTSTATUS status;
    HANDLE thread;

    pool = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*pool) );
    if (!pool)
        return STATUS_NO_MEMORY;

    pool->refcount              = 2; /* this thread + worker proc */
    pool->shutdown              = FALSE;

    RtlInitializeCriticalSection( &pool->cs );
    pool->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": threadpool.cs");

    list_init( &pool->pool );
    RtlInitializeConditionVariable( &pool->update_event );

    pool->max_workers           = 500;
    pool->min_workers           = 1;
    pool->num_workers           = 1;
    pool->num_busy_workers      = 0;

    status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE, NULL, 0, 0,
                                  threadpool_worker_proc, pool, &thread, NULL );
    if (status != STATUS_SUCCESS)
    {
        pool->cs.DebugInfo->Spare[0] = 0;
        RtlDeleteCriticalSection( &pool->cs );
        RtlFreeHeap( GetProcessHeap(), 0, pool );
        return status;
    }
    NtClose( thread );

    TRACE("allocated threadpool %p\n", pool);

    *out = pool;
    return STATUS_SUCCESS;
}

/* shutdown all threads of the threadpool */
static void tp_threadpool_shutdown( struct threadpool *pool )
{
    assert( pool != default_threadpool );

    pool->shutdown = TRUE;
    RtlWakeAllConditionVariable( &pool->update_event );
}

/* release a reference to a threadpool */
static BOOL tp_threadpool_release( struct threadpool *pool )
{
    if (interlocked_dec( &pool->refcount ))
        return FALSE;

    TRACE("destroying threadpool %p\n", pool);

    assert( pool->shutdown );
    assert( list_empty( &pool->pool ) );

    pool->cs.DebugInfo->Spare[0] = 0;
    RtlDeleteCriticalSection( &pool->cs );

    RtlFreeHeap( GetProcessHeap(), 0, pool );
    return TRUE;
}

/* threadpool worker function */
static void CALLBACK threadpool_worker_proc( void *param )
{
    struct threadpool *pool = param;
    LARGE_INTEGER timeout;
    struct list *ptr;

    RtlEnterCriticalSection( &pool->cs );
    for (;;)
    {
        while ((ptr = list_head( &pool->pool )))
        {
            struct threadpool_object *object = LIST_ENTRY( ptr, struct threadpool_object, pool_entry );
            assert( object->num_pending_callbacks > 0 );

            /* If further pending callbacks are queued, move the work item to
             * the end of the pool list. Otherwise remove it from the pool. */
            list_remove( &object->pool_entry );
            if (--object->num_pending_callbacks)
                list_add_tail( &pool->pool, &object->pool_entry );

            /* Leave critical section and do the actual callback. */
            object->num_running_callbacks++;
            pool->num_busy_workers++;
            RtlLeaveCriticalSection( &pool->cs );

            switch (object->type)
            {
                case TP_OBJECT_TYPE_SIMPLE:
                {
                    TRACE( "executing simple callback %p(NULL, %p)\n",
                           object->u.simple.callback, object->userdata );
                    object->u.simple.callback( NULL, object->userdata );
                    TRACE( "callback %p returned\n", object->u.simple.callback );
                    break;
                }

                default:
                    assert(0);
                    break;
            }

            RtlEnterCriticalSection( &pool->cs );
            pool->num_busy_workers--;
            object->num_running_callbacks--;
            tp_object_release( object );
        }

        /* Shutdown worker thread if requested. */
        if (pool->shutdown)
            break;

        /* Wait for new tasks or until timeout expires. Never terminate the last worker. */
        timeout.QuadPart = (ULONGLONG)THREADPOOL_WORKER_TIMEOUT * -10000;
        if (RtlSleepConditionVariableCS( &pool->update_event, &pool->cs, &timeout ) == STATUS_TIMEOUT &&
            !list_head( &pool->pool ) && pool->num_workers > 1)
        {
            break;
        }
    }
    pool->num_workers--;
    RtlLeaveCriticalSection( &pool->cs );
    tp_threadpool_release( pool );
}

/* initializes a new threadpool object */
static void tp_object_initialize( struct threadpool_object *object, struct threadpool *pool,
                                  PVOID userdata, TP_CALLBACK_ENVIRON *environment )
{
    object->refcount                = 1;
    object->shutdown                = FALSE;

    object->pool                    = pool;
    object->userdata                = userdata;

    memset( &object->pool_entry, 0, sizeof(object->pool_entry) );
    object->num_pending_callbacks   = 0;
    object->num_running_callbacks   = 0;

    if (environment)
        FIXME("environment not implemented yet\n");

    /* Increase reference-count on the pool */
    interlocked_inc( &pool->refcount );

    TRACE("allocated object %p of type %u\n", object, object->type);
}

/* allocates and submits a 'simple' threadpool task. */
static NTSTATUS tp_object_submit_simple( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                         TP_CALLBACK_ENVIRON *environment )
{
    struct threadpool_object *object;
    struct threadpool *pool = NULL;

    if (environment)
        pool = (struct threadpool *)environment->Pool;

    if (!pool)
    {
        pool = get_default_threadpool();
        if (!pool)
            return STATUS_NO_MEMORY;
    }

    object = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*object) );
    if (!object)
        return STATUS_NO_MEMORY;

    object->type = TP_OBJECT_TYPE_SIMPLE;
    object->u.simple.callback = callback;
    tp_object_initialize( object, pool, userdata, environment );

    tp_object_submit( object );

    tp_object_shutdown( object );
    tp_object_release( object );
    return STATUS_SUCCESS;
}

/* submits an object to a threadpool */
static void tp_object_submit( struct threadpool_object *object )
{
    struct threadpool *pool = object->pool;

    assert( !object->shutdown );
    assert( !pool->shutdown );

    RtlEnterCriticalSection( &pool->cs );

    /* Start new worker threads if required (and allowed) */
    if (pool->num_busy_workers >= pool->num_workers && pool->num_workers < pool->max_workers)
    {
        NTSTATUS status;
        HANDLE thread;

        status = RtlCreateUserThread( GetCurrentProcess(), NULL, FALSE, NULL, 0, 0,
                                      threadpool_worker_proc, pool, &thread, NULL );
        if (status == STATUS_SUCCESS)
        {
            interlocked_inc( &pool->refcount );
            pool->num_workers++;
            NtClose( thread );
            goto out;
        }
    }

    assert( pool->num_workers > 0 );
    RtlWakeConditionVariable( &pool->update_event );

out:
    /* Queue work item into pool and increment refcount */
    interlocked_inc( &object->refcount );
    if (!object->num_pending_callbacks++)
        list_add_tail( &pool->pool, &object->pool_entry );

    RtlLeaveCriticalSection( &pool->cs );
}

/* mark an object as 'shutdown', submitting is no longer possible */
static void tp_object_shutdown( struct threadpool_object *object )
{
    object->shutdown = TRUE;
}

/* release a reference to a threadpool object */
static BOOL tp_object_release( struct threadpool_object *object )
{
    if (interlocked_dec( &object->refcount ))
        return FALSE;

    TRACE("destroying object %p of type %u\n", object, object->type);

    assert( object->shutdown );
    assert( !object->num_pending_callbacks );
    assert( !object->num_running_callbacks );

    /* release reference to threadpool */
    tp_threadpool_release( object->pool );

    RtlFreeHeap( GetProcessHeap(), 0, object );
    return TRUE;
}


/***********************************************************************
 *           TpAllocPool    (NTDLL.@)
 */
NTSTATUS WINAPI TpAllocPool( TP_POOL **out, PVOID reserved )
{
    TRACE("%p %p\n", out, reserved);

    if (reserved)
        FIXME("reserved argument is nonzero (%p)", reserved);

    if (!out)
        return STATUS_ACCESS_VIOLATION;

    return tp_threadpool_alloc( (struct threadpool **)out );
}

/***********************************************************************
 *           TpReleasePool    (NTDLL.@)
 */
VOID WINAPI TpReleasePool( TP_POOL *pool )
{
    struct threadpool *this = impl_from_TP_POOL( pool );
    TRACE("%p\n", pool);

    if (this)
    {
        tp_threadpool_shutdown( this );
        tp_threadpool_release( this );
    }
}

/***********************************************************************
 *           TpSimpleTryPost    (NTDLL.@)
 */
NTSTATUS WINAPI TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                 TP_CALLBACK_ENVIRON *environment )
{
    TRACE("%p %p %p\n", callback, userdata, environment);

    return tp_object_submit_simple( callback, userdata, environment );
}
