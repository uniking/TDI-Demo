#pragma once


//////////////////////////////////////////////////////////////////////////
// IPv4 设备名称
// 定义IPv4设备名称
// ICMP
#define DEVICE_IPV4_NAME            L"\\Device\\Ip"
// Stream
#define DEVICE_TCP4_NAME            L"\\Device\\Tcp"
// Datagram
#define DEVICE_UDP4_NAME            L"\\Device\\Udp"
// Raw
#define DEVICE_RAWIP4_NAME          L"\\Device\\RawIp"
// IGMP
#define DEVICE_IPMULTICAST4_NAME    L"\\Device\\IPMULTICAST"


extern PDEVICE_OBJECT g_tcpFltDevice;
extern PDEVICE_OBJECT g_udpFltDevice;
extern PDEVICE_OBJECT g_rawIpFltDevice;
extern PDEVICE_OBJECT g_ipFltDevice;
extern PDEVICE_OBJECT g_igmpFltDevice;



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
    );


//////////////////////////////////////////////////////////////////////////
// 名称: DetachFilter
// 说明: 卸载并删除设备对象
// 入参: DeviceObject 要删除的设备对象
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachFilter( IN PDEVICE_OBJECT DeviceObject );


// 挂载TDI网络对象IPv4
NTSTATUS AattachIPv4Filters( IN PDRIVER_OBJECT DriverObject );


//////////////////////////////////////////////////////////////////////////
// 名称: DetachIPv4Filters
// 说明: 卸载IPv4设备
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
void DetachIPv4Filters();



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
    );



//////////////////////////////////////////////////////////////////////////
// 名称: GetRawProtocolType
// 说明: RawIp的协议类型获取函数
// 入参: FileObject 对应的文件对象指针
// 返回: 协议类型
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
UCHAR GetRawProtocolType( IN PFILE_OBJECT FileObject );



//////////////////////////////////////////////////////////////////////////
// 名称: TiGetProtocolNumber
// 说明: 根据文件名返回协议号
// 入参: FileName 文件对象名
// 出参: Protocol 返回协议号
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TiGetProtocolNumber( IN PUNICODE_STRING FileName, OUT PULONG Protocol );



