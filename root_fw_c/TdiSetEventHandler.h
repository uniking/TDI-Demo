#pragma once



//////////////////////////////////////////////////////////////////////////
// 名称: TdiDispSetEventHandler
// 说明: TDI_SET_EVENT_HANDLER 派遣例程
// 入参: DeviceObject 本层设备对象
//       Irp 请求的IRP对象指针
// 返回: NTSTATUS
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS TdiDispSetEventHandler( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );




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
    );



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
    );






//////////////////////////////////////////////////////////////////////////
// 名称: ReceiveEventHandler (TDI_EVENT_RECEIVE and TDI_EVENT_RECEIVE_EXPEDITED)
// 说明: 数据接收事件据操作函数  (面向连接)
// 入参: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 出参: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 返回: 详见 MSDN ClientEventReceive 或 ClientEventReceiveExpedited
// 备注: 如果不接收,则返回 STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ReceiveEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN ULONG  ReceiveFlags, 
    IN ULONG  BytesIndicated, 
    IN ULONG  BytesAvailable, 
    OUT ULONG  *BytesTaken, 
    IN PVOID  Tsdu, 
    OUT PIRP  *IoRequestPacket 
    );



//////////////////////////////////////////////////////////////////////////
// 名称: ChainedReceiveEventHandler (TDI_EVENT_CHAINED_RECEIVE and TDI_EVENT_CHAINED_RECEIVE_EXPEDITED)
// 说明: 链式数据接收操作函数 (面向连接)
// 入参: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 出参: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 返回: 详见 MSDN ClientEventChainedReceive 或 ClientEventChainedReceiveExpedited
// 备注: 拦截数据,则返回 STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ChainedReceiveEventHandler( IN PVOID  TdiEventContext, 
    IN CONNECTION_CONTEXT  ConnectionContext, 
    IN ULONG  ReceiveFlags, 
    IN ULONG  ReceiveLength, 
    IN ULONG  StartingOffset, 
    IN PMDL  Tsdu, 
    IN PVOID  TsduDescriptor 
    );



//////////////////////////////////////////////////////////////////////////
// 名称: ReceiveDatagramEventHandler (TDI_EVENT_RECEIVE_DATAGRAM)
// 说明: 数据流接收函数 (面向无连接)
// 入参: 详见 MSDN ClientEventReceiveDatagram
// 出参: 详见 MSDN ClientEventReceiveDatagram
// 返回: 详见 MSDN ClientEventReceiveDatagram
// 备注: 拦截数据,则返回 STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ReceiveDatagramEventHandler( IN PVOID  TdiEventContext, 
    IN LONG  SourceAddressLength, 
    IN PVOID  SourceAddress, 
    IN LONG  OptionsLength, 
    IN PVOID  Options, 
    IN ULONG  ReceiveDatagramFlags, 
    IN ULONG  BytesIndicated, 
    IN ULONG  BytesAvailable, 
    OUT ULONG  *BytesTaken, 
    IN PVOID  Tsdu, 
    OUT PIRP  *IoRequestPacket 
    );



//////////////////////////////////////////////////////////////////////////
// 名称: ChainedReceiveDatagramEventHandler (TDI_EVENT_CHAINED_RECEIVE_DATAGRAM)
// 说明: 链式数据流操作函数 (面向无连接)
// 入参: 详见 MSDN ClientEventChainedReceiveDatagram
// 出参: 详见 MSDN ClientEventChainedReceiveDatagram
// 返回: 详见 MSDN ClientEventChainedReceiveDatagram
// 备注: 拦截数据,则返回 STATUS_DATA_NOT_ACCEPTED
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
NTSTATUS ChainedReceiveDatagramEventHandler( IN PVOID  TdiEventContext, 
    IN LONG  SourceAddressLength, 
    IN PVOID  SourceAddress, 
    IN LONG  OptionsLength, 
    IN PVOID  Options, 
    IN ULONG  ReceiveDatagramFlags, 
    IN ULONG  ReceiveDatagramLength, 
    IN ULONG  StartingOffset, 
    IN PMDL  Tsdu, 
    IN PVOID  TsduDescriptor 
    );
