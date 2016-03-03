#include "stdafx.h"
#include "TdiSetEventHandler.h"



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
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION IrpSP;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    // 调用原来的处理函数
    status = ((PTDI_IND_RECEIVE_DATAGRAM)(pEventContext->OldHandler))( pEventContext->OldContext, 
        SourceAddressLength, SourceAddress, OptionsLength, Options, ReceiveDatagramFlags, 
        BytesIndicated, BytesAvailable, BytesTaken, Tsdu, IoRequestPacket );

    //DbgMsg(__FILE__, __LINE__, "ReceiveDatagramEventHandler Status: %08X, BytesTaken: %u, ReceiveDatagramFlags: %d.\n", 
    //    status, *BytesTaken, ReceiveDatagramFlags);

    return status;
}



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
    )
{
    NTSTATUS status;
    PUCHAR pData = NULL;
    ULONG nDataSize;
    PTDI_EVENT_CONTEXT pEventContext = (PTDI_EVENT_CONTEXT)TdiEventContext;

    // 调用原来的接收函数
    status = ((PTDI_IND_CHAINED_RECEIVE_DATAGRAM)(pEventContext->OldHandler))( pEventContext->OldContext, 
        SourceAddressLength, SourceAddress, OptionsLength, Options, ReceiveDatagramFlags, 
        ReceiveDatagramLength, StartingOffset, Tsdu, TsduDescriptor );

    pData = (PUCHAR)MmGetSystemAddressForMdlSafe( Tsdu, NormalPagePriority );
    nDataSize = MmGetMdlByteCount( Tsdu );

    //DbgMsg(__FILE__, __LINE__, "ChainedReceiveDatagramEventHandler Status: %08X, ReceiveDatagramFlags: %d, DataSize: %u.\n", 
    //    status, ReceiveDatagramFlags, nDataSize);

    return status;
}

