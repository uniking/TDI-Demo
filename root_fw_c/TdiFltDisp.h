#pragma once

#include "stdafx.h"


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
    );



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
    );



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispConnect
// 说明: TDI_CONNECT内部派遣例程
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispConnect(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispDisConnect
// 说明: TDI_DISCONNECT 断开连接
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisConnect( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispAssociateAddress
// 说明: 连接上下文关联地址文件对象
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispAssociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispDisassociateAddress
// 说明: 取消连接上下文与地址文件对象的关联
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTAUTS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispDisassociateAddress( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSend
// 说明: TDI_SEND 发送面向连接的数据包
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSend( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSendDatagram
// 说明: TDI_SEND_DATAGRAM 发送面向无连接的数据包
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSendDatagram( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



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
    );


//////////////////////////////////////////////////////////////////////////
// 名称: DispatchFilterIrp
// 说明: 处理TDI 普通请求 IRP_MJ_DEVICE_CONTROL
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchFilterIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



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
    );



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
    );



//////////////////////////////////////////////////////////////////////////
// 名称: EventCompleteRoutine
// 说明: 通知事件的完成例程
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
//       Context 设置的完成例程请求参数(TDI_DEVICE_EXTENSION指针)
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
IO_COMPLETION_ROUTINE 
EventNotifyCompleteRoutine;



//////////////////////////////////////////////////////////////////////////
// 名称: FindEaData
// 说明: 验证FILE_FULL_EA_INFORMATION请求类型
// 入参: FileEaInfo FILE_FULL_EA_INFORMATION指针
//       EaName 用来查找和匹配的File EA名称
//       NameLength EaName的长度(匹配用)
// 返回: PFILE_FULL_EA_INFORMATION 查找到的指定File EA信息指针
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PFILE_FULL_EA_INFORMATION FindEaData( IN PFILE_FULL_EA_INFORMATION FileEaInfo, 
    IN PCHAR EaName, 
    IN ULONG NameLength 
    );



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
    );


NTSTATUS new_TCPSendData(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);
NTSTATUS new_TCPSendDgData(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

NTSTATUS
TdiCompletionRoutine(
					IN PDEVICE_OBJECT   DeviceObject,
					IN PIRP             Irp,
					IN PVOID            Context
					);

//协议处理线程
VOID ProtocolThread(
					PVOID StartContext
				 );

