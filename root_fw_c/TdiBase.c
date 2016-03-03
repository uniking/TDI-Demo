#include "TdiBase.h"

VOID 
FsSwapBytes(
			IN OUT PCHAR buf,
			IN USHORT size
			) 
{ 
	CHAR tmp; 	
	switch (size) 
	{ 
	case 2: 
		tmp=buf[0]; buf[0]=buf[1]; buf[1]=tmp; 
		break; 
	case 4: 
		tmp=buf[0]; buf[0]=buf[3]; buf[3]=tmp; 
		tmp=buf[1]; buf[1]=buf[2]; buf[2]=tmp; 
		break; 
	case 6:
		tmp=buf[0]; buf[0]=buf[5]; buf[5]=tmp; 
		tmp=buf[1]; buf[1]=buf[4]; buf[4]=tmp; 
		tmp=buf[2]; buf[2]=buf[3]; buf[3]=tmp; 
		break;
	case 8: 
		tmp=buf[0]; buf[0]=buf[7]; buf[7]=tmp; 
		tmp=buf[1]; buf[1]=buf[6]; buf[6]=tmp; 
		tmp=buf[2]; buf[2]=buf[5]; buf[5]=tmp; 
		tmp=buf[3]; buf[3]=buf[4]; buf[4]=tmp; 
		break; 
	default: 
		break; 
	} 
	return; 
} 
ULONG fs_ntohl(ULONG ulVal)
{
	ULONG ulTmp = ulVal;

	FsSwapBytes((PCHAR)&ulTmp, sizeof(ulTmp));

	return ulTmp;
}
USHORT fs_ntohs(USHORT usVal)
{
	USHORT usTmp = usVal;

	FsSwapBytes((PCHAR)&usTmp, sizeof(usTmp));

	return usTmp;
}
ULONG fs_htonl(ULONG ulVal)
{
	ULONG ulTmp = ulVal;

	FsSwapBytes((PCHAR)&ulTmp, sizeof(ulTmp));

	return ulTmp;
}
USHORT fs_htons(USHORT usVal)
{
	USHORT	usTmp = usVal;

	FsSwapBytes((PCHAR)&usTmp, sizeof(usTmp));

	return usTmp;
}

BOOLEAN
TdiGetIpAddressAndPort(
						 IN PTRANSPORT_ADDRESS TransportAddress,
						 OUT PULONG IpAddress,
						 OUT PUSHORT IpPort
						 )
{
	PTA_ADDRESS TaAddress = (PTA_ADDRESS)TransportAddress->Address;
	PTDI_ADDRESS_IP TdiAddress = (PTDI_ADDRESS_IP)TaAddress->Address;

	if (TaAddress->AddressType != TDI_ADDRESS_TYPE_IP)
	{
		return FALSE;
	}

	// 转换网络字节为主机字节
	*IpAddress = (ULONG)fs_ntohl(TdiAddress->in_addr);
	*IpPort = (USHORT)fs_ntohs(TdiAddress->sin_port);

	return TRUE;
}

BOOLEAN
TdiGetIpAddressAndPortByTAAddress(
					   IN PTA_ADDRESS TaAddress,
					   OUT PULONG IpAddress,
					   OUT PUSHORT IpPort
					   )
{
	PTDI_ADDRESS_IP TdiAddress = (PTDI_ADDRESS_IP)TaAddress->Address;

	if (TaAddress->AddressType != TDI_ADDRESS_TYPE_IP)
	{
		return FALSE;
	}

	// 转换网络字节为主机字节
	*IpAddress = (ULONG)fs_ntohl(TdiAddress->in_addr);
	*IpPort = (USHORT)fs_ntohs(TdiAddress->sin_port);

	return TRUE;
}