#define MAX_LINK_NAME_LENGTH 64
#define ADAPTER_NAME_LENGTH 256 + 12

typedef struct _PACKET {
    HANDLE       hEvent;
    OVERLAPPED   OverLapped;
    PVOID        Buffer;
    UINT         Length;
    DWORD        ulBytesReceived;
    BOOLEAN      bIoComplete;
}  PACKET, *LPPACKET;

typedef struct WAN_ADAPTER_INT WAN_ADAPTER;
typedef WAN_ADAPTER *PWAN_ADAPTER;

typedef struct _ADAPTER  {
    HANDLE hFile;
    char SymbolicLink[MAX_LINK_NAME_LENGTH];
    int NumWrites;
    HANDLE ReadEvent;
    UINT ReadTimeOut;
    char Name[ADAPTER_NAME_LENGTH];
    PWAN_ADAPTER pWanAdapter;
    UINT Flags;
}  ADAPTER, *LPADAPTER;

LPPACKET PacketAllocatePacket(void);
VOID PacketFreePacket(PACKET *packet);
BOOLEAN PacketGetAdapterNames(char *buffer, DWORD *size);
LPADAPTER PacketOpenAdapter(char *adapter_name);
VOID PacketCloseAdapter(ADAPTER *adapter);
