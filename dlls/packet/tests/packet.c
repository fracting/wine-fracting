#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <stdio.h>
#include <iphlpapi.h>
#include <packet32.h>
#include <string.h>

#include "wine/test.h"

void test_PacketAllocatePacket(void)
{
    LPPACKET packet1, packet2;
    packet1 = PacketAllocatePacket();
    ok(packet1 != NULL, "packet allocate fails.\n");

    packet2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PACKET));
    if(packet1)
    {
        ok(memcmp(packet1, packet2, sizeof(PACKET)) == 0, "packet should be zero initialized.\n");
        PacketFreePacket(packet1);
    }
    HeapFree(GetProcessHeap(), 0, packet2);
}

void test_PacketGetAdapterNames(void)
{
    char *buffer, *zero, *ptr;
    DWORD len, total, ret, got_size, size, infolen;
    PIP_ADAPTER_INFO adapter_info;

    infolen = 0;
    ret = GetAdaptersInfo(NULL, &infolen);
    ok(ret == ERROR_NO_DATA || ret == ERROR_BUFFER_OVERFLOW, "Query infolen fail! ret is %x\n", ret);

    if (ret == ERROR_NO_DATA)
    {
        size = 1;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size == 0, "size should be zero when no adapter exists!\n");

        size = 1;
        buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(buffer, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size == 0, "size should be zero when no adapter exists!\n");
        HeapFree(GetProcessHeap(), 0, buffer);
    }
    else if(ret == ERROR_BUFFER_OVERFLOW)
    {
        adapter_info = HeapAlloc(GetProcessHeap(), 0, infolen);
        ret = GetAdaptersInfo(adapter_info, &infolen);

        ok(ret == NO_ERROR, "Get Adapters Info fail! ret is %x\n", ret);

        /* test NULL buffer with different size */
        size = 0;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        todo_wine ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        todo_wine ok(size > 0, "size should be non zero!\n");

        got_size = size;

        size = got_size - 1;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        todo_wine ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        todo_wine ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);

        size = got_size;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        todo_wine ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);

        size = got_size + 1;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        todo_wine ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        todo_wine ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);

        /* test non NULL buffer with different size */
        buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, got_size);
        zero = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, got_size);

        size = got_size - 1;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(buffer, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        todo_wine ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        todo_wine ok(size == got_size, "size %d and got_size %d don't match!\n", size, got_size);
        ok(!memcmp(buffer, zero, got_size), "buffer should not be modified!\n");

        size = got_size;
        ret = PacketGetAdapterNames(buffer, &size);
        todo_wine ok(ret == TRUE, "PacketGetAdapterNames should success!\n");
        ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);
        todo_wine ok(memcmp(buffer, zero, got_size), "buffer should be filled!\n");

        size = got_size + 1;
        ret = PacketGetAdapterNames(buffer, &size);
        todo_wine ok(ret == TRUE, "PacketGetAdapterNames should success!\n");
        ok(size == got_size + 1, "size should not be modified!\n");
        todo_wine ok(memcmp(buffer, zero, got_size), "buffer should be filled!\n");

        total = 0;
        ptr = buffer;
        do
        {
            trace("adapter name: %s\n", ptr);
            len = lstrlenA(ptr);
            ptr += len + 1;
            total++;
        } while (*(ptr) != '\0');
        ptr++;
        do
        {
            trace("adapter description: %s\n", ptr);
            len = lstrlenA(ptr);
            ptr += len + 1;
            total--;
        } while (total > 0);
        ptr++;
        todo_wine ok(ptr - buffer == got_size, "got_size don't match: %d %d\n", ptr - buffer, got_size);

        HeapFree(GetProcessHeap(), 0, buffer);
        HeapFree(GetProcessHeap(), 0, zero);
        HeapFree(GetProcessHeap(), 0, adapter_info);
    }
}

START_TEST( packet )
{
    test_PacketAllocatePacket();
    test_PacketGetAdapterNames();
}
