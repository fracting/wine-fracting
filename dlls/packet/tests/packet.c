#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <packet32.h>
#include <string.h>

#include "wine/test.h"

void test_PacketAllocatePacket(void)
{
    LPPACKET packet1, packet2;
    packet1 = PacketAllocatePacket();
    todo_wine ok(packet1 != NULL, "packet allocate fails.\n");

    packet2 = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PACKET));
    if(packet1)
    {
        todo_wine ok(memcmp(packet1, packet2, sizeof(PACKET)) == 0, "packet should be zero initialized.\n");
        PacketFreePacket(packet1);
    }
    HeapFree(GetProcessHeap(), 0, packet2);
}

START_TEST( packet )
{
    test_PacketAllocatePacket();
}
