#include "stdafx.h"
#include "TdiSetEventHandler.h"


//////////////////////////////////////////////////////////////////////////
// 名称: ConnectEventHandler
// 说明: 连接事件操作函数(当本机作为服务器的时候,接收远程的连接)
// 入参: 详见 MSDN ConnectEventHandler
// 出参: 详见 MSDN ConnectEventHandler
// 返回: 详见 MSDN ConnectEventHandler
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ConnectEventHandler( IN PVOID  TdiEventContext, 
    IN LONG  RemoteAddressLength, 
    IN PVOID  RemoteAddress, 
    IN LONG  UserDataLength, 
    IN PVOID  UserData, 
    IN LONG  OptionsLength, 
    IN PVOID  Options, 
    OUT CONNECTION_CONTEXT  *ConnectionContext, 
    OUT PIRP  *AcceptIrp 
    )
{
    NTSTATUS status;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    // 调用原来的处理函数
    status = ((PTDI_IND_CONNECT)(pEventContext->OldHandler))( pEventContext->OldContext, 
        RemoteAddressLength, RemoteAddress, UserDataLength, UserData, OptionsLength, 
        Options, ConnectionContext, AcceptIrp );

    if ( MmIsAddressValid(*AcceptIrp) )
    {
        // 设置完成例程
        PIO_STACK_LOCATION IrpSP = IoGetCurrentIrpStackLocation( *AcceptIrp );
        PCONNECTION_ITEM ConnectItem = FindConnectItemByFileObj( IrpSP->FileObject );
        PADDRESS_ITEM AddressItem = FindAddressItemByFileObj( pEventContext->FileObject );

        // 入站状态
        ConnectItem->ConnectType = IN_SITE_STATE;

        // 检测长度
        if ( RemoteAddressLength > sizeof(ConnectItem->RemoteAddress) )
        {
            DbgMsg(__FILE__, __LINE__, "ConnectEventHandler RemoteAddressLength OverSize: %d.\n", RemoteAddressLength);
            return status;
        }

        // 关联地址对象
        ConnectItem->AddressItemPtr = AddressItem;
		AddressItem->ConnectItem = ConnectItem;

        // 保存远程地址
        RtlCopyMemory( ConnectItem->RemoteAddress, ((TRANSPORT_ADDRESS *)RemoteAddress)->Address, RemoteAddressLength );

        PrintIpAddressInfo( "ConnectEventHandler RemoteAddress", ((TRANSPORT_ADDRESS *)RemoteAddress)->Address );
    }

    return status;
}



//////////////////////////////////////////////////////////////////////////
// 名称: DisconnectEventHandler
// 说明: 断开连接事件操作函数 (当本机作为服务器的时候,断开远程主机的连接)
// 入参: 详见MSDN ClientEventDisconnect
// 返回: 详见MSDN ClientEventDisconnect
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS DisconnectEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN LONG  DisconnectDataLength, 
    IN PVOID  DisconnectData, 
    IN LONG  DisconnectInformationLength, 
    IN PVOID  DisconnectInformation, 
    IN ULONG  DisconnectFlags 
    )
{
    NTSTATUS status;
    PCONNECTION_ITEM ConnectItem;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    status = ((PTDI_IND_DISCONNECT)(pEventContext->OldHandler))( pEventContext->OldContext, 
        ConnectionContext, DisconnectDataLength, DisconnectData, DisconnectInformationLength, 
        DisconnectInformation,  DisconnectFlags );

    // 根据连接上下文找到连接对象结构体指针
    ConnectItem = FindConnectItemByConnectContext( ConnectionContext );

    if ( ConnectItem != NULL )
    {
        // 设置为无状态
        ConnectItem->ConnectType = UNKNOWN_STATE;

        // 取消与地址对象的绑定
        ConnectItem->AddressItemPtr = NULL;

        // 清空远程地址
        RtlZeroMemory( ConnectItem->RemoteAddress, sizeof(ConnectItem->RemoteAddress) );

		//删除应用程序上下文
		if (ConnectItem->AppContext.SendContext)
		{
			ExFreePoolWithTag(ConnectItem->AppContext.SendContext, 'ppa-');
		}

		if (ConnectItem->AppContext.ReceiveContext)
		{
			ExFreePoolWithTag(ConnectItem->AppContext.ReceiveContext, 'ppa-');
		}

		if (ConnectItem->AppContext.Context)
		{
			ExFreePoolWithTag(ConnectItem->AppContext.Context, 'ppa-');
		}

		//if (!IsListEmpty(&ConnectItem->ThreadDataList))
		//{
		//	PDATA_INFO pDataNode = NULL;
		//	PLIST_ENTRY pTemp = NULL;

		//	pTemp = RemoveHeadList(&ConnectItem->ThreadDataList);
		//	pDataNode = CONTAINING_RECORD(pTemp,DATA_INFO,node);
		//	ExFreePoolWithTag(pDataNode, 'xxxx');
		//}
    }

    //DbgMsg(__FILE__, __LINE__, "DisconnectEventHandler Result status: %08X.\n", status);
    return status;
}

