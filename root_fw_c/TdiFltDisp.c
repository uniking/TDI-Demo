#include "root_fw.h"
#include "TdiFltDisp.h"

extern TCPSendData_t *g_TCPSendData;
extern TCPSendDgData_t *g_TCPSendDgData;

//////////////////////////////////////////////////////////////////////////
// 名称: DispatchFilterIrp
// 说明: 处理TDI 普通请求 IRP_MJ_DEVICE_CONTROL
// 入参: DeviceObject 挂载到TDI的本层设备对象
//       Irp IRP请求指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchFilterIrp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( Irp );

    switch ( IrpSP->MajorFunction )
    {
        // 创建一个文件对象
    case IRP_MJ_CREATE:
        status = TdiCreate( DeviceObject, Irp, TdiDeviceExt );
        break;

    case IRP_MJ_CLOSE:
        status = TdiClose( DeviceObject, Irp, TdiDeviceExt );
        break;

        // 所有的文件对象的句柄已关闭(到了必须清理的流程)
    case IRP_MJ_CLEANUP:
        status = TdiCleanup( DeviceObject, Irp, TdiDeviceExt );
        break;

    case IRP_MJ_DEVICE_CONTROL:
        status = TdiMapUserRequest(DeviceObject, Irp, IrpSP);
        if ( NT_SUCCESS(status) )
        {
            status = OnDispatchInternal( DeviceObject, Irp );
        }
        else
        {
			//hook TcpSendData 
			PVOID buf;
			if (IrpSP->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER)
			{
				buf = IrpSP->Parameters.DeviceIoControl.Type3InputBuffer;
				status = TdiSendIrpWaitReturnRight(TdiDeviceExt->NextStackDevice, Irp);
				if (buf != NULL && status == STATUS_SUCCESS) 
				{
					g_TCPSendData = *(TCPSendData_t **)buf;
					*(TCPSendData_t **)buf = new_TCPSendData;
				}
				IoCompleteRequest (Irp, IO_NO_INCREMENT);
			}
			else if (IrpSP->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER)
			{
				buf = IrpSP->Parameters.DeviceIoControl.Type3InputBuffer;
				status = TdiSendIrpWaitReturnRight(TdiDeviceExt->NextStackDevice, Irp);
				if (buf != NULL && status == STATUS_SUCCESS) 
				{
					//g_TCPSendDgData = *(TCPSendDgData_t **)buf;
					//*(TCPSendDgData_t **)buf = new_TCPSendDgData;
				}
				IoCompleteRequest (Irp, IO_NO_INCREMENT);
			}
			else
			{
				// 其他的事情交给下层处理
				IoSkipCurrentIrpStackLocation( Irp );
				status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
			}
        }
        break;

    default:
        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
        break;
    }

    return status;
}



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
    )
{
    PFILE_FULL_EA_INFORMATION pNextEa = FileEaInfo;
    PFILE_FULL_EA_INFORMATION CurrentEaInfo = NULL;

    if ( NULL == pNextEa )
        return NULL;

    // 遍历整个EA信息结构
    while ( TRUE )
    {
        CurrentEaInfo = pNextEa;
        pNextEa += pNextEa->NextEntryOffset;    // 下一个EA信息入口偏移

        if ( CurrentEaInfo->EaNameLength == NameLength )
        {
            // 找到EA则退出循环
            if ( RtlCompareMemory(CurrentEaInfo->EaName, EaName, NameLength) == NameLength )
                break;
        }

        // 没有信息了
        if ( !CurrentEaInfo->NextEntryOffset )
            return NULL;
    }

    return CurrentEaInfo;
}




//////////////////////////////////////////////////////////////////////////
// 名称: EventCompleteRoutine
// 说明: 通知事件的完成例程(通知IRP下发已经处理完成)
// 入参: DeviceObject 设备对象
//       Irp IRP请求指针
//       Context 设置的完成例程请求参数(TDI_DEVICE_EXTENSION指针)
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS EventNotifyCompleteRoutine( IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp, 
    IN PVOID Context 
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DeviceObject);

    // 保存下层操作结果
    Irp->UserIosb->Information = Irp->IoStatus.Information;
    Irp->UserIosb->Status = Irp->IoStatus.Status;

    KeSetEvent( Irp->UserEvent, IO_NETWORK_INCREMENT, FALSE );

    // 不再往上回溯,自己分配的irp必须返回这个
    return STATUS_MORE_PROCESSING_REQUIRED;
}