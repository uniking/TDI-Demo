#ifndef _APP_PROTOCOL_H
#define _APP_PROTOCOL_H
#include <ntdef.h>
#include <tdi.h>
#include <wdm.h>
#include "stdafx.h"

typedef unsigned int UINT, *PUINT;


UINT AppProtocolAnalyze(
						BOOLEAN SendData,
						PUCHAR buffer, 
						PULONG length, 
						PFILE_OBJECT AddressFileObject,
						PFILE_OBJECT ConnectFileObject
						);

VOID AppClearReceiveBuffer(PFILE_OBJECT AddressFileObject);

#endif