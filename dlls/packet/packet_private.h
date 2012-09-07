#include "packet32.h"
#include <pcap/pcap.h>

typedef struct _ADAPTER_EX {
    ADAPTER adapter;
    pcap_t *pcap_handle;
}  ADAPTER_EX;

