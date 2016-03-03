#include "TdiFltDisp.h"
#include "root_fw.h"



//////////////////////////////////////////////////////////////////////////
// 名称: TdiCreate
// 说明: TDI IRP_MJ_CREATE 派遣处理例程
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       TdiDeviceExt 挂载在TDI设备对象上的扩展
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiCreate( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt 
    )
{
    PFILE_FULL_EA_INFORMATION EaInfo;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    PAGED_CODE()

    EaInfo = (PFILE_FULL_EA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

    // 请求创建Address File
    if ( FindEaData( EaInfo, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH ) )
    {
        // 处理创建Address File请求
        status = DispatchTransportAddress( DeviceObject, Irp, TdiDeviceExt, IrpSP );
    }
    else if ( FindEaData( EaInfo, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH) )
    {
        // 处理创建连接上下文的请求
        CONNECTION_CONTEXT ConnectionContext = *(CONNECTION_CONTEXT *)(EaInfo->EaName + EaInfo->EaNameLength + 1);

        status = DispatchConnectionContext( DeviceObject, 
            Irp, 
            TdiDeviceExt, 
            IrpSP, 
            ConnectionContext 
            );
    }
    else
    {
        // 请求创建控制通道
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TransportAddressCompleteRoutine
// 说明: 生成传输地址请求完成例程 IRP_MJ_CREATE TransportAddress
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context 设置的完成例程请求参数(TDI_DEVICE_EXTENSION指针)
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TransportAddressCompleteRoutine( IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context 
    )
{
    NTSTATUS status;
    UCHAR LocalAddress[TA_ADDRESS_MAX];
    PADDRESS_ITEM AddressItem = (PADDRESS_ITEM)Context;

    // 传递Pending位
    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( AddressItem == NULL )
        return STATUS_SUCCESS;

    // 错误的设备对象
    if ( !NT_SUCCESS(Irp->IoStatus.Status) )
    {
        FreeAddressItem( AddressItem );
        return STATUS_SUCCESS;
    }

    // 查询地址信息
    status = TdiQueryAddressInfo( AddressItem->FileObject, sizeof(LocalAddress), LocalAddress );
    if ( !NT_SUCCESS(status) )
    {
        DbgMsg(__FILE__, __LINE__, "TdiQueryAddressInfo failed FileObject: %08X, Status: %08X.\n", 
            AddressItem->FileObject, status );

        FreeAddressItem( AddressItem );
        return STATUS_SUCCESS;
    }

    //PrintIpAddressInfo( "TransportAddressCompleteRoutine LocalAddress:", (PTA_ADDRESS)LocalAddress );

    // 保存地址信息
    RtlCopyMemory( &AddressItem->LocalAddress, LocalAddress, sizeof(AddressItem->LocalAddress) );

    // 插入到传输地址链表中
    InsertAddressItemToList( AddressItem );
    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// 名称: DispatchTransportAddress
// 说明: TDI 生成传输地址请求 IRP_MJ_CREATE TransportAddress
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       TdiDeviceExt 挂载在TDI设备对象上的扩展指针
//       IrpSP 当前IRP栈数据IO_STACK_LOCATION指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchTransportAddress( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt, 
    IN PIO_STACK_LOCATION IrpSP 
    )
{
    NTSTATUS status;
    ULONG ProtocolType;
    PADDRESS_ITEM pAddressItem;

    // 获取协议号
    status = GetProtocolType( DeviceObject, IrpSP->FileObject, &ProtocolType );

    if ( Irp->CurrentLocation > 1 && NT_SUCCESS(status) )
    {
        pAddressItem = MallocAddressItem();
        if ( pAddressItem != NULL )
        {
            pAddressItem->FileObject = IrpSP->FileObject;
            pAddressItem->ProtocolType = ProtocolType;

            // 保存当前进程的ID
            pAddressItem->ProcessID = PsGetCurrentProcessId();
        }

        // 设置完成例程
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, TransportAddressCompleteRoutine, pAddressItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else 
    {
        // 跳过本层设备栈
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TdiQueryAddressInfo
// 说明: 向下层设备对象查询关联的FileObject地址和端口信息
// 入参: FileObject 和地址信息关联的文件对象
//       AddressLength 地址缓冲区长度
//       pIPAddress IP地址信息
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiQueryAddressInfo( IN PFILE_OBJECT FileObject, 
    IN ULONG AddressLength, 
    OUT PUCHAR pIPAddress 
    )
{
    PIRP Irp = NULL;
    PMDL mdl = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PTDI_ADDRESS_INFO pAddrInfo = NULL;
    ULONG nBufSize = sizeof(TDI_ADDRESS_INFO) + AddressLength;

    // 从设备栈顶开始往下传递
    PDEVICE_OBJECT RelatedDevice = IoGetRelatedDeviceObject( FileObject );

    RtlZeroMemory( pIPAddress, AddressLength );

    __try 
    {
        // 分配IRP请求
        Irp = IoAllocateIrp( RelatedDevice->StackSize, FALSE );
        if ( Irp == NULL )
        {
            DbgMsg(__FILE__, __LINE__, "TdiQueryAddressInfo IoAllocateIrp == NULL, Insufficient Resources.\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        // 分配内存
        pAddrInfo = kmalloc( nBufSize );
        if ( pAddrInfo == NULL )
        {
            DbgMsg(__FILE__, __LINE__, "TdiQueryAddressInfo kmalloc == NULL, Insufficient Resources.\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            __leave;
        }

        RtlZeroMemory( pAddrInfo, nBufSize );

        // 创建内存描述表
        mdl = IoAllocateMdl( pAddrInfo, nBufSize, FALSE, FALSE, NULL );
        if ( mdl == NULL )
            __leave;

        MmProbeAndLockPages( mdl, KernelMode, IoModifyAccess );

        // 生成查询请求
        TdiBuildQueryInformation( Irp, RelatedDevice, FileObject, EventNotifyCompleteRoutine, 
            NULL, TDI_QUERY_ADDRESS_INFO, mdl );

        // 调用下层设备并等待它完成
        status = TdiSendIrpSynchronous( RelatedDevice, Irp );

        if ( NT_SUCCESS(status) )
        {
            PTA_ADDRESS pTAddress = pAddrInfo->Address.Address;

            if ( pTAddress->AddressLength > AddressLength )
            {
                DbgMsg(__FILE__, __LINE__, "Warning: TdiQueryAddressInfo AddressLength OverSize: %d.\n", pTAddress->AddressLength);
                __leave;
            }

            // 保存本地地址
            RtlCopyBytes( pIPAddress, pTAddress, AddressLength );
        }
    }
    __finally
    {
        if ( mdl )
        {
            MmUnlockPages( mdl );
            IoFreeMdl( mdl );
        }

        if ( pAddrInfo )
        {
            kfree( pAddrInfo );
        }

        if ( Irp )
        {
            IoFreeIrp( Irp );
        }
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: ConnectionContextCompleteRoutine
// 说明: 创建连接上下文址请求完成例程 IRP_MJ_CREATE ConnectionContext
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context 设置的完成例程请求参数(TDI_DEVICE_EXTENSION指针)
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ConnectionContextCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    PCONNECTION_ITEM pConnectionItem = (PCONNECTION_ITEM)Context;

    if ( Irp->PendingReturned )
        IoMarkIrpPending( Irp );

    if ( pConnectionItem == NULL )
        return STATUS_SUCCESS;

    // 插入到连接上下文链表中
    if ( NT_SUCCESS(Irp->IoStatus.Status) )
    {
        InsertConnectionItemToList( pConnectionItem );
    }
    else
    {
        FreeConnectionItem( pConnectionItem );
    }

    // 继续网上层迭代完成例程
    return STATUS_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
// 名称: DispatchConnectionContext
// 说明: TDI 生成链接上下文请求 IRP_MJ_CREATE ConnectionContext
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       TdiDeviceExt 挂载在TDI设备对象上的扩展指针
//       IrpSP 当前IRP栈数据IO_STACK_LOCATION指针
//       ConnectionContext 连接上下文指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchConnectionContext( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PTDI_DEVICE_EXTENSION TdiDeviceExt, 
    IN PIO_STACK_LOCATION IrpSP, 
    IN CONNECTION_CONTEXT ConnectionContext 
    )
{
    NTSTATUS status;
    PCONNECTION_ITEM pConnectionItem = NULL;

    if ( Irp->CurrentLocation > 1 )
    {
        // 木有找到则分配哈希项
        pConnectionItem = MallocConnectionItem();
        if ( NULL != pConnectionItem )
        {
            // 填充哈希项
            pConnectionItem->FileObject = IrpSP->FileObject;

            // 保存连接上下文指针
            pConnectionItem->ConnectionContext = ConnectionContext;
        }

        // 设置完成例程
        IoCopyCurrentIrpStackLocationToNext( Irp );
        IoSetCompletionRoutine( Irp, ConnectionContextCompleteRoutine, pConnectionItem, TRUE, TRUE, TRUE );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }
    else
    {
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    return status;
}




