#include "stdafx.h"
#include "TdiSetEventHandler.h"
#include "TdiBase.h"
#include "AppProtocol.h"

typedef struct _Chained_Receive_Parameter
{
	IN PVOID  TdiEventContext;
	IN CONNECTION_CONTEXT  ConnectionContext;
	IN ULONG  ReceiveFlags;
	IN ULONG  ReceiveLength;
	IN ULONG  StartingOffset;
	IN PMDL  Tsdu;
	IN PVOID  TsduDescriptor;
}Chained_Receive_Parameter, *PChained_Receive_Parameter;
typedef struct _Chained_Receive_Packet
{
	LIST_ENTRY node;
	Chained_Receive_Parameter Para; //用于向上层传递数据
	PUCHAR buffer;//第一个分析时自己分配的buffer，内容只有TCP的数据部分
	ULONG length;//buffer的长度
}Chained_Receive_Packet, *PChained_Receive_Packet;


typedef struct _tdi_client_irp_ctx {
	PIO_COMPLETION_ROUTINE	completion;
	PVOID					context;
	UCHAR					old_control;
	//PFILE_OBJECT            connobj;
	PFILE_OBJECT AddressFileObjec;
}tdi_client_irp_ctx, *ptdi_client_irp_ctx;

NTSTATUS
ReceiveEventHandlerComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	ptdi_client_irp_ctx ctx = (ptdi_client_irp_ctx)Context;
	NTSTATUS status;
	UINT uStatus = 0;
	PMDL myMdl = NULL;
	PMDL oldMdl = NULL;
	ULONG oldLength = 0;
	PIO_STACK_LOCATION Irpsp = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION IrpspNext = IoGetNextIrpStackLocation(Irp);

	KdPrint(("[tdi_fw] tdi_client_irp_complete: status: 0x%x; len: %u\n",
		Irp->IoStatus.Status, Irp->IoStatus.Information));

	if (Irp->IoStatus.Status == STATUS_SUCCESS) 
	{
		PMDL tempMdl = NULL;
		PUCHAR pMdlVa = NULL;
		UINT dataoffset = 0;
		ULONG mdlLength = 0;
		ULONG mdlMaxLenght = 0;
		PUCHAR pReceiveData = NULL;
		ULONG Length = 0;
		
		oldLength = Irp->IoStatus.Information;
		for (tempMdl = Irp->MdlAddress;tempMdl != NULL; tempMdl = tempMdl->Next)
		{
			mdlLength = MmGetMdlByteCount(tempMdl);
			mdlMaxLenght+=mdlLength;
		}

		do 
		{
			pReceiveData = ExAllocatePoolWithTag(NonPagedPool,  mdlMaxLenght+512, 'xxxx');
		} while (pReceiveData == NULL);
		
		for (tempMdl = Irp->MdlAddress;tempMdl != NULL; tempMdl = tempMdl->Next)
		{
			pMdlVa = (PUCHAR)MmGetSystemAddressForMdlSafe(tempMdl, NormalPagePriority);
			mdlLength = MmGetMdlByteCount(tempMdl);
			RtlCopyMemory(pReceiveData+dataoffset, pMdlVa, mdlLength);

			dataoffset+=mdlLength;

			//ASSERT(dataoffset <= oldLength);
		}

		Length = oldLength;
		uStatus = AppProtocolAnalyze(
			FALSE,
			pReceiveData, 
			&Length, 
			ctx->AddressFileObjec,
			NULL
			);
		if (FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
		{
			/*myMdl = IoAllocateMdl(
				pReceiveData,
				Length,
				FALSE,
				FALSE,
				NULL
				);
			MmBuildMdlForNonPagedPool(myMdl);

			oldMdl = Irp->MdlAddress;
			Irp->MdlAddress = myMdl;
			Irp->IoStatus.Information = Length;*/

			//直接修改上层mdl中的数据
			pMdlVa = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
			mdlLength = MmGetMdlByteCount(Irp->MdlAddress);
			RtlCopyMemory(pMdlVa, pReceiveData, Length);
			Irp->IoStatus.Information = Length;
			ExFreePoolWithTag(pReceiveData, 'xxxx');
		}
		else
		{
			ExFreePoolWithTag(pReceiveData, 'xxxx');
		}
	}

	// call original completion
	if (ctx->completion != NULL)
	{
		// call old completion (see the old control)
		BOOLEAN b_call = FALSE;

		if (Irp->Cancel)
		{
			// cancel
			if (ctx->old_control & SL_INVOKE_ON_CANCEL)
				b_call = TRUE;
		}
		else
		{
			if (Irp->IoStatus.Status >= STATUS_SUCCESS) 
			{
				// success
				if (ctx->old_control & SL_INVOKE_ON_SUCCESS)
					b_call = TRUE;
			} else {
				// error
				if (ctx->old_control & SL_INVOKE_ON_ERROR)
					b_call = TRUE;
			}
		}

		if (b_call)
		{
			status = (ctx->completion)(DeviceObject, Irp, ctx->context);
			//替换没用，完成函数还是使用自己的mdl，需要在上层的mdl中修改数据

			//
			//if (FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
			//{
				//Irp->MdlAddress = oldMdl;
				//Irp->IoStatus.Information = oldLength;
				//释放buffer mdl
			//}
			//KdPrint(("[tdi_flt] tdi_client_irp_complete: original handler: 0x%x; status: 0x%x\n",
			//	ctx->completion, status));

		} else
			status = STATUS_SUCCESS;

	}

	if (STATUS_SUCCESS == status)
	{
		ExFreePoolWithTag(ctx, 'xxxx');
	}
	//free(ctx);

	//Irpsp = IoGetNextIrpStackLocation(Irp);
	//Irpsp->Control = ctx->old_control;
	//status = (ctx->completion)(DeviceObject, Irp, ctx->context);
	
	return status;
}

