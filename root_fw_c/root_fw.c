#include "root_fw.h"
#include "TdiFltDisp.h"
#include "TdiSetEventHandler.h"


// 防火墙与R3交互设备对象
PDEVICE_OBJECT g_cmdhlpDevice = NULL;
extern BOOLEAN g_ProtocolThreadStart;
extern LIST_ENTRY g_DataList;
extern KSPIN_LOCK  g_DataListSpinLock;
extern LIST_ENTRY g_SmbCreateList;
extern KMUTEX g_SMBKMutex;
extern KMUTEX g_HttpKMutex;

// TDI派遣函数表
TDI_REQUEST_TABLE TdiRequestTable[] = {
    { TDI_ASSOCIATE_ADDRESS, TdiDispAssociateAddress },             // (0x01)
    { TDI_DISASSOCIATE_ADDRESS, TdiDispDisassociateAddress },       // (0x02)
    { TDI_CONNECT, TdiDispConnect },        // (0x03)
    { TDI_LISTEN, NULL },                   // (0x04)
    { TDI_ACCEPT, NULL },                   // (0x05)
    { TDI_DISCONNECT, TdiDispDisConnect },  // (0x06)
    { TDI_SEND, TdiDispSend },              // (0x07)
    { TDI_RECEIVE, NULL },                  // (0x08)
    { TDI_SEND_DATAGRAM, TdiDispSendDatagram },                     // (0x09)
    { TDI_RECEIVE_DATAGRAM, NULL },         // (0x0A)
    { TDI_SET_EVENT_HANDLER, TdiDispSetEventHandler },              // (0x0B)
    { TDI_QUERY_INFORMATION, NULL },        // (0x0C)
    { TDI_SET_INFORMATION, NULL },          // (0x0D)
    { TDI_ACTION, NULL },                   // (0x0E)

    { TDI_DIRECT_SEND, NULL },              // (0x27)
    { TDI_DIRECT_SEND_DATAGRAM, NULL },     // (0x29)
    { TDI_DIRECT_ACCEPT, NULL }             // (0x2A)
};



//////////////////////////////////////////////////////////////////////////
// 名称: DriverEntry
// 说明: 驱动入口点
// 入参: 标准参数
// 出参: 标准参数
// 返回: 标准返回值
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT DriverObject, 
    IN PUNICODE_STRING RegistryPath 
    )
{
    ULONG nIndex;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
	HANDLE  ThreadHandle;

#ifndef DBG
    DriverObject->DriverUnload = NULL;
#else
    DriverObject->DriverUnload = DriverUnload;
#endif

    // 使用默认的派遣函数
    for ( nIndex = 0; nIndex < IRP_MJ_MAXIMUM_FUNCTION; ++nIndex )
    {
        DriverObject->MajorFunction[nIndex] = OnDispatchRoutine;
    }

    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = OnDispatchInternal;

    // 创建设备对象
    status = CreateFlthlpDevice( DriverObject );
    if ( !NT_SUCCESS(status) )
    {
        return status;
    }

    // 挂载TDI网络对象IPv4
    status = AattachIPv4Filters( DriverObject );
    if ( !NT_SUCCESS(status) )
    {
        DriverUnload( DriverObject );
        return status;
    }

    // 初始化对象链表管理
    InitializeListManager();
	KeInitializeMutex(&g_SMBKMutex, FALSE);
	KeInitializeMutex(&g_HttpKMutex, FALSE);

	//启动协议处理线程
	g_ProtocolThreadStart = TRUE;
	InitializeListHead(&g_DataList);
	InitializeListHead(&g_SmbCreateList);
	KeInitializeSpinLock(&g_DataListSpinLock);
	//PsCreateSystemThread(
	//&ThreadHandle,
	//THREAD_ALL_ACCESS,
	//NULL,
	//NULL,
	//NULL,
	//ProtocolThread, //新线程的运行地址
	//NULL
	//);

    DbgMsg(__FILE__, __LINE__, "root_fw DriverEntry success.\n");
    return status;
}


