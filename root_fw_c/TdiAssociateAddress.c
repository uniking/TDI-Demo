#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// 名称: AssociateAddressCompleteRoutine
// 说明: 关联地址请求完成例程(用来关联地址和连接上下文用)
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context 地址关联对象指针
// 返回: STATUS_SUCCESS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS AssociateAddressCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PCONNECTION_ITEM ConnectItem;
    PIO_STACK_LOCATION IrpSP;
    PADDRESS_ITEM AddressItem = (PADDRESS_ITEM)Context;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    // 文件对象被清理了,那么就得从连接上下文链表中删除
    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
        NULL != TdiDeviceExt && 
        NULL != AddressItem && 
        TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // 获取连接上下文结构
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // 关联对象地址和连接上下文
        if ( ConnectItem != NULL && AddressItem != NULL )
        {
            ConnectItem->AddressItemPtr = AddressItem;

			AddressItem->ConnectItem = ConnectItem;
        }
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// 名称: DisAssociateAddressCompleteRoutine
// 说明: 地址文件和连接上下文取消关联操作
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context 连接上下文指针
// 返回: STATUS_SUCCESS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DisAssociateAddressCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCONNECTION_ITEM ConnectionItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( NT_SUCCESS(Irp->IoStatus.Status) && 
         ConnectionItem != NULL && 
         TdiDeviceExt->FltDeviceObject == DeviceObject )
    {
        // 取消地址和连接上下文的关联
        if ( ConnectionItem != NULL )
        {
            ConnectionItem->AddressItemPtr = NULL;
        }
    }

    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispAssociateAddress
// 说明: 连接上下文关联地址文件对象
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispAssociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION IrpSP;
    PFILE_OBJECT FileObject;
    NTSTATUS status;
    PTDI_REQUEST_KERNEL_ASSOCIATE AssociateParam;
    PADDRESS_ITEM AddressItem = NULL;
    PTDI_DEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        AssociateParam = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSP->Parameters;

        status = ObReferenceObjectByHandle(AssociateParam->AddressHandle, 
            0, 
            *IoFileObjectType, 
            KernelMode, 
            (PVOID*)&FileObject, 
            NULL 
            );

        if ( NT_SUCCESS(status) )
        {
            // 从地址链表中查找
            AddressItem = FindAddressItemByFileObj( FileObject );
            ObDereferenceObject( FileObject );
        }

        // 设置完成例程
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, AssociateAddressCompleteRoutine, AddressItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispDisassociateAddress
// 说明: 取消连接上下文与地址文件对象的关联
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTAUTS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisassociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status;
    PCONNECTION_ITEM ConnectItem;
    PIO_STACK_LOCATION IrpSP;
    PTDI_DEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;

    if ( Irp->CurrentLocation > 1 )
    {
        IrpSP = IoGetCurrentIrpStackLocation( Irp );
        ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );

        // 在完成例程中取消地址和连接上下文的关联
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, DisAssociateAddressCompleteRoutine, ConnectItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( pDevExt->NextStackDevice, Irp );
    }

    return status;
}


