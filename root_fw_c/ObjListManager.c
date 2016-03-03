#include "stdafx.h"
#include "ObjListManager.h"


#define ADDRESSFILE_TAG             'file'
#define CONNECTIONENDPOINT_TAG      'conn'



// 传输地址文件对象管理
LIST_ENTRY g_AddressFileListHead;
KSPIN_LOCK g_AddressFileListLock;

// 连接上下文文件对象管理
LIST_ENTRY g_ConnectionEndpointListHead;
KSPIN_LOCK g_ConnectionEndpointListLock;


KIRQL g_AddressFileOldIrql;
KIRQL g_ConnectionEndpointOldIrql;

// 内存池
NPAGED_LOOKASIDE_LIST g_AddressFileListPool;
NPAGED_LOOKASIDE_LIST g_ConnectionEndpointListPool;


// 同步函数的定义
#define AddressFileList_Lock()  \
    KeAcquireSpinLock( &g_AddressFileListLock, &g_AddressFileOldIrql )

#define AddressFileList_Unlock()    \
    KeReleaseSpinLock( &g_AddressFileListLock, g_AddressFileOldIrql )

#define ConnectionEndpointList_Lock()   \
    KeAcquireSpinLock( &g_ConnectionEndpointListLock, &g_ConnectionEndpointOldIrql )

#define ConnectionEndpointList_Unlock() \
    KeReleaseSpinLock( &g_ConnectionEndpointListLock, g_ConnectionEndpointOldIrql )



//////////////////////////////////////////////////////////////////////////
// 名称: InitializeListManager
// 说明: 初始化链表
// 返回: 初始化结构,返回TRUE表示初始化完成
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID InitializeListManager()
{
    InitializeListHead( &g_AddressFileListHead );
    KeInitializeSpinLock( &g_AddressFileListLock );

    InitializeListHead( &g_ConnectionEndpointListHead );
    KeInitializeSpinLock( &g_ConnectionEndpointListLock );

    // 初始化内存池链表
    ExInitializeNPagedLookasideList( &g_AddressFileListPool, NULL, NULL, 0,
        sizeof(ADDRESS_ITEM), ADDRESSFILE_TAG, 0 );

    ExInitializeNPagedLookasideList( &g_ConnectionEndpointListPool, NULL, NULL,
        0, sizeof(CONNECTION_ITEM), CONNECTIONENDPOINT_TAG, 0 );
}



//////////////////////////////////////////////////////////////////////////
// 名称: UninitializeListManager
// 说明: 删除所有链表对象
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID UninitializeListManager()
{
    PLIST_ENTRY pListEntry;
    PADDRESS_ITEM pAddressItem;
    PCONNECTION_ITEM pConnectionItem;

    // 清空传输地址链表
    AddressFileList_Lock();
    while ( !IsListEmpty(&g_AddressFileListHead) )
    {
        pListEntry = RemoveTailList( &g_AddressFileListHead );
        pAddressItem = CONTAINING_RECORD( pListEntry, ADDRESS_ITEM, ListEntry );
        FreeAddressItem( pAddressItem );
    }
    ExDeleteNPagedLookasideList( &g_AddressFileListPool );
    AddressFileList_Unlock();

    // 清空连接上下文链表
    ConnectionEndpointList_Lock();
    while ( !IsListEmpty(&g_ConnectionEndpointListHead) )
    {
        pListEntry = RemoveTailList( &g_ConnectionEndpointListHead );
        pConnectionItem = CONTAINING_RECORD( pListEntry, CONNECTION_ITEM, ListEntry );
        FreeConnectionItem( pConnectionItem );
    }
    ExDeleteNPagedLookasideList( &g_ConnectionEndpointListPool );
    ConnectionEndpointList_Unlock();

    // 移除链表头部
    if ( RemoveHeadList( &g_AddressFileListHead ) )
    {
        DbgMsg(__FILE__, __LINE__, "Remove AddressFileList Success.\n");
    }

    if ( RemoveHeadList( &g_ConnectionEndpointListHead ) )
    {
        DbgMsg(__FILE__, __LINE__, "Remove ConnectionEndpointList Success.\n");
    }
}



