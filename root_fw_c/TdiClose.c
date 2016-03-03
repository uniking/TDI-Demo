#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// 名称: CleanupConnectionFileRoutine
// 说明: TDI_CLEANUP 删除文件对象连接上下文文件完成例程
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context 设置的完成例程请求参数(TDI_DEVICE_EXTENSION指针)
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupConnectionFileRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    NTSTATUS status = Irp->IoStatus.Status;
    PCONNECTION_ITEM pConnectionItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    // 文件对象被清理了,那么就得从连接上下文链表中删除
    if ( NT_SUCCESS(status) && NULL != pConnectionItem )
    {
        DeleteConnectionItemFromList( pConnectionItem );
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// 名称: CleanupAddressFileRoutine
// 说明: TDI_CLEANUP 删除传输地址文件完成例程
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context 设置的完成例程请求参数(TDI_DEVICE_EXTENSION指针)
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupAddressFileRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    NTSTATUS status = Irp->IoStatus.Status;
    PADDRESS_ITEM pAddressItem = (PADDRESS_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    // 文件对象被清理了,那么就得从地址链表中的对象
    if ( NT_SUCCESS(status) && NULL != pAddressItem )
    {
        DeleteAddressItemFromList( pAddressItem );
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiClose
// 说明: TDI IRP_MJ_CLOSE 派遣处理例程
// 入参: DeviceObject 本层设备对象
//       Irp IRP请求指针
//       TdiDeviceExt 本层设备对象的扩展
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiClose( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt
    )
{
    PAGED_CODE()

    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}



//////////////////////////////////////////////////////////////////////////
// 名称: CleanupTransportAddressFile
// 说明: 清理传输地址文件
// 入参: DeviceObject 本层设备对象
//       Irp IRP请求指针
//       TdiDeviceExt 本层设备对象扩展指针
//       IrpSP 本层设备对象栈指针
// 出参: 
// 返回: 
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupTransportAddressFile( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt, 
    IN PIO_STACK_LOCATION IrpSP 
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PADDRESS_ITEM pAddressItem = FindAddressItemByFileObj( IrpSP->FileObject );

    if ( Irp->CurrentLocation > 1 && pAddressItem != NULL )
    {
        // 准备回调例程
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, CleanupAddressFileRoutine, pAddressItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: CleanupConnectionFile
// 说明: 清理连接上下文文件对象
// 入参: DeviceObject 本层设备对象
//       Irp IRP请求指针
//       TdiDeviceExt 本层设备对象扩展指针
//       IrpSP 本层设备对象栈指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CleanupConnectionFile( IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt,  
    IN PIO_STACK_LOCATION IrpSP 
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PCONNECTION_ITEM pConnectionItem = FindConnectItemByFileObj( IrpSP->FileObject );

    // 检查参数
    if ( Irp->CurrentLocation > 1 && pConnectionItem != NULL )
    {
        // 准备回调例程
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, CleanupConnectionFileRoutine, pConnectionItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiCleanup
// 说明: TDI IRP_MJ_CLEANUP 派遣处理例程
// 入参: DeviceObject 本层设备对象
//       Irp IRP请求指针
//       TdiDeviceExt 本层设备对象的扩展
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiCleanup( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE()

    // 检查设备栈与参数
    if ( Irp->CurrentLocation > 0 && IrpSP->FileObject->FsContext2 != NULL )
    {
        switch ( (ULONG)IrpSP->FileObject->FsContext2 )
        {
            // 关闭的是一个传送地址文件
        case TDI_TRANSPORT_ADDRESS_FILE:
            status = CleanupTransportAddressFile( DeviceObject, Irp, TdiDeviceExt, IrpSP );
            break;

            // 关闭的是一个连接上下文
        case TDI_CONNECTION_FILE:
            status = CleanupConnectionFile( DeviceObject, Irp, TdiDeviceExt, IrpSP );
            break;

        default:    // TDI_CONTROL_CHANNEL_FILE
            IoSkipCurrentIrpStackLocation( Irp );
            status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
            break;
        }
    }
    else
    {
        // 让下层设备栈处理
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



