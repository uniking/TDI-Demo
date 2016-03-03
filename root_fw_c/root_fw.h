#pragma once

#include "stdafx.h"
#include "TdiFltDisp.h"



//////////////////////////////////////////////////////////////////////////
// 名称: CreateFlthlpDevice
// 说明: 创建防火墙设备(该设备对象用于和R3通信)
// 入参: DriverObject Windows提供的驱动对象
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS CreateFlthlpDevice( IN PDRIVER_OBJECT DriverObject );



//////////////////////////////////////////////////////////////////////////
// 名称: DispatchCmdhlpIrp
// 说明: 处理R3对防火墙请求(该设备对象用于和R3通信)
// 入参: DeviceObject 防火墙通讯设备对象
//       Irp IRP请求指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchCmdhlpIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: IRPUnFinish
// 说明: 无法处理的IRP请求(status == STATUS_UNSUCCESSFUL)
// 入参: Irp 请求的IRP指针
// 返回: STATUS_UNSUCCESSFUL
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPUnFinish( IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: IRPFinish
// 说明: 完成IRP请求(status == STATUS_SUCCESS)
// 入参: Irp 请求的IRP指针
// 返回: STATUS_SUCCESS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS IRPFinish( IN PIRP Irp );



//////////////////////////////////////////////////////////////////////////
// 名称: TdiSendIrpSynchronous
// 说明: 等待下层设备完成请求(同步)
// 入参: NextDevice 下层设备对象
//       Irp 需要操作的IRP指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiSendIrpSynchronous( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

NTSTATUS TdiSendIrpWaitReturnRight( IN PDEVICE_OBJECT NextDevice, IN PIRP Irp );

// 定义主要的驱动派遣函数
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH OnDispatchRoutine;

__drv_dispatchType(IRP_MJ_INTERNAL_DEVICE_CONTROL)
DRIVER_DISPATCH OnDispatchInternal;


typedef NTSTATUS  TCPSendData_t(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);


//static TCPSendData_t new_TCPSendData;

typedef NTSTATUS TCPSendDgData_t(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp);

//static TCPSendDgData_t new_TCPSendDgData;


#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, OnDispatchRoutine)
    #pragma alloc_text( PAGE, OnDispatchInternal)
    #pragma alloc_text( PAGE, DriverUnload)
#endif // ALLOC_PRAGMA