//////////////////////////////////////////////////////////////////////////
// 名称: MallocConnectionItem
// 说明: 分配连接上下文结构体内存
// 返回: 分配的连接上下文结构体内存地址
// 备注: 如果分配失败,返回值为NULL
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PCONNECTION_ITEM MallocConnectionItem()
{
    PCONNECTION_ITEM pConnectionItem = ExAllocateFromNPagedLookasideList( &g_ConnectionEndpointListPool );

    if ( NULL != pConnectionItem )
    {
        RtlZeroMemory( pConnectionItem, sizeof(CONNECTION_ITEM) );
        InitializeListHead( &pConnectionItem->ListEntry );

		//InitializeListHead(&pConnectionItem->ThreadDataList);

		//KeInitializeMutex(&pConnectionItem->AppContext.sendBuffer.kMutex, FALSE);

        // 初始化未知连接状态
        pConnectionItem->ConnectType = UNKNOWN_STATE;
    }

    return pConnectionItem;
}


//////////////////////////////////////////////////////////////////////////
// 名称: MallocAddressItem
// 说明: 分配传输地址结构体内存
// 返回: 分配的传输地址结构体内存地址
// 备注: 如果分配失败,返回值为NULL
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PADDRESS_ITEM MallocAddressItem()
{
    PADDRESS_ITEM pAddressItem = ExAllocateFromNPagedLookasideList( &g_AddressFileListPool );

    if ( NULL != pAddressItem )
    {
        RtlZeroMemory( pAddressItem, sizeof(ADDRESS_ITEM) );
        InitializeListHead( &pAddressItem->ListEntry );
    }

    return pAddressItem;
}



//////////////////////////////////////////////////////////////////////////
// 名称: FreeAddressItem
// 说明: 释放传输地址关联结构体内存
// 入参: pAddressItem 地址结构体内存
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID FreeAddressItem( IN PADDRESS_ITEM pAddressItem )
{
    ASSERT( pAddressItem != NULL );

    ExFreeToNPagedLookasideList( &g_AddressFileListPool, pAddressItem );
}


//////////////////////////////////////////////////////////////////////////
// 名称: FreeConnectionItem
// 说明: 释放连接上下文相关结构体内存
// 入参: pConnectionitem 连接上下文结构体内存
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID FreeConnectionItem( IN PCONNECTION_ITEM pConnectionItem )
{
    ASSERT( NULL != pConnectionItem );

    ExFreeToNPagedLookasideList( &g_ConnectionEndpointListPool, pConnectionItem );
}


//////////////////////////////////////////////////////////////////////////
// 名称: InsertAddressItemToList
// 说明: 插入传输地址项到传输地址项的链表中
// 入参: pAddressItem 传输地址结构体指针
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID InsertAddressItemToList( IN PADDRESS_ITEM pAddressItem )
{
    ASSERT( NULL != pAddressItem );

    AddressFileList_Lock();
    InsertTailList( &g_AddressFileListHead, &pAddressItem->ListEntry );
    AddressFileList_Unlock();
}



//////////////////////////////////////////////////////////////////////////
// 名称: InsertConnectionItemToList
// 说明: 插入到连接上下文链表中
// 入参: pConnectionItem 连接上下文结构体内存
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
VOID InsertConnectionItemToList( IN PCONNECTION_ITEM pConnectionItem )
{
    ASSERT( NULL != pConnectionItem );

    ConnectionEndpointList_Lock();
    InsertTailList( &g_ConnectionEndpointListHead, &pConnectionItem->ListEntry );
    ConnectionEndpointList_Unlock();
}



//////////////////////////////////////////////////////////////////////////
// 名称: DeleteConnectionItemFromList
// 说明: 从链表中删除连接上下文结构体内存
// 入参: pConnectionItem 要删除的连接上下文指针
// 返回: 操作的结果
// 备注: 元素只有在链表中的时候才其效果
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
BOOLEAN DeleteConnectionItemFromList( IN PCONNECTION_ITEM pItem )
{
    ASSERT( NULL != pItem );

    if ( IsListEmpty(&pItem->ListEntry) )
    {
        DbgMsg(__FILE__, __LINE__, "DeleteConnectionItemFromList List is empty...\n");
        return FALSE;
    }

    // 从链表中移除数据
    ConnectionEndpointList_Lock();
    RemoveEntryList( &pItem->ListEntry );
    FreeConnectionItem( pItem );
    ConnectionEndpointList_Unlock();

    return TRUE;
}



//////////////////////////////////////////////////////////////////////////
// 名称: DeleteAddressItemFromList
// 说明: 从链表中删除传输地址结构体内存
// 入参: pAddressItem 要删除的传输地址结构体内存指针
// 返回: 操作的结果
// 备注: 元素只有在链表中的时候才其效果
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
BOOLEAN DeleteAddressItemFromList( IN PADDRESS_ITEM pAddrItem )
{
    ASSERT( NULL != pAddrItem );

    if ( IsListEmpty(&pAddrItem->ListEntry) )
    {
        DbgMsg(__FILE__, __LINE__, "DeleteAddressItemFromList List is empty...\n");
        return FALSE;
    }

    // 从链表中移除数据
    AddressFileList_Lock();
    RemoveEntryList( &pAddrItem->ListEntry );
    FreeAddressItem( pAddrItem );
    AddressFileList_Unlock();

    return TRUE;
}