//////////////////////////////////////////////////////////////////////////
// 名称: OnUnload
// 说明: 设备卸载例程
// 备注: 通知TDIBase反初始化
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID 
DriverUnload( IN PDRIVER_OBJECT DriverObject )
{
    DbgMsg(__FILE__, __LINE__, "root_fw DriverUnload.\n");

    // 卸载IPv4协议集的挂载操作
    DetachIPv4Filters();

    // 删除对象链表管理
    UninitializeListManager();

    if ( NULL != g_cmdhlpDevice )
    {
        // 删除防火墙设备
        UNICODE_STRING uniSymblicLinkName;
        RtlInitUnicodeString( &uniSymblicLinkName, SYMBLIC_LINK_NAME );

        IoDeleteSymbolicLink( &uniSymblicLinkName );

        IoDeleteDevice( g_cmdhlpDevice );
    }
}


//////////////////////////////////////////////////////////////////////////
// 名称: Dispatch 除了IRP_MJ_INTERNAL_DEVICE_CONTROL的所有IOCTL请求
// 说明: 驱动派遣例程
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS 
OnDispatchRoutine( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // 防火墙IRP请求
    if ( DeviceObject == g_cmdhlpDevice )
    {
        status = DispatchCmdhlpIrp( DeviceObject, Irp );
    }
    else if ( DeviceObject == g_tcpFltDevice || 
              DeviceObject == g_udpFltDevice || 
              DeviceObject == g_rawIpFltDevice || 
              DeviceObject == g_ipFltDevice )
    {
        // IPv4 设备集请求
        status = DispatchFilterIrp( DeviceObject, Irp );
    }
    else
    {
        DbgMsg(__FILE__, __LINE__, "Warning: OnDeviceControl unknown DeviceObject: %08X.\n", DeviceObject);
        status = IRPUnFinish( Irp );
    }

    return status;
}


