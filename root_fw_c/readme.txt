服务端，多个链接上下文会绑定到一个地址文件。
客户端，一个链接上下文只关联一个地址文件。
建议结构体只有链接上下文保存指向地址文件结构的指针。而地址文件不能保存链接上下文的指针。

关于数据接收的问题从SeiEventHandler入手。


TDI_CONNECTION(出站 一个地址对象绑定一个连接上下文)
客户端连接到远程主机

TDI_DISCONNECTION
客户端断开与远程主机的连接

TDI_SET_EVENT_HANDLER
{
    TDI_EVENT_CONNECT(入站 多个连接上下文绑定一个地址对象)
    本机(Server)收来自远程计算机的连接

    TDI_EVENT_DISCONNECT
    服务端断开远程计算机的连接

    TDI_EVENT_RECEIVE
    接收数据的回调例程事件
}