//////////////////////////////////////////////////////////////////////////
// 名称: FindAddressItemByFileObj
// 说明: 根据FileObject查找对应的传输地址结构体指针
// 入参: FileObject 传输地址文件对象指针
// 返回: 与FileObject对应的传输地址结构体指针
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PADDRESS_ITEM FindAddressItemByFileObj( IN PFILE_OBJECT FileObject )
{
    PLIST_ENTRY pLink = NULL;
    PADDRESS_ITEM pResult = NULL;

    if ( IsListEmpty(&g_AddressFileListHead) )
    {
        DbgMsg(__FILE__, __LINE__, "FindAddressItem AddressFileList is empty...\n");
        return NULL;
    }

    // 遍历整个传输地址结构体链表
    AddressFileList_Lock();
    for ( pLink = g_AddressFileListHead.Flink; 
          pLink != &g_AddressFileListHead; 
          pLink = pLink->Flink )
    {
        PADDRESS_ITEM pAddressItem = CONTAINING_RECORD( pLink, ADDRESS_ITEM, ListEntry );

        if ( pAddressItem->FileObject == FileObject )
        {
            // 保存查找结果
            pResult = pAddressItem;
            break;
        }
    }
    AddressFileList_Unlock();

    return pResult;
}


//////////////////////////////////////////////////////////////////////////
// 名称: FindConnectItemByFileObj
// 说明: 根据FileObject查找对应的连接上下文结构体指针
// 入参: FileObject 连接上下文文件对象指针
// 返回: 与FileObject 对应的连接上下文结构体指针
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PCONNECTION_ITEM FindConnectItemByFileObj( IN PFILE_OBJECT FileObject )
{
    PLIST_ENTRY pLink = NULL;
    PCONNECTION_ITEM pResult = NULL;

    if ( IsListEmpty(&g_ConnectionEndpointListHead) )
    {
        DbgMsg(__FILE__, __LINE__, "FindConnectionItem ConnectionEndpointList is empty...\n");
        return NULL;
    }

    // 遍历整个连接上下文结构体链表
    ConnectionEndpointList_Lock();
    for ( pLink = g_ConnectionEndpointListHead.Flink;
          pLink != &g_ConnectionEndpointListHead;
          pLink = pLink->Flink )
    {
        PCONNECTION_ITEM pConnectionItem = CONTAINING_RECORD( pLink, CONNECTION_ITEM, ListEntry );

        if ( pConnectionItem->FileObject == FileObject )
        {
            // 保存查找结果
            pResult = pConnectionItem;
            break;
        }
    }
    ConnectionEndpointList_Unlock();

    return pResult;
}


//////////////////////////////////////////////////////////////////////////
// 名称: FindConnectItemByConnectContext
// 说明: 根据ConnectionContext(连接上下文指针)查找对应的连接上下文结构体指针
// 入参: ConnectionContext 连接上下文指针
// 返回: 与ConnectionContext 对应的连接上下文结构体指针
// 备注: 
// email: cppcoffee@gmail.com
//////////////////////////////////////////////////////////////////////////
PCONNECTION_ITEM FindConnectItemByConnectContext( IN CONNECTION_CONTEXT ConnectionContext )
{
    PLIST_ENTRY pLink = NULL;
    PCONNECTION_ITEM pResult = NULL;

    if ( IsListEmpty(&g_ConnectionEndpointListHead) )
    {
        DbgMsg(__FILE__, __LINE__, "FindConnectionItem ConnectionEndpointList is empty...\n");
        return NULL;
    }

    // 遍历整个连接上下文结构体链表
    ConnectionEndpointList_Lock();
    for ( pLink = g_ConnectionEndpointListHead.Flink;
        pLink != &g_ConnectionEndpointListHead;
        pLink = pLink->Flink )
    {
        PCONNECTION_ITEM pConnectionItem = CONTAINING_RECORD( pLink, CONNECTION_ITEM, ListEntry );

        if ( pConnectionItem->ConnectionContext == ConnectionContext )
        {
            // 保存查找结果
            pResult = pConnectionItem;
            break;
        }
    }
    ConnectionEndpointList_Unlock();

    return pResult;
}



