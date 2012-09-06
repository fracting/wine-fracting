typedef struct _PACKET {
    HANDLE       hEvent;
    OVERLAPPED   OverLapped;
    PVOID        Buffer;
    UINT         Length;
    DWORD        ulBytesReceived;
    BOOLEAN      bIoComplete;
}  PACKET, *LPPACKET;

LPPACKET PacketAllocatePacket(void);
