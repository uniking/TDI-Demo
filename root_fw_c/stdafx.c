//This file is used to build a precompiled header
#include "stdafx.h"


// 打印IP地址信息
void PrintIpAddressInfo( PCHAR pszMsg, PTA_ADDRESS pTaAddress )
{
    PTDI_ADDRESS_IP ipAddr = (PTDI_ADDRESS_IP)pTaAddress->Address;

    KdPrint(( "[root_fw] %s %s: %d, AddressLength: %d.\n", 
        pszMsg, 
        inet_ntoa(ipAddr->in_addr), 
        ntohs(ipAddr->sin_port), 
        pTaAddress->AddressLength ));
}


void DbgMsg(char *lpszFile, int Line, char *lpszMsg, ...)
{
#ifdef DBG
    char szBuff[0x100], szOutBuff[0x100];
    va_list mylist;

    va_start(mylist, lpszMsg);
    RtlStringCbVPrintfA( szBuff, _countof(szBuff), lpszMsg, mylist);
    va_end(mylist);

    RtlStringCbPrintfA( szOutBuff, _countof(szOutBuff), "%s(%d) : %s", lpszFile, Line, szBuff);

    DbgPrint(szOutBuff);
#endif
}


