#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <windows.h>
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
        ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size > 0, "size should be non zero!\n");

        got_size = size;

        size = got_size - 1;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);

        size = got_size;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);

        size = got_size + 1;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(NULL, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);

        /* test non NULL buffer with different size */
        buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, got_size);
        zero = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, got_size);

        size = got_size - 1;
        SetLastError(0xdeadbeef);
        ret = PacketGetAdapterNames(buffer, &size);
        ok(ret == FALSE, "PacketGetAdapterNames should fail!\n");
        ret = GetLastError();
        ok(ret == ERROR_INSUFFICIENT_BUFFER,"GetLastError returned %x instead of ERROR_INSUFFICIENT_BUFFER!\n", ret);
        ok(size == got_size, "size %d and got_size %d don't match!\n", size, got_size);
        ok(!memcmp(buffer, zero, got_size), "buffer should not be modified!\n");

        size = got_size;
        ret = PacketGetAdapterNames(buffer, &size);
        ok(ret == TRUE, "PacketGetAdapterNames should success!\n");
        ok(size == got_size, "size %d and got %d don't match!\n", size, got_size);
        ok(memcmp(buffer, zero, got_size), "buffer should be filled!\n");

        size = got_size + 1;
        ret = PacketGetAdapterNames(buffer, &size);
        ok(ret == TRUE, "PacketGetAdapterNames should success!\n");
        ok(size == got_size + 1, "size should not be modified!\n");
        ok(memcmp(buffer, zero, got_size), "buffer should be filled!\n");

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
        ok(ptr - buffer == got_size, "got_size don't match: %d %d\n", ptr - buffer, got_size);

        HeapFree(GetProcessHeap(), 0, buffer);
        HeapFree(GetProcessHeap(), 0, zero);
        HeapFree(GetProcessHeap(), 0, adapter_info);
    }
}

void test_PacketOpenAdapter(void)
{
    ADAPTER *adapter;
    DWORD ret, size, len;
    char fake_adapterA[] = "fake", *buffer, *ptr;
    WCHAR fake_adapterW[] = {'f','a','k','e',0}, *nameW;

    SetLastError(0xdeadbeef);
    adapter = PacketOpenAdapter(fake_adapterA);
    ok(adapter == NULL, "expect NULL, return %p.\n", adapter);
    ret = GetLastError();
    ok(ret == ERROR_BAD_UNIT, "expect ERROR_BAD_UNIT, returned %x.\n", ret);

    SetLastError(0xdeadbeef);
    adapter = PacketOpenAdapter((char *)fake_adapterW);
    ok(adapter == NULL, "expect NULL, return %p.\n", adapter);
    ret = GetLastError();
    ok(ret == ERROR_BAD_UNIT, "expect ERROR_BAD_UNIT, returned %x.\n", ret);

    PacketGetAdapterNames(NULL, &size);
    if (size > 0)
    {
        buffer = HeapAlloc(GetProcessHeap(), 0, size);
        ret = PacketGetAdapterNames(buffer, &size);
        ptr = buffer;
        do
        {
           len = lstrlenA(ptr);

           adapter = PacketOpenAdapter(ptr);
           ok(adapter != NULL, "adapter should not be NULL.\n");
           if(!adapter) break;
           trace("adapter %p : %p, %s, %d, %d, %s, %p, %d\n", adapter, adapter->hFile, adapter->SymbolicLink, adapter->NumWrites, adapter->ReadTimeOut, adapter->Name, adapter->pWanAdapter, adapter->Flags);
           ok(adapter->SymbolicLink[0] == 0, "expect empty str, returned %s.\n", adapter->SymbolicLink);
           ok(adapter->NumWrites == 1, "expect 1, returned %d.\n", adapter->NumWrites);
           ok(adapter->ReadTimeOut == 0, "expect 0, returned %d.\n", adapter->ReadTimeOut);
           ok(!lstrcmpA(ptr, adapter->Name), "expect %s, returned %s.\n", ptr, adapter->Name);
           ok(adapter->pWanAdapter == NULL, "expect NULL, returned %p.\n", adapter->pWanAdapter);
           ok(adapter->Flags == 0, "expect 0, returned %d.\n", adapter->Flags);
           PacketCloseAdapter(adapter);

           nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
           MultiByteToWideChar(CP_ACP, 0, ptr, -1, nameW, len + 1);
           adapter = PacketOpenAdapter((char *)nameW);
           ok(adapter != NULL, "adapter should not be NULL.\n");
           if(!adapter) break;
           trace("adapter %p : %p, %s, %d, %d, %s, %p, %d\n", adapter, adapter->hFile, adapter->SymbolicLink, adapter->NumWrites, adapter->ReadTimeOut, adapter->Name, adapter->pWanAdapter, adapter->Flags);
           ok(adapter->SymbolicLink[0] == 0, "expect empty str, returned %s.\n", adapter->SymbolicLink);
           ok(adapter->NumWrites == 1, "expect 1, returned %d.\n", adapter->NumWrites);
           ok(adapter->ReadTimeOut == 0, "expect 0, returned %d.\n", adapter->ReadTimeOut);
           ok(!lstrcmpA(ptr, adapter->Name), "expect %s, returned %s.\n", ptr, adapter->Name);
           ok(adapter->pWanAdapter == NULL, "expect NULL, returned %p.\n", adapter->pWanAdapter);
           ok(adapter->Flags == 0, "expect 0, returned %d.\n", adapter->Flags);
           PacketCloseAdapter(adapter);
           HeapFree(GetProcessHeap(), 0, nameW);

           ptr += len + 1;
        } while (*ptr != '\0');

        HeapFree(GetProcessHeap(), 0, buffer);
    }
}

START_TEST( packet )
{
    test_PacketAllocatePacket();
    test_PacketGetAdapterNames();
    test_PacketOpenAdapter();
}
