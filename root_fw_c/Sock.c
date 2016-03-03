#include "Sock.h"
#include "stdafx.h"


// 全局变量
char AddressBuffer[20];

char* inet_ntoa(u_long addr)
{
    IN_ADDR inaddr;

    inaddr.s_addr = addr;
    RtlZeroMemory( AddressBuffer, _countof(AddressBuffer) );
    RtlStringCchPrintfA( AddressBuffer, _countof(AddressBuffer), "%d.%d.%d.%d", 
        inaddr.s_net, 
        inaddr.s_host, 
        inaddr.s_lh, 
        inaddr.s_impno 
        );

    return AddressBuffer;
}


u_long ntohl(u_long netlong)
{
    u_long result = 0;
    ((char *)&result)[0] = ((char *)&netlong)[3];
    ((char *)&result)[1] = ((char *)&netlong)[2];
    ((char *)&result)[2] = ((char *)&netlong)[1];
    ((char *)&result)[3] = ((char *)&netlong)[0];
    return result;
}


u_short ntohs(u_short netshort)
{
    u_short result = 0;
    ((char *)&result)[0] = ((char *)&netshort)[1];
    ((char *)&result)[1] = ((char *)&netshort)[0];
    return result;
}