//////////////////////////////////////////////////////////////////////////
// 名称: ReceiveEventHandlerEx (TDI_EVENT_RECEIVE and TDI_EVENT_RECEIVE_EXPEDITED)
// 说明: 数据接收事件据操作函数 (面向连接)
// 入参: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 出参: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 返回: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 备注: 如果不接收,则返回 STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ReceiveEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN ULONG  ReceiveFlags, 
    IN ULONG  BytesIndicated, 
    IN ULONG  BytesAvailable, 
    OUT ULONG  *BytesTaken, 
    IN PVOID  Tsdu, 
    OUT PIRP  *IoRequestPacket 
    )
{
    NTSTATUS status = 0;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;
	PUCHAR pData = NULL;
	UINT uStatus = 0;
	ULONG Length = 0;
	ULONG MyBytesTaken = 0;

	Length = BytesAvailable;

	do 
	{
		pData = ExAllocatePoolWithTag(NonPagedPool, BytesAvailable+512, 'xxxx');
	} while (pData == NULL);
	
	RtlCopyMemory(pData, Tsdu, BytesAvailable);

	uStatus = AppProtocolAnalyze(
		FALSE,
		pData, 
		&Length, 
		pEventContext->FileObject,
		NULL
		);

	if (FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
	{
		status = ((PTDI_IND_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
			ConnectionContext, ReceiveFlags, Length, Length, 
			BytesTaken, pData, IoRequestPacket );
		//*BytesTaken = BytesAvailable;
		if (status == STATUS_MORE_PROCESSING_REQUIRED)
		{
			AppClearReceiveBuffer(pEventContext->FileObject);
		}

		if (*IoRequestPacket != NULL) 
		{
			// got IRP. replace completion.
			ptdi_client_irp_ctx new_ctx;
			PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(*IoRequestPacket);

			new_ctx = (ptdi_client_irp_ctx)ExAllocatePoolWithTag(NonPagedPool, sizeof(tdi_client_irp_ctx), 'xxxx');
			if (new_ctx != NULL) 
			{

				new_ctx->AddressFileObjec = pEventContext->FileObject;

				if (irps->CompletionRoutine != NULL) 
				{
					new_ctx->completion = irps->CompletionRoutine;
					new_ctx->context = irps->Context;
					new_ctx->old_control = irps->Control;

				}
				else
				{
					// we don't use IoSetCompletionRoutine because it uses next not current location

					new_ctx->completion = NULL;
					new_ctx->context = NULL;

				}

				irps->CompletionRoutine = ReceiveEventHandlerComplete;
				irps->Context = new_ctx;
				irps->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
			}
		}

		if (status == STATUS_SUCCESS)
		{
			ExFreePoolWithTag(pData, 'xxxx');
		}
		else if (status == STATUS_MORE_PROCESSING_REQUIRED)
		{
			ExFreePoolWithTag(pData, 'xxxx');
		}
		else
		{
			ASSERT(FALSE);
		}
	}
	else
	{
		ExFreePoolWithTag(pData, 'xxxx');
		// 调用原来的处理函数，提交给上层
		status = ((PTDI_IND_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
			ConnectionContext, ReceiveFlags, BytesIndicated, BytesAvailable, 
			BytesTaken, Tsdu, IoRequestPacket );
	}

   /* DbgMsg(__FILE__, __LINE__, "ReceiveEventHandler Status: %08X, BytesTaken: %u, ReceiveFlags: 0x%X.\n", 
        status, *BytesTaken, ReceiveFlags );*/

    return status;
}

//NTSTATUS ReceiveEventHandler( IN PVOID  TdiEventContext, 
//    IN CONNECTION_CONTEXT  ConnectionContext, 
//    IN ULONG  ReceiveFlags, 
//    IN ULONG  BytesIndicated, 
//    IN ULONG  BytesAvailable, 
//    OUT ULONG  *BytesTaken, 
//    IN PVOID  Tsdu, 
//    OUT PIRP  *IoRequestPacket 
//    )
//{
//    NTSTATUS status;
//    PIO_STACK_LOCATION IrpSP;
//    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;
//	PUCHAR pData;
//	UINT uStatus = 0;
//	ULONG Length;
//	ULONG MyBytesTaken;
//
//
//	// 调用原来的处理函数，提交给上层
//	status = ((PTDI_IND_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
//		ConnectionContext, ReceiveFlags, BytesIndicated, BytesAvailable, 
//		BytesTaken, Tsdu, IoRequestPacket );
//
//		if (*IoRequestPacket != NULL) 
//		{
//			// got IRP. replace completion.
//			ptdi_client_irp_ctx new_ctx;
//			PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(*IoRequestPacket);
//
//			new_ctx = (ptdi_client_irp_ctx)ExAllocatePoolWithTag(NonPagedPool, sizeof(tdi_client_irp_ctx), 'xxxx');
//			if (new_ctx != NULL) 
//			{
//
//				new_ctx->AddressFileObjec = pEventContext->FileObject;
//
//				if (irps->CompletionRoutine != NULL) 
//				{
//					new_ctx->completion = irps->CompletionRoutine;
//					new_ctx->context = irps->Context;
//					new_ctx->old_control = irps->Control;
//
//				}
//				else
//				{
//					// we don't use IoSetCompletionRoutine because it uses next not current location
//
//					new_ctx->completion = NULL;
//					new_ctx->context = NULL;
//
//				}
//
//				irps->CompletionRoutine = ReceiveEventHandlerComplete;
//				irps->Context = new_ctx;
//				irps->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
//			}
//		}
//
//
//
//
//
//    return status;
//}



//NTSTATUS ReceiveEventHandler( IN PVOID  TdiEventContext, 
//							 IN CONNECTION_CONTEXT  ConnectionContext, 
//							 IN ULONG  ReceiveFlags, 
//							 IN ULONG  BytesIndicated, 
//							 IN ULONG  BytesAvailable, 
//							 OUT ULONG  *BytesTaken, 
//							 IN PVOID  Tsdu, 
//							 OUT PIRP  *IoRequestPacket 
//							 )
//{
//	NTSTATUS status = 0;
//	PIO_STACK_LOCATION IrpSP;
//	PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;
//	PUCHAR pData;
//	UINT uStatus = 0;
//	ULONG Length;
//	ULONG MyBytesTaken;
//
//
//	pData = Tsdu;
//	Length = BytesAvailable;
//
//	
//	//// 调用原来的处理函数，提交给上层
//	status = ((PTDI_IND_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
//		ConnectionContext, ReceiveFlags, BytesIndicated, BytesAvailable, 
//		BytesTaken, Tsdu, IoRequestPacket );
//
//	return status;
//}



//////////////////////////////////////////////////////////////////////////
// 名称: ChainedReceiveEventHandler (TDI_EVENT_CHAINED_RECEIVE and TDI_EVENT_CHAINED_RECEIVE_EXPEDITED)
// 说明: 链式数据接收操作函数 (面向连接)
// 入参: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 出参: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 返回: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 备注: 拦截数据,则返回 STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ChainedReceiveEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN ULONG  ReceiveFlags, 
    IN ULONG  ReceiveLength, 
    IN ULONG  StartingOffset, 
    IN PMDL  Tsdu, 
    IN PVOID  TsduDescriptor 
    )
{
    NTSTATUS status;
	UINT uStatus = 0;
    ULONG uDataLength = 0;
	ULONG uOffset = 0;
	PUCHAR pMdl = NULL;
    PUCHAR pData = NULL;
	PUCHAR pNewData = NULL;
	UINT uNewDataOffset = 0;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;
	PMDL myMdl = NULL;
	PMDL TempMdl = NULL;
	ULONG   Length = 0;
	PLIST_ENTRY pTemp = NULL;
	PChained_Receive_Packet pPacket = NULL;


	//status = ((PTDI_IND_CHAINED_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
	//	ConnectionContext, ReceiveFlags, ReceiveLength, StartingOffset, Tsdu, TsduDescriptor );

	do 
	{
		// 获取接收到的数据信息



		for (TempMdl = Tsdu; TempMdl != NULL; TempMdl = TempMdl->Next)
		{
			uDataLength += MmGetMdlByteCount( TempMdl );
		}

		do 
		{
			pData = ExAllocatePoolWithTag(NonPagedPool, uDataLength+512, 'xxxx');
		} while (pData == NULL);
		

		for(TempMdl = Tsdu, uOffset=0; TempMdl != NULL; TempMdl = TempMdl->Next)
		{
			uDataLength = MmGetMdlByteCount( TempMdl );
			pMdl = (PUCHAR)MmGetSystemAddressForMdlSafe( TempMdl, NormalPagePriority );
			if (uOffset == 0)
			{
				RtlCopyMemory(pData + uOffset, pMdl+StartingOffset, uDataLength-StartingOffset);
			}
			else
			{
				RtlCopyMemory(pData + uOffset, pMdl, uDataLength-StartingOffset);
			}
			
			uOffset += uDataLength;
		}

		Length = ReceiveLength;

		if (!IsListEmpty(&pEventContext->ReceiveList))
		{//合并数据
			pNewData = ExAllocatePoolWithTag(NonPagedPool, Length + pEventContext->ReceiveLength, 'xxxx');
			for(pTemp = pEventContext->ReceiveList.Flink;pTemp != &pEventContext->ReceiveList; pTemp = pTemp->Flink)
			{
				pPacket = CONTAINING_RECORD(
					pTemp,
					Chained_Receive_Packet,
					node
					);
				RtlCopyMemory(pNewData+uNewDataOffset, pPacket->buffer, pPacket->length);
				uNewDataOffset += pPacket->length;
			}

			RtlCopyMemory(pNewData+uNewDataOffset, pData, Length);
			uNewDataOffset+=Length;
		}
		else
		{
		}

		Length = (uNewDataOffset>Length?uNewDataOffset:Length);

		uStatus = AppProtocolAnalyze(
			FALSE,
			(pNewData==NULL?pData:pNewData), 
			&Length, 
			pEventContext->FileObject,
			NULL
			);

		if (!FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
		{
			break;
		}

		if (FLAG_ON_UINT(uStatus, SPLIT_PACKET))
		{
			//将本次的包缓存
			PChained_Receive_Packet pPacket = NULL;
			do 
			{
				pPacket = ExAllocatePoolWithTag(NonPagedPool, sizeof(Chained_Receive_Packet), 'xxxx');
			} while (pPacket == NULL);
			InitializeListHead(&pPacket->node);
			pPacket->Para.TdiEventContext = pEventContext->OldContext;
			pPacket->Para.ConnectionContext = ConnectionContext;
			pPacket->Para.ReceiveFlags = ReceiveFlags;
			pPacket->Para.ReceiveLength = ReceiveLength;
			pPacket->Para.StartingOffset = StartingOffset;
			pPacket->Para.Tsdu = Tsdu;
			pPacket->Para.TsduDescriptor = TsduDescriptor;

			pPacket->buffer = pData;
			pPacket->length = ReceiveLength;

			pEventContext->ReceiveLength += ReceiveLength;

			InsertTailList(&pEventContext->ReceiveList, &pPacket->node);

			status = STATUS_PENDING;
			break;
		}

		//协议已经可以正常分析数据了，清除已经缓存的数据包，只要没有SPLIT_PACKET，就处理
		if (!IsListEmpty(&pEventContext->ReceiveList))
		{//到这里，说明pNewData不为空，修改原数据包并提交给上层
			for(uNewDataOffset=0 ;!IsListEmpty(&pEventContext->ReceiveList);)
			{
				pTemp = RemoveHeadList(&pEventContext->ReceiveList);
				pPacket = CONTAINING_RECORD(
					pTemp,
					Chained_Receive_Packet,
					node
					);

				
				pMdl =  (PUCHAR)MmGetSystemAddressForMdlSafe( pPacket->Para.Tsdu, NormalPagePriority );
				ASSERT(pPacket->Para.Tsdu->Next == NULL);//做测试，假设只有一段

				RtlCopyMemory(pMdl+pPacket->Para.StartingOffset, pNewData+uNewDataOffset, pPacket->length);
				uNewDataOffset += pPacket->length;

				status = ((PTDI_IND_CHAINED_RECEIVE)(pEventContext->OldHandler))(pPacket->Para.TdiEventContext, 
					pPacket->Para.ConnectionContext, 
					pPacket->Para.ReceiveFlags, 
					pPacket->Para.ReceiveLength, 
					pPacket->Para.StartingOffset, 
					pPacket->Para.Tsdu,
					pPacket->Para.TsduDescriptor );

				ExFreePoolWithTag(pPacket->buffer, 'xxxx');
				ExFreePoolWithTag(pPacket, 'xxxx');
			}

			

			//最后将当前的数据也发送出去
			pMdl =  (PUCHAR)MmGetSystemAddressForMdlSafe( Tsdu, NormalPagePriority );
			ASSERT(Tsdu->Next == NULL);//做测试，假设只有一段
			RtlCopyMemory(pMdl+StartingOffset, pNewData+uNewDataOffset, ReceiveLength);
			status = ((PTDI_IND_CHAINED_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
				ConnectionContext, ReceiveFlags, ReceiveLength, StartingOffset, Tsdu, TsduDescriptor );

			//最后释放合并数据
			ExFreePoolWithTag(pNewData, 'xxxx');
			break;
		}


		//下面是没有缓存的数据修改，例如文件的512之后的数据

		//myMdl = IoAllocateMdl(
		//	pData,
		//	Length,
		//	FALSE,
		//	FALSE,
		//	NULL
		//	);
		//MmBuildMdlForNonPagedPool(myMdl);

		pMdl = (PUCHAR)MmGetSystemAddressForMdlSafe( Tsdu, NormalPagePriority );
		RtlCopyMemory(pMdl+StartingOffset, pData, Length);

		status = ((PTDI_IND_CHAINED_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
			ConnectionContext, ReceiveFlags, Length, StartingOffset, Tsdu, TsduDescriptor );
		if (status == STATUS_SUCCESS)
		{
			//ExFreePoolWithTag(pData, 'xxxx');
			//IoFreeMdl(myMdl);
		}
		else if (status == STATUS_PENDING)
		{
			//KdPrint(("STATUS_PENDING\n"));
		}
		else if (status == STATUS_DATA_NOT_ACCEPTED)
		{
			//KdPrint(("STATUS_DATA_NOT_ACCEPTED\n"));
		}

	} while (FALSE);

	if (!FLAG_ON_UINT(uStatus, CHANGE_PACKET_DATA))
	{
		status = ((PTDI_IND_CHAINED_RECEIVE)(pEventContext->OldHandler))( pEventContext->OldContext, 
			ConnectionContext, ReceiveFlags, ReceiveLength, StartingOffset, Tsdu, TsduDescriptor );
	}

	if (pData != NULL &&
		!FLAG_ON_UINT(uStatus, SPLIT_PACKET)
		) //缓存后不用释放pData
	{
		ExFreePoolWithTag(pData, 'xxxx');
	}

    return status;
}