//////////////////////////////////////////////////////////////////////////
// 名称: DispatchInternal
// 说明: IRP_MJ_INTERNAL_DEVICE_CONTROL派遣例程
// 备注: 该请求只有挂载在TDI设备上的才有发送该IRP
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS 
OnDispatchInternal( 
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    int nIndex;
    PTDI_DEVICE_EXTENSION DevExt;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpSP;

    IrpSP = IoGetCurrentIrpStackLocation( Irp );

    if ( DeviceObject == g_ipFltDevice || 
         DeviceObject == g_tcpFltDevice || 
         DeviceObject == g_udpFltDevice || 
         DeviceObject == g_rawIpFltDevice )
    {
        DevExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

        // 根据派遣函数组调用数组中指定的回调例程
        for ( nIndex = 0; nIndex < _countof(TdiRequestTable); ++nIndex )
        {
            if ( TdiRequestTable[nIndex].RequestCode == IrpSP->MinorFunction )
            {
                PDRIVER_DISPATCH pDispatchFunc = TdiRequestTable[nIndex].DispatchFunc;

                // 有派遣函数例程
                if ( NULL != pDispatchFunc )
                {
                    // 调用对应的派遣函数
                    status = pDispatchFunc( DeviceObject, Irp );
                }
                else
                {
                    // 获取实际操作的设备对象
                    IoSkipCurrentIrpStackLocation( Irp );
                    status = IoCallDriver( DevExt->NextStackDevice, Irp );
                }

                break;
            }
        }
    }
    else
    {
        DbgMsg(__FILE__, __LINE__, "Warning: OnInternalDeviceCotrol unknown DeviceObject: %08X\n", DeviceObject);
        status = IRPUnFinish( Irp );        // 不能完成该请求
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: CreateFlthlpDevice
// 说明: 创建防火墙设备(该设备对象用于和R3通信)
// 入参: DriverObject Windows提供的驱动对象
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CreateFlthlpDevice( IN PDRIVER_OBJECT DriverObject )
{
    UNICODE_STRING uniDeviceName;
    UNICODE_STRING uniSymblicLinkName;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT DeviceObject;

    RtlInitUnicodeString( &uniDeviceName, DEVICE_NAME );
    status = IoCreateDevice( DriverObject, 
        0, 
        &uniDeviceName, 
        FILE_DEVICE_UNKNOWN, 
        0, 
        TRUE,       // 独占访问
        &DeviceObject 
        );

    if ( !NT_SUCCESS(status) )
        return status;

    // 创建语法连接名
    RtlInitUnicodeString( &uniSymblicLinkName, SYMBLIC_LINK_NAME );
    status = IoCreateSymbolicLink( &uniSymblicLinkName, &uniDeviceName );
    if ( !NT_SUCCESS(status) )
    {
        IoDeleteDevice( DeviceObject );
    }
    else 
    {
        g_cmdhlpDevice = DeviceObject;
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: DispatchCmdhlpIrp
// 说明: 处理R3对防火墙请求(该设备对象用于和R3通信)
// 入参: DeviceObject 防火墙通讯设备对象
//       Irp IRP请求指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchCmdhlpIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );

    switch ( IrpSP->MajorFunction )
    {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        status = IRPFinish( Irp );
        break;

        // IOCTL
    case IRP_MJ_DEVICE_CONTROL:
        break;

    default:
        status = IRPUnFinish( Irp );
        break;
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: IRPUnFinish
// 说明: 无法处理的IRP请求(status == STATUS_UNSUCCESSFUL)
// 入参: Irp 请求的IRP指针
// 返回: STATUS_UNSUCCESSFUL
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPUnFinish( IN PIRP Irp )
{
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return STATUS_UNSUCCESSFUL;
}



//////////////////////////////////////////////////////////////////////////
// 名称: IRPFinish
// 说明: 完成IRP请求(status == STATUS_SUCCESS)
// 入参: Irp 请求的IRP指针
// 返回: STATUS_SUCCESS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPFinish( IN PIRP Irp )
{
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NETWORK_INCREMENT );

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
// 名称: TdiSendIrpSynchronous
// 说明: 等待下层设备完成请求(同步)
// 入参: NextDevice 下层设备对象
//       Irp 需要操作的IRP指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiSendIrpSynchronous( IN PDEVICE_OBJECT NextDevice, IN PIRP Irp )
{
    KEVENT kEvent;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    // 设置事件通知完成例程
    KeInitializeEvent( &kEvent, NotificationEvent, FALSE );
    Irp->UserEvent = &kEvent;
    Irp->UserIosb = &iosb;

    status = IoCallDriver( NextDevice, Irp );

    // 等待事件的完成信号
    if ( status == STATUS_PENDING )
    {
        KeWaitForSingleObject( &kEvent, Executive, KernelMode, FALSE, NULL );
        status = iosb.Status;
    }

    return status;
}




NTSTATUS
TdiCompletionRoutine(
					 IN PDEVICE_OBJECT   DeviceObject,
					 IN PIRP             Irp,
					 IN PVOID            Context
					 )
{
	if (Irp->PendingReturned == TRUE) {
		KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
	}
	// This is the only status you can return.
	return STATUS_MORE_PROCESSING_REQUIRED;  
}

//等待Irp并再次获取Irp操作权限
NTSTATUS TdiSendIrpWaitReturnRight( IN PDEVICE_OBJECT NextDevice, IN PIRP Irp )
{
	KEVENT kEvent;
	NTSTATUS status;
	IO_STATUS_BLOCK iosb;

	// 设置事件通知完成例程
	KeInitializeEvent( &kEvent, NotificationEvent, FALSE );
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp,
		TdiCompletionRoutine,
		&kEvent,
		TRUE,
		TRUE,
		TRUE
		);
	status = IoCallDriver(NextDevice, Irp );
	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&kEvent,
			Executive, // WaitReason
			KernelMode, // must be Kernelmode to prevent the stack getting paged out
			FALSE,
			NULL // indefinite wait
			);
		status = Irp->IoStatus.Status;
	}

	return status;
}


