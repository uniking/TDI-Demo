#pragma once

// 结构体定义头文件
#include <ntddk.h>


#define MAX_EVENT                   (TDI_EVENT_ERROR_EX + 1)

// 最大地址结构体
#define TDI_ADDRESS_MAX_LENGTH	    TDI_ADDRESS_LENGTH_OSI_TSAP
#define TA_ADDRESS_MAX			    (sizeof(TA_ADDRESS) - 1 + TDI_ADDRESS_MAX_LENGTH)
#define TDI_ADDRESS_INFO_MAX	    (sizeof(TDI_ADDRESS_INFO) - 1 + TDI_ADDRESS_MAX_LENGTH)



// TDI设备对象的设备扩展结构体
typedef struct _TDI_DEVICE_EXTENSION
{
    PFILE_OBJECT  FileObject;           // 文件对象
    PDEVICE_OBJECT  FltDeviceObject;    // 本层设备对象
    PDEVICE_OBJECT  NextStackDevice;    // 下层设备对象
} TDI_DEVICE_EXTENSION, *PTDI_DEVICE_EXTENSION;



// TDI请求处理派遣表结构体
typedef struct _TDI_REQUEST_TABLE
{
    UCHAR  RequestCode;                 // IRP IOCTL
    PDRIVER_DISPATCH  DispatchFunc;     // 派遣处理函数指针
} TDI_REQUEST_TABLE, *PTDI_REQUEST_TABLE;



// TDI事件请求列表
typedef struct _TDI_EVENT_DISPATCH_TABLE
{
    ULONG  EventType;                   // event type
    PVOID  EventHandler;                // 处理函数地址指针
} TDI_EVENT_DISPATCH_TABLE, *PTDI_EVENT_DISPATCH_TABLE;


// TDI 事件结构体
typedef struct _TDI_EVENT_CONTEXT
{
    PFILE_OBJECT  FileObject;   // 保存地址文件对象
    PVOID  OldHandler;          // 保存旧操作函数指针
    PVOID  OldContext;          // 保存旧操作函数参数上下文

	LIST_ENTRY ReceiveList; //存放要合并的数据包
	ULONG ReceiveLength; //缓存数据包的总长度
} TDI_EVENT_CONTEXT, *PTDI_EVENT_CONTEXT;


// 地址信息结构体
typedef struct _ADDRESS_ITEM
{
    LIST_ENTRY  ListEntry;                  // 地址信息结构体链表
    PFILE_OBJECT  FileObject;               // 文件指针(索引)

	//PFILE_OBJECT ConnectFileObject;	//关联文件对象;
	struct _CONNECTION_ITEM *ConnectItem;

    HANDLE  ProcessID;                      // 进程ID
    ULONG  ProtocolType;                    // 协议类型

    UCHAR  LocalAddress[TA_ADDRESS_MAX];    // 本地地址信息

    TDI_EVENT_CONTEXT  EventContext[MAX_EVENT]; // 事件处理的结构体上下文
} ADDRESS_ITEM, *PADDRESS_ITEM;

typedef struct _APP_DATA_BUFFER
{
	PUCHAR Buffer;					// TCP数据指针
	ULONG Offset;						////本层协议偏移量
	ULONG length;						//length本层协议长度，由协议分析修改，减小则数据包减小
	ULONG MaximumLength;		//MaximumLength TCP传递的长度，不可由协议分析改变
	//KSPIN_LOCK  SpinLock;
	//KMUTEX kMutex;
}APP_DATA_BUFFER, *PAPP_DATA_BUFFER;


//虚拟进程数据结构
typedef struct _APP_FICTION_PROCESS
{
	INT realPid;
	PVOID Context;
	PVOID ReceiveContext;//协议上下文，由具体协议分配，如http，smtp，存放本进程的全局变量
	PVOID SendContext;//协议上下文，由具体协议分配，如http，smtp，存放本进程的全局变量

	APP_DATA_BUFFER sendBuffer;
	APP_DATA_BUFFER receiveBuffer;

	ULONG SplitLength;//应用层数据的长度

}APP_FICTION_PROCESS, *PAPP_FICTION_PROCESS;




// 连接上下文对象指针
typedef struct _CONNECTION_ITEM
{
    LIST_ENTRY  ListEntry;                  // 连接上下文对象链表
    PFILE_OBJECT  FileObject;               // 文件指针(索引)
    CONNECTION_CONTEXT  ConnectionContext;  // 连接上下文指针(索引 事件处理函数使用)

    UCHAR  ConnectType;                     // 连接类型状态(入站 or 出站)
    UCHAR  RemoteAddress[TA_ADDRESS_MAX];   // 远程地址信息

    PADDRESS_ITEM  AddressItemPtr;          // 关联的地址信息结构体

	APP_FICTION_PROCESS AppContext;		//应用层协议使用的上下文

	//LIST_ENTRY ThreadDataList;				//DATA_INFO队列
} CONNECTION_ITEM, *PCONNECTION_ITEM;

typedef unsigned int        UINT;
typedef struct _DATA_INFO
{
	LIST_ENTRY node;
	PETHREAD ThreadId;
	BOOLEAN isSendData;
	USHORT RemoteIpPort;
	KEVENT	WaitEvent;//线程等待处理完毕才返回
	APP_FICTION_PROCESS	threadData;
	PCONNECTION_ITEM ConnectItem;
	UINT uStatus;//处理结果
}DATA_INFO, *PDATA_INFO;
