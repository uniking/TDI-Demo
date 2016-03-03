#ifndef __PACKET_NETBIOS_H__
#define __PACKET_NETBIOS_H__
#include <ndis.h>
#include "stdafx.h"

typedef struct _NET_BIOS
{
	//UCHAR MgsType;		//1
	//ULONG length;        //3
	ULONG NetBios;
}NET_BIOS, *PNET_BIOS;//sizeof(NET_BIOS)==4

UINT netbios_send_ndis_packet(PAPP_FICTION_PROCESS pProcess);
UINT netbios_receive_ndis_packet(PAPP_FICTION_PROCESS pProcess);

VOID netbios_clear_receive_buffer(PAPP_FICTION_PROCESS pProcess);

#endif