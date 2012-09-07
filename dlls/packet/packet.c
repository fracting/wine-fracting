#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "iphlpapi.h"
#include "packet32.h"

WINE_DEFAULT_DEBUG_CHANNEL(packet);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

LPPACKET PacketAllocatePacket(void)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PACKET));
}

VOID PacketFreePacket(PACKET *packet)
{
    TRACE("free packet %p\n", packet);
    HeapFree(GetProcessHeap(), 0, packet);
}

BOOLEAN PacketGetAdapterNames(char *buffer, DWORD *size)
{
    IP_ADAPTER_INFO *adapter, *adapter_list = NULL;
    DWORD infolen = 0, size_desc= 0, size_name = 0, size_total = 0, desc_offset = 0;

    TRACE("buffer %p, size %p, *size %d\n", buffer, size, *size);

    GetAdaptersInfo(adapter_list, &infolen);
    if (infolen == 0)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    adapter_list = HeapAlloc(GetProcessHeap(), 0, infolen);
    GetAdaptersInfo(adapter_list, &infolen);

    for (adapter = adapter_list; adapter != NULL; adapter = adapter->Next)
    {
        size_name += lstrlenA(adapter->AdapterName) + 1;
        size_desc += lstrlenA(adapter->Description) + 1;;
    }
    size_total = size_name + size_desc + 2;

    if (!buffer || *size < size_total)
    {
        *size = size_total;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    desc_offset = size_name + 1;
    size_name = 0;
    size_desc = 0;
    for (adapter = adapter_list; adapter != NULL; adapter = adapter->Next)
    {
        lstrcpyA(buffer + size_name, adapter->AdapterName);
        lstrcpyA(buffer + desc_offset + size_desc, adapter->Description);
        size_name += lstrlenA(adapter->AdapterName) + 1;
        size_desc += lstrlenA(adapter->Description) + 1;
    }
    buffer[size_name] = 0;
    buffer[size_total - 1] = 0;

    HeapFree(GetProcessHeap(), 0, adapter_list);

    return TRUE;
}

LPADAPTER PacketOpenAdapter(char *adapter_name)
{
    FIXME("adapter_name: %s\n", adapter_name);

    return NULL;
}

VOID PacketCloseAdapter(ADAPTER *adapter)
{
    FIXME("adapter: %p\n", adapter);
}
