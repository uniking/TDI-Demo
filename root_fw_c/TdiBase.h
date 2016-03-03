#ifndef _TDI_BASE_H
#define _TDI_BASE_H

#include <ntdef.h>
#include <tdi.h>

BOOLEAN
TdiGetIpAddressAndPort(
						 IN PTRANSPORT_ADDRESS TransportAddress,
						 OUT PULONG IpAddress,
						 OUT PUSHORT IpPort
						 );

BOOLEAN
TdiGetIpAddressAndPortByTAAddress(
								  IN PTA_ADDRESS TaAddress,
								  OUT PULONG IpAddress,
								  OUT PUSHORT IpPort
								  );
#endif

