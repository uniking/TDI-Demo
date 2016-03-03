#include "stdafx.h"
#include "TdiSetEventHandler.h"


// 事件操作结构体定义
TDI_EVENT_DISPATCH_TABLE TdiEventDispatchTable[] = {
    { TDI_EVENT_CONNECT, ConnectEventHandler }, 
    { TDI_EVENT_DISCONNECT, DisconnectEventHandler }, 
    { TDI_EVENT_ERROR, NULL }, 
    { TDI_EVENT_RECEIVE, ReceiveEventHandler }, 
    { TDI_EVENT_RECEIVE_DATAGRAM, ReceiveDatagramEventHandler }, 
    { TDI_EVENT_RECEIVE_EXPEDITED, ReceiveEventHandler }, 
    { TDI_EVENT_SEND_POSSIBLE, NULL }, 
    { TDI_EVENT_CHAINED_RECEIVE, ChainedReceiveEventHandler }, 
    { TDI_EVENT_CHAINED_RECEIVE_DATAGRAM, ChainedReceiveDatagramEventHandler }, 
    { TDI_EVENT_CHAINED_RECEIVE_EXPEDITED, ChainedReceiveEventHandler }, 
    { TDI_EVENT_ERROR_EX, NULL }
};



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSetEventHandler
// 说明: TDI_SET_EVENT_HANDLER 派遣例程
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSetEventHandler( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    ULONG nIndex;
    PIO_STACK_LOCATION IrpSP;
    PADDRESS_ITEM AddressItem;
    PTDI_REQUEST_KERNEL_SET_EVENT param;
    PTDI_DEVICE_EXTENSION TdiDeviceExt = (PTDI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    IrpSP = IoGetCurrentIrpStackLocation( Irp );
    param = (PTDI_REQUEST_KERNEL_SET_EVENT)(&IrpSP->Parameters);

    // 保存处理函数信息
    AddressItem = FindAddressItemByFileObj( IrpSP->FileObject );

    // 没有操作函数,或者超过了事件定义的范围则跳过操作
    if ( param->EventHandler == NULL || 
         param->EventType < 0 || 
         param->EventType >= MAX_EVENT || 
         AddressItem == NULL )
    {
        IoSkipCurrentIrpStackLocation( Irp );
        return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
    }

    for ( nIndex = 0; nIndex < _countof(TdiEventDispatchTable); ++nIndex )
    {
        if ( TdiEventDispatchTable[nIndex].EventType != param->EventType )
            continue;

        if ( TdiEventDispatchTable[nIndex].EventHandler != NULL )
        {
            PTDI_EVENT_CONTEXT pEventContext = &AddressItem->EventContext[nIndex];

            // 保存旧的回调操作信息
            pEventContext->FileObject = IrpSP->FileObject;
            pEventContext->OldHandler = param->EventHandler;
            pEventContext->OldContext = param->EventContext;

			InitializeListHead(&pEventContext->ReceiveList);
			pEventContext->ReceiveLength = 0;

            // 设置新的回调处理函数和参数
            param->EventHandler = TdiEventDispatchTable[nIndex].EventHandler;
            param->EventContext = pEventContext;
        }

        break;
    }

    IoSkipCurrentIrpStackLocation( Irp );
    return IoCallDriver( TdiDeviceExt->NextStackDevice, Irp );
}




