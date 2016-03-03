#include "stdafx.h"
#include "IPv4ProtocolSet.h"


//////////////////////////////////////////////////////////////////////////
// IPv4过滤设备对象
PDEVICE_OBJECT g_tcpFltDevice = NULL;
PDEVICE_OBJECT g_udpFltDevice = NULL;
PDEVICE_OBJECT g_rawIpFltDevice = NULL;
PDEVICE_OBJECT g_ipFltDevice = NULL;
PDEVICE_OBJECT g_igmpFltDevice = NULL;


//////////////////////////////////////////////////////////////////////////
// 名称: AttachIPv4Filter
// 说明: 挂接IPv4设备
// 入参: DriverObject 驱动对象指针
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS AattachIPv4Filters( IN PDRIVER_OBJECT DriverObject )
{
    // TCP
    AttachFilter( DriverObject, &g_tcpFltDevice, DEVICE_TCP4_NAME );

    // UDP
    AttachFilter( DriverObject, &g_udpFltDevice, DEVICE_UDP4_NAME );

    // RawIp
    AttachFilter( DriverObject, &g_rawIpFltDevice, DEVICE_RAWIP4_NAME );

    // IP
    AttachFilter( DriverObject, &g_ipFltDevice, DEVICE_IPV4_NAME );

    // IGMP
    AttachFilter( DriverObject, &g_igmpFltDevice, DEVICE_IPMULTICAST4_NAME );

    if ( g_tcpFltDevice && g_udpFltDevice && 
         g_rawIpFltDevice && g_ipFltDevice && 
         g_igmpFltDevice )
    {
        return STATUS_SUCCESS;
    }

    DbgMsg(__FILE__, __LINE__, "Warning: Attach IPv4 failed.\n");
    return STATUS_UNSUCCESSFUL;
}


//////////////////////////////////////////////////////////////////////////
// 名称: DetachIPv4Filters
// 说明: 卸载IPv4设备
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachIPv4Filters()
{
    DetachFilter( g_ipFltDevice );
    DetachFilter( g_tcpFltDevice );
    DetachFilter( g_udpFltDevice );
    DetachFilter( g_rawIpFltDevice );
    DetachFilter( g_igmpFltDevice );
}


//////////////////////////////////////////////////////////////////////////
// 名称: AttachFilter
// 说明: 根据设备名创建并挂载设备对象
// 入参: DriverObject 驱动对象
//       fltDeviceObj 过滤设备对象
//       DeviceName 要挂接的目标设备名
// 返回: 状态值
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS AttachFilter( 
    IN PDRIVER_OBJECT DriverObject, 
    OUT PDEVICE_OBJECT* fltDeviceObject, 
    IN PWCHAR pszDeviceName 
    )
{
    UNICODE_STRING uniDeviceName;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pNextStackDevice;
    PTDI_DEVICE_EXTENSION DeviceExt;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // 获取设备对象
    RtlInitUnicodeString( &uniDeviceName, pszDeviceName );
    status = IoGetDeviceObjectPointer( &uniDeviceName, 
        FILE_ATTRIBUTE_NORMAL, 
        &pFileObject, 
        &pNextStackDevice 
        );

    if ( !NT_SUCCESS(status) )
    {
        DbgMsg(__FILE__, __LINE__, "IoGetDeviceObjectPointer failed DeviceName: %ws\n", pszDeviceName);
        return status;
    }

    // 创建过滤设备对象
    status = IoCreateDevice( DriverObject, 
        sizeof(TDI_DEVICE_EXTENSION), 
        NULL, 
        pNextStackDevice->DeviceType, 
        pNextStackDevice->Characteristics, 
        FALSE,          // 非独占设备
        fltDeviceObject 
        );

    if ( !NT_SUCCESS(status) )
    {
        DbgMsg(__FILE__, __LINE__, "IoCreateDevice failed DeviceName: %ws, Status: %08X\n", 
            pszDeviceName, status);

        ObDereferenceObject( pFileObject );
        return status;
    }

    // 使用设备直接读取标志位
    (*fltDeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;
    (*fltDeviceObject)->Flags |= (*fltDeviceObject)->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO);

    // 保存下层设备对象
    DeviceExt = (PTDI_DEVICE_EXTENSION)(*fltDeviceObject)->DeviceExtension;
    RtlZeroMemory( DeviceExt, sizeof(*DeviceExt) );

    // 将设备挂接到设备栈,并保存底层设备对象
    DeviceExt->NextStackDevice = IoAttachDeviceToDeviceStack( *fltDeviceObject, pNextStackDevice );
    DeviceExt->FileObject = pFileObject;
    DeviceExt->FltDeviceObject = *fltDeviceObject;

    DbgMsg(__FILE__, __LINE__, "Attach success: %ws\n", pszDeviceName);
    return status;
}


