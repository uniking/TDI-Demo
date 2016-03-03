#include "NetBIOS.h"
#include "smb.h"

UINT netbios_send_ndis_packet(PAPP_FICTION_PROCESS pProcess)
{
	UINT status = 0;

	if (pProcess->sendBuffer.MaximumLength - 4 >4)
	{
		pProcess->sendBuffer.Offset += 4;
		pProcess->sendBuffer.length -= 4;
		status = smb_send_ndis_packet(pProcess);
		pProcess->sendBuffer.Offset -= 4;
		pProcess->sendBuffer.length +=4;
	}

	return status;
}
UINT netbios_receive_ndis_packet(PAPP_FICTION_PROCESS pProcess)
{
	UINT status = 0;

	if (pProcess->receiveBuffer.MaximumLength - 4 >4)
	{
		pProcess->receiveBuffer.Offset += 4;
		pProcess->receiveBuffer.length -= 4;
		status = smb_receive_ndis_packet(pProcess);
		pProcess->receiveBuffer.Offset -= 4;
		pProcess->receiveBuffer.length +=4;
	}
	return status;
}

VOID netbios_clear_receive_buffer(PAPP_FICTION_PROCESS pProcess)
{
	smbClearReceiveBuffer(pProcess);
}