#include "stdafx.h"
#include "TdiFltDisp.h"
//#include "TdiBase.h"
#include "root_fw.h"
#include "AppProtocol.h"

#include <tdikrnl.h>


TCPSendDgData_t *g_TCPSendDgData = NULL;
TCPSendData_t *g_TCPSendData = NULL;

typedef struct _SEND_COMPLETE
{
	PMDL oldMdl;
	PVOID OldContext;
	PVOID OldCompletionRoutine;
	PVOID newBuffer;
	PVOID newMdl;
}SEND_COMPLETE, *PSEND_COMPLETE;

//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSendDatagram
// 说明: TDI_SEND_DATAGRAM 发送面向无连接的数据包
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 拦截就返回 STATUS_INVALID_ADDRESS
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSendDatagram( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION IrpSP;
    PTDI_REQUEST_KERNEL_SENDDG SenddgRequest;
    PCONNECTION_ITEM ConnectItem;
    PUCHAR pSendData;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        SenddgRequest = (PTDI_REQUEST_KERNEL_SENDDG)(&IrpSP->Parameters);
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );
        pSendData = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

        if ( ConnectItem != NULL )
        {
            DbgMsg(__FILE__, __LINE__, "SendDatagram Data: %08X, SendLength: %d.\n", 
                pSendData, SenddgRequest->SendLength );
        }
    }

    // 提交给下层设备对象发送
    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}

//NTSTATUS
//TdiCompletionRoutine(
//					IN PDEVICE_OBJECT   DeviceObject,
//					IN PIRP             Irp,
//					IN PVOID            Context
//					)
//{
//	if (Irp->PendingReturned == TRUE) {
//		KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
//	}
//	// This is the only status you can return.
//	return STATUS_MORE_PROCESSING_REQUIRED;  
//}

//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSend
// 说明: TDI_SEND 发送数据包
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 拦截就返回 STATUS_FILE_CLOSED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
//NTSTATUS TdiDispSend( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
//{
//	PIO_STACK_LOCATION IrpSP;
//	PTDI_REQUEST_KERNEL_SEND param;
//	PUCHAR pSendData;
//	PCONNECTION_ITEM ConnectItem;
//	PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
//
//	if ( Irp->CurrentLocation > 1 )
//	{
//		IrpSP = IoGetCurrentIrpStackLocation( Irp );
//		param = (PTDI_REQUEST_KERNEL_SEND)(&IrpSP->Parameters);
//		pSendData = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
//		ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );
//
//		// 打印发送数据信息
//		if ( ConnectItem != NULL )
//		{
//			DbgMsg(__FILE__, __LINE__, "Send Data: %08X, SendLength: %d\n", 
//				pSendData, param->SendLength);
//		}
//	}
//
//	// 提交给下层设备对象发送处理
//	IoSkipCurrentIrpStackLocation( Irp );
//	return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
//}

