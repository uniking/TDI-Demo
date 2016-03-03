#include "stdafx.h"
#include "root_fw.h"



//////////////////////////////////////////////////////////////////////////
// 名称: DisConnectCompleteRoutine
// 说明: 断开与远程计算机连接的完成例程
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context PCONNECTION_ITEM指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DisConnectCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PADDRESS_ITEM AddressItem;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCONNECTION_ITEM ConnectItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
         TdiDeviceExt != NULL && 
         ConnectItem != NULL && 
         TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // 修改为无状态
        ConnectItem->ConnectType = UNKNOWN_STATE;

        // 清空远程计算机地址信息
        RtlZeroMemory( ConnectItem->RemoteAddress, sizeof(ConnectItem->RemoteAddress) );

		//删除应用程序上下文
		//if (ConnectItem->AppContext.SendContext)
		//{
		//	ExFreePoolWithTag(ConnectItem->AppContext.SendContext, 'ppa-');
		//}

		//if (ConnectItem->AppContext.ReceiveContext)
		//{
		//	ExFreePoolWithTag(ConnectItem->AppContext.ReceiveContext, 'ppa-');
		//}

		//if (ConnectItem->AppContext.Context)
		//{
		//	ExFreePoolWithTag(ConnectItem->AppContext.Context, 'ppa-');
		//}

		//if (!IsListEmpty(&ConnectItem->ThreadDataList))
		//{
		//	PDATA_INFO pDataNode = NULL;
		//	PLIST_ENTRY pTemp = NULL;

		//	pTemp = RemoveHeadList(&ConnectItem->ThreadDataList);
		//	pDataNode = CONTAINING_RECORD(pTemp,DATA_INFO,node);
		//	ExFreePoolWithTag(pDataNode, 'xxxx');
		//}
    }

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// 名称: ConnectCompletionRoutine
// 说明: 连接远程端口完成例程
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context PCONNECTION_ITEM指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ConnectCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PTDI_REQUEST_KERNEL_CONNECT param;
    PIO_STACK_LOCATION IrpSP;
    PTA_ADDRESS RemoteAddr;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCONNECTION_ITEM ConnectItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
         ConnectItem != NULL && 
         ConnectItem->AddressItemPtr != NULL && 
         TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // 获取参数信息
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        param = (PTDI_REQUEST_KERNEL_CONNECT)(&IrpSP->Parameters);

        if ( NULL == param )
        {
            //DbgMsg(__FILE__, __LINE__, "ConnectCompleteRoutine: NULL == IrpSP->Parameters.\n");
            return STATUS_SUCCESS;
        }

        RemoteAddr = ((PTRANSPORT_ADDRESS)(param->RequestConnectionInformation->RemoteAddress))->Address;

        // 检查地址长度
        if ( RemoteAddr->AddressLength > sizeof(ConnectItem->RemoteAddress) )
        {
            //DbgMsg(__FILE__, __LINE__, "ConnectCompleteRoutine RemoteAddressLength OverSize: %d.\n", RemoteAddr->AddressLength );
            return STATUS_SUCCESS;
        }

        // 出站状态
        ConnectItem->ConnectType = OUT_SITE_STATE;

        // 保存连接到远程计算机地址信息
        RtlCopyBytes( ConnectItem->RemoteAddress, RemoteAddr, RemoteAddr->AddressLength );

        //PrintIpAddressInfo( "ConnectCompleteRoutine Success RemoteIP:", RemoteAddr );
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispConnect
// 说明: TDI_CONNECT内部派遣例程
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 如果有拦截,就返回 STATUS_REMOTE_NOT_LISTENING
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispConnect(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status;
    PCONNECTION_ITEM ConnectItem;
    PTDI_DEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );
    PTDI_REQUEST_KERNEL_CONNECT param = (PTDI_REQUEST_KERNEL_CONNECT)(&IrpSP->Parameters);

    if ( Irp->CurrentLocation > 1 )
    {
		// 根据文件对象找到对应的连接上下文
		ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        if (ConnectItem &&
			param != NULL && 
			param->RequestConnectionInformation->RemoteAddressLength > 0 )
        {
            // 打印连接地址
            PTA_ADDRESS TaRequestRemoteAddr = ((PTRANSPORT_ADDRESS)(param->RequestConnectionInformation->RemoteAddress))->Address;

			RtlCopyBytes( ConnectItem->RemoteAddress, TaRequestRemoteAddr, TaRequestRemoteAddr->AddressLength );

            //PrintIpAddressInfo( "TdiDispConnect RequestConnection RequestRemoteIP", TaRequestRemoteAddr );
        }

        // 设置完成例程
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, ConnectCompleteRoutine, ConnectItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }
    else
    {
		{
			ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );
			if (ConnectItem)
			{
				if ( param != NULL && param->RequestConnectionInformation->RemoteAddressLength > 0 )
				{
					// 打印连接地址
					PTA_ADDRESS TaRequestRemoteAddr = ((PTRANSPORT_ADDRESS)(param->RequestConnectionInformation->RemoteAddress))->Address;

					RtlCopyBytes( ConnectItem->RemoteAddress, TaRequestRemoteAddr, TaRequestRemoteAddr->AddressLength );

					//PrintIpAddressInfo( "TdiDispConnect RequestConnection RequestRemoteIP", TaRequestRemoteAddr );
				}
			}
		}


        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispDisConnect
// 说明: TDI_DISCONNECT 断开连接
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisConnect( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSP;
    PCONNECTION_ITEM ConnectItem;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // 设置完成例程
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, DisConnectCompleteRoutine, ConnectItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}


;