//////////////////////////////////////////////////////////////////////////
// 名称: DetachFilter
// 说明: 卸载并删除设备对象
// 入参: DeviceObject 要删除的设备对象
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachFilter( IN PDEVICE_OBJECT DeviceObject )
{
    PTDI_DEVICE_EXTENSION DeviceExt;

    if ( NULL == DeviceObject )
        return;

    DeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    if ( NULL == DeviceExt )
    {
        DbgMsg(__FILE__, __LINE__, "Warring: DetachFilter DeviceExtension == NULL.\n");
        return;
    }

    // 减少下层文件句柄的引用
    if ( NULL != DeviceExt->FileObject )
    {
        ObReferenceObject( DeviceExt->FileObject );
    }

    // 摘除挂接设备
    if ( NULL != DeviceExt->NextStackDevice )
    {
        IoDetachDevice( DeviceExt->NextStackDevice );
    }

    // 删除设备
    if ( NULL != DeviceExt->FltDeviceObject )
    {
        IoDeleteDevice( DeviceExt->FltDeviceObject );
    }
}



//////////////////////////////////////////////////////////////////////////
// 名称: GetProtocolType
// 说明: 根据当前的设备对象获取协议类型
// 入参: DeviceObject 当前请求的设备对象
//       FileObject IRP栈中的文件对象,用来对RawIp协议类型进行获取操作
// 出参: ProtocolType 协议类型
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS GetProtocolType( IN PDEVICE_OBJECT DeviceObject, 
    IN PFILE_OBJECT FileObject, 
    OUT PULONG ProtocolType 
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ( DeviceObject == g_tcpFltDevice )
        *ProtocolType = IPPROTO_TCP;
    else if ( DeviceObject == g_udpFltDevice )
        *ProtocolType = IPPROTO_UDP;
    else if ( DeviceObject == g_rawIpFltDevice )
    {
        // 获取原始接口协议号
        status = TiGetProtocolNumber( &FileObject->FileName, ProtocolType );
        if ( !NT_SUCCESS(status) )
        {
            DbgMsg(__FILE__, __LINE__, "TiGetProtocolNumber Raw IP protocol number is invalid.\n");
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else if ( DeviceObject == g_ipFltDevice )
        *ProtocolType = IPPROTO_IP;
    else if ( DeviceObject == g_igmpFltDevice )
        *ProtocolType = IPPROTO_IGMP;
    else
        status = STATUS_INVALID_PARAMETER;

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: TiGetProtocolNumber
// 说明: 根据文件名返回协议号
// 入参: FileName 文件对象名
// 出参: Protocol 返回协议号
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TiGetProtocolNumber( IN PUNICODE_STRING FileName, OUT PULONG Protocol )
{
    UNICODE_STRING us;
    NTSTATUS Status;
    ULONG Value;
    PWSTR Name;

    Name = FileName->Buffer;

    if ( *Name++ != (WCHAR)L'\\' )
        return STATUS_UNSUCCESSFUL;

    if ( *Name == L'\0' )
        return STATUS_UNSUCCESSFUL;

    RtlInitUnicodeString(&us, Name);

    Status = RtlUnicodeStringToInteger(&us, 10, &Value);
    if ( !NT_SUCCESS(Status) || ((Value > IPPROTO_RAW)) )
        return STATUS_UNSUCCESSFUL;

    *Protocol = Value;
    return STATUS_SUCCESS;
}



