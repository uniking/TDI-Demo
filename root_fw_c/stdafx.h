#pragma once


//////////////////////////////////////////////////////////////////////////
// Thanks to tdifw1.4.4
// Thanks to ReactOS 0.3.13
//////////////////////////////////////////////////////////////////////////


#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						


#include "VisualDDKHelpers.h"
#include <ntddk.h>
#include <ntddstor.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <tdi.h>
#include <tdiinfo.h>
#include <TdiKrnl.h>
#include <tdistat.h>
#include <ntstrsafe.h>

#include <stdlib.h>


// 结构体定义的头文件
#include "structs.h"

// 网络传输类型定义
#include "Sock.h"

// IPv4设备集
#include "fw_ioctl.h"
#include "IPv4ProtocolSet.h"
#include "ObjListManager.h"


// 定义未公开的函数(根据EPROCESS 获取进程名)
//UCHAR* PsGetProcessImageFileName( PEPROCESS Process );


// 内存操作宏定义
#define kmalloc(size)           ExAllocatePoolWithTag( NonPagedPool, size, 'root' )
#define kfree(ptr)              ExFreePoolWithTag( ptr, 'root' )


#define PASS_NET_WORK 0x01
#define FORBID_NET_WORK 0x02

#define STORAGE_DATA_BUFFER 0x04 //使用了传来的buffer，不能释放
#define RECEIVE_STORAGE_PACKET 0x08//将临时存储的包提交上层
#define SEND_STORAGE_PACKET 0x10 //将临时存储的包发出
#define CHANGE_PACKET_DATA 0x20 //修改数据包
#define RECEIVE_DISCARD_PACKET 0x40 //直接丢弃收到的数据包
#define DATA_IS_LESS 0x80 //指示现在的包太小，需要暂存这个包（当数据不足512,不能解密文件头）
#define RETURN_PACKET 0x100 //指示将现在的包返回
#define SPLIT_PACKET 0x200 //拆分本次的包为两个包，前一部分发送，后一部分保留下层到下次发送
//（http与文件混杂，文件长度不够，如果简单返回return_packet，下载在进入分析，状态就不对了）
#define DEBUG_TEST	0x4000

#define FLAG_ON_UINT(x, flag) ((UINT)x & (UINT)flag)
#define FLAG_SET_UINT(x, flag) (x = x | flag)

// 打印IP地址信息
void PrintIpAddressInfo( PCHAR pszMsg, PTA_ADDRESS pTaAddress );

void DbgMsg(char *lpszFile, int Line, char *lpszMsg, ...);
