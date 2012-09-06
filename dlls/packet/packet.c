#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

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
    FIXME("stub!\n");
    return NULL;
}

VOID PacketFreePacket(PACKET *packet)
{
    FIXME("stub %p\n", packet);
}