NTSTATUS TdiDispSend( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION IrpSP = NULL;
	KEVENT   event;
    PTDI_REQUEST_KERNEL_SEND param = NULL;
    PUCHAR pSendData = NULL;
    PCONNECTION_ITEM ConnectItem = NULL;
	ULONG IpAddress = 0;
	USHORT IpPort = 0;
	ULONG oldLength = 0;
	PMDL myMdl = NULL;
	PMDL oldMdl = NULL;
	PMDL TempMdl = NULL;
	PUCHAR pMdl = NULL;
	NTSTATUS ntStatus = 0;
	UINT uStatus = 0;
	ULONG SendLength = 0;
	ULONG uOffset = 0;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	do 
	{
		 if ( Irp->CurrentLocation <= 1 )
		 {
			 break;
		 }

		 IrpSP = IoGetCurrentIrpStackLocation( Irp );
		 param = (PTDI_REQUEST_KERNEL_SEND)(&IrpSP->Parameters);

		 for (SendLength = 0, TempMdl = Irp->MdlAddress; TempMdl != NULL ; TempMdl = TempMdl->Next)
		 {
			 SendLength += MmGetMdlByteCount( TempMdl );
		 }

		 do 
		 {
			 pSendData = ExAllocatePoolWithTag(NonPagedPool, SendLength, 'xxxx');
		 } while (pSendData == NULL);

		 for(uOffset=0, TempMdl =  Irp->MdlAddress; TempMdl != NULL; TempMdl = TempMdl->Next)
		 {
			 SendLength = MmGetMdlByteCount( TempMdl );
			 pMdl = (PUCHAR)MmGetSystemAddressForMdlSafe( TempMdl, NormalPagePriority );
			 RtlCopyMemory(pSendData + uOffset, pMdl, SendLength);
			 uOffset += SendLength;
		 }

		 SendLength = param->SendLength;
		 oldLength = param->SendLength;
		 uStatus = AppProtocolAnalyze(
			 TRUE,
			 pSendData, 
			 &SendLength, 
			 NULL,
			 IrpSP->FileObject
			 );

		 if (!FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
		 {
			 break;
		 }

		 myMdl = IoAllocateMdl(
			 pSendData,
			 SendLength,
			 FALSE,
			 FALSE,
			 NULL
			 );
		 MmBuildMdlForNonPagedPool(myMdl);

	} while (FALSE);

	if (FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
	{
		KeInitializeEvent(&event, NotificationEvent, FALSE);

		oldMdl = Irp->MdlAddress;
		oldLength = param->SendLength;
		Irp->MdlAddress = myMdl;
		param->SendLength = SendLength;

		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp,
			TdiCompletionRoutine,
			&event,
			TRUE,
			TRUE,
			TRUE
			);
		ntStatus = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
		if (ntStatus == STATUS_PENDING)
		{
			KeWaitForSingleObject(&event,
				Executive, // WaitReason
				KernelMode, // must be Kernelmode to prevent the stack getting paged out
				FALSE,
				NULL // indefinite wait
				);
			ntStatus = Irp->IoStatus.Status;
		}

		Irp->MdlAddress = oldMdl;
		param->SendLength = oldLength;

		ExFreePoolWithTag(pSendData, 'xxxx');
		IoFreeMdl(myMdl);

		IoCompleteRequest (Irp, IO_NO_INCREMENT);
	}
	else
	{
		// 提交给下层设备对象发送处理
		IoSkipCurrentIrpStackLocation( Irp );
		ntStatus = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
		if (ntStatus == STATUS_PENDING)
		{
			//KdPrint(("STATUS_PENDING\n"));
		}

		if (pSendData != NULL)
		{
			ExFreePoolWithTag(pSendData, 'xxxx');
		}
	}

    return ntStatus;
}



/*
 * for IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER
 */
//NTSTATUS new_TCPSendData(IN PIRP Irp, IN PIO_STACK_LOCATION parIrpSp)
//{
//	UINT uStatus = 0;
//	NTSTATUS ntStatus = 0;
//	PUCHAR pSendData;
//	ULONG SendLength;
//	PIO_STACK_LOCATION IrpSP;
//	PTDI_REQUEST_KERNEL_SEND param;
//	ULONG oldLength = 0;
//	ULONG mdlLength = 0;
//	PMDL oldMdl = NULL;
//	PMDL myMdl = NULL;
//
//	IrpSP = IoGetCurrentIrpStackLocation( Irp );
//	param = (PTDI_REQUEST_KERNEL_SEND)(&IrpSP->Parameters);
//	pSendData = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
//	mdlLength = MmGetMdlByteCount(Irp->MdlAddress);
//
//	SendLength = param->SendLength <= mdlLength ?param->SendLength:mdlLength;
//	oldLength = SendLength;
//
//	//uStatus = AppProtocolAnalyze(
//	//	TRUE,
//	//	&pSendData, 
//	//	&SendLength, 
//	//	NULL,
//	//	IrpSP->FileObject
//	//	);
//	if (FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
//	{
//		KdPrint(("Change packet data\n"));
//
//		myMdl = IoAllocateMdl(
//			pSendData,
//			SendLength,
//			FALSE,
//			FALSE,
//			NULL
//			);
//		MmBuildMdlForNonPagedPool(myMdl);
//
//		oldMdl = Irp->MdlAddress;
//		Irp->MdlAddress = myMdl;
//
//		param->SendLength = SendLength;
//
//		ntStatus = g_TCPSendData(Irp, IrpSP);
//
//		if (STATUS_SUCCESS == ntStatus)
//		{
//			Irp->MdlAddress = oldMdl;
//			param->SendLength = oldLength;
//
//			ExFreePoolWithTag(pSendData, 'xxxx');
//			IoFreeMdl(myMdl);
//		}
//		else if(STATUS_PENDING == ntStatus)
//		{
//			KdPrint(("pending\n"));
//		}
//	}
//	else
//	{
//		ntStatus = g_TCPSendData(Irp, parIrpSp);
//	}
//
//	
//	return ntStatus;
//}

NTSTATUS
	hookSendCompleteRoutine(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp,
	__in_xcount_opt("varies") PVOID Context
	)
{
	NTSTATUS ntStatus = 0;
	PSEND_COMPLETE sendContext = (PSEND_COMPLETE)Context;
	PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION nexIrpSp = IoGetNextIrpStackLocation(Irp);
	PVOID OldContext = NULL;

	////还原原始Irp
	nexIrpSp->CompletionRoutine = sendContext->OldCompletionRoutine;
	nexIrpSp->Context = sendContext->OldContext;
	Irp->MdlAddress = sendContext->oldMdl;

	OldContext = sendContext->OldContext;

	////释放自己的内存
	ExFreePoolWithTag(sendContext->newBuffer, 'xxxx');
	IoFreeMdl(sendContext->newMdl);
	ExFreePoolWithTag(sendContext, 'xxxx');

	//原函数没有等待，而是返回0
	return nexIrpSp->CompletionRoutine(DeviceObject, Irp, OldContext);
}
NTSTATUS new_TCPSendData(IN PIRP Irp, IN PIO_STACK_LOCATION parIrpSp)
{
	UINT uStatus = 0;
	NTSTATUS ntStatus = 0;
	PUCHAR pSendData = NULL;
	ULONG SendLength = 0;
	PIO_STACK_LOCATION IrpSP = NULL;
	PTDI_REQUEST_KERNEL_SEND param = NULL;
	ULONG mdlLength = 0;
	PMDL oldMdl = NULL;
	PMDL myMdl = NULL;
	PMDL tempMdl = NULL;
	PUCHAR pMdlVa = NULL;
	UINT dataoffset = 0;
	PSEND_COMPLETE sendContext = NULL;

	PIRP newIrp = NULL;

	IrpSP = IoGetCurrentIrpStackLocation( Irp );
	param = (PTDI_REQUEST_KERNEL_SEND)(&IrpSP->Parameters);

	do 
	{
		pSendData = ExAllocatePoolWithTag(NonPagedPool, param->SendLength+512, 'xxxx');
	} while (pSendData == NULL);
	

	for (tempMdl = Irp->MdlAddress;tempMdl != NULL; tempMdl = tempMdl->Next)
	{
		pMdlVa = (PUCHAR)MmGetSystemAddressForMdlSafe(tempMdl, NormalPagePriority);
		mdlLength = MmGetMdlByteCount(tempMdl);
		RtlCopyMemory(pSendData+dataoffset, pMdlVa, mdlLength);

		dataoffset+=mdlLength;
		ASSERT(param->SendLength >= dataoffset);
	}

	SendLength = dataoffset;
	uStatus = AppProtocolAnalyze(
		TRUE,
		pSendData, 
		&SendLength, 
		NULL,
		IrpSP->FileObject
		);

	if (!FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
	{
		ExFreePoolWithTag(pSendData, 'xxxx');
		ntStatus = g_TCPSendData(Irp, IrpSP);
	}
	else
	{//替换MDL，hook完成函数
		myMdl = IoAllocateMdl(
			pSendData,
			param->SendLength,
			FALSE,
			FALSE,
			NULL
			);
		MmBuildMdlForNonPagedPool(myMdl);

		sendContext = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(SEND_COMPLETE), 'xxxx');

		//保存释放数据
		sendContext->newBuffer = pSendData;
		sendContext->newMdl = myMdl;

		//保存还原数据
		sendContext->OldCompletionRoutine = IrpSP->CompletionRoutine;
		sendContext->OldContext = IrpSP->Context;
		sendContext->oldMdl = Irp->MdlAddress;

		//hook
		IrpSP->CompletionRoutine = hookSendCompleteRoutine;
		IrpSP->Context = sendContext;
		Irp->MdlAddress = myMdl;

		ntStatus = g_TCPSendData(Irp, IrpSP);

	}

	return ntStatus;
}
//NTSTATUS new_TCPSendData(IN PIRP Irp, IN PIO_STACK_LOCATION parIrpSp)
//{
//	UINT uStatus = 0;
//	NTSTATUS ntStatus = 0;
//	PUCHAR pSendData;
//	ULONG SendLength;
//	PIO_STACK_LOCATION IrpSP;
//	PIO_STACK_LOCATION myIrpSP;
//	PTDI_REQUEST_KERNEL_SEND param;
//	ULONG oldLength = 0;
//	ULONG mdlLength = 0;
//	PMDL oldMdl = NULL;
//	PMDL myMdl = NULL;
//	PMDL tempMdl = NULL;
//	PUCHAR pMdlVa = NULL;
//	UINT dataoffset = 0;
//	PSEND_COMPLETE sendContext = NULL;
//
//	PIRP newIrp = NULL;
//
//	IrpSP = IoGetCurrentIrpStackLocation( Irp );
//	param = (PTDI_REQUEST_KERNEL_SEND)(&IrpSP->Parameters);
//
//	pSendData = ExAllocatePoolWithQuotaTag(PagedPool, param->SendLength, 'xxxx');
//	for (tempMdl = Irp->MdlAddress;tempMdl != NULL; tempMdl = tempMdl->Next)
//	{
//		pMdlVa = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
//		mdlLength = MmGetMdlByteCount(Irp->MdlAddress);
//		RtlCopyMemory(pSendData+dataoffset, pMdlVa, mdlLength);
//
//		dataoffset+=mdlLength;
//	}
//
//	{//替换MDL，hook完成函数
//
//		myMdl = IoAllocateMdl(
//			pSendData,
//			param->SendLength,
//			FALSE,
//			FALSE,
//			NULL
//			);
//		MmBuildMdlForNonPagedPool(myMdl);
//
//		//sendContext = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(SEND_COMPLETE), 'xxxx');
//
//		//sendContext->newBuffer = pSendData;
//		//sendContext->newMdl = myMdl;
//		//sendContext->OldCompletionRoutine = IrpSP->CompletionRoutine;
//		//sendContext->OldContext = IrpSP->Context;
//		//sendContext->oldMdl = Irp->MdlAddress;
//
//		oldMdl = Irp->MdlAddress;
//		Irp->MdlAddress = myMdl;
//		//IrpSP->CompletionRoutine = hookSendCompleteRoutine;
//		//IrpSP->Context = sendContext;
//		
//	}
//
//	oldLength = param->SendLength;
//
//	{
//		KEVENT kEvent;
//		IO_STATUS_BLOCK iosb;
//
//		// 设置事件通知完成例程
//		KeInitializeEvent( &kEvent, NotificationEvent, FALSE );
//		IoCopyCurrentIrpStackLocationToNext(Irp);
//		IoSetCompletionRoutine(Irp,
//			TdiCompletionRoutine,
//			&kEvent,
//			TRUE,
//			TRUE,
//			TRUE
//			);
//		status = IoCallDriver(NextDevice, Irp );
//		myIrpSP = IoGetNextIrpStackLocation(Irp);
//		myIrpSP->MajorFunction = IrpSP->MajorFunction;
//		myIrpSP->MinorFunction = IrpSP->MinorFunction;
//		myIrpSP->DeviceObject = IrpSP->DeviceObject;
//		myIrpSP->Control = IrpSP->Control;
//		myIrpSP->FileObject = IrpSP->FileObject;
//
//		ntStatus = g_TCPSendData(Irp, myIrpSP);
//		if (ntStatus == STATUS_PENDING)
//		{
//			KeWaitForSingleObject(&kEvent,
//				Executive, // WaitReason
//				KernelMode, // must be Kernelmode to prevent the stack getting paged out
//				FALSE,
//				NULL // indefinite wait
//				);
//			ntStatus = Irp->IoStatus.Status;
//		}
//
//		释放自己的内存
//		ExFreePoolWithTag(pSendData, 'xxxx');
//		IoFreeMdl(myMdl);
//		ExFreePoolWithTag(sendContext, 'xxxx');
//
//		Irp->MdlAddress = oldMdl;
//		IoCompleteRequest (Irp, IO_NO_INCREMENT);
//	}
//
//	
//
//	return ntStatus;
//}

/*
 * for IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER
 */
NTSTATUS new_TCPSendDgData(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp)
{
	

	return g_TCPSendDgData(Irp, IrpSp);
}