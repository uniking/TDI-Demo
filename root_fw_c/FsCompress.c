//#include "../inc/FsGlobal.h"
//#include <ndis.h >
#include "FsCompress.h"



int FsDataCompress62(BYTE* pucData, 
					 DWORD dwLength, 
					 BYTE* pucCompress, 
					 DWORD* pdwCompressLength)
{
    BYTE* pucBuffer = (BYTE*)ExAllocatePoolWithTag(NonPagedPool ,0x10010, Tagi);
    memset(pucBuffer, 0, 0x10010);

    FsDataCompressDo62(pucData, dwLength, pucCompress, pdwCompressLength, pucBuffer);

    if (NULL != pucBuffer)
    {
        ExFreePoolWithTag(pucBuffer, Tagi);
        pucBuffer = NULL;
    }

    return 0;
}

int FsDataCompressDo62(BYTE* pucData, 
                       DWORD dwLength, 
                       BYTE* pucCompress, 
                       DWORD* pdwCompressLength, 
                       BYTE* pucBuffer)
{
    lzo1x_1_15_compress(
        pucData, 
        dwLength,
        pucCompress, 
        (lzo_uintp)pdwCompressLength,
        pucBuffer);
    return 0;
}

int FsDataDecompress62(BYTE* pucCompress, 
                       DWORD dwCompressLength, 
                       BYTE* pucData, 
                       DWORD* pdwLength)
{
    return lzo1x_decompress_safe        ( 
        pucCompress, 
        dwCompressLength,
        pucData, 
        (lzo_uintp)pdwLength,
        NULL);
   // return 0;
}

BYTE gFileTypeTable[127][9] = 
{
    //{有效数据个数， 数据1，数据2，数据3，数据4，数据5，数据6，数据7，数据8}
	{0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // 6*9=54
	{0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},//6

	{0x08, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
	{0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
	{0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00},
	{0x05, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00},
	{0x04, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
	{0x03, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00},//12
	
	{0x08, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20},
	{0x07, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00},
	{0x06, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00},
	{0x05, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00},
	{0x04, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00},
	{0x03, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00},//18

	{0x07, 0x25, 0x50, 0x44, 0x46, 0x2d, 0x31, 0x2e, 0x00}, //.pdf
	{0x07, 0x25, 0x50, 0x44, 0x46, 0x2d, 0x32, 0x2e, 0x00}, 
	{0x08, 0x2f, 0x4c, 0x65, 0x6e, 0x67, 0x74, 0x68, 0x20}, 
	{0x08, 0x2f, 0x46, 0x69, 0x6c, 0x74, 0x65, 0x72, 0x20}, 
	{0x08, 0x2f, 0x46, 0x6c, 0x61, 0x74, 0x65, 0x44, 0x65}, 
	{0x08, 0x63, 0x6f, 0x64, 0x65, 0x3e, 0x3e, 0x0a, 0x73}, //24

	{0x06, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x0a, 0x00, 0x00}, 
	{0x08, 0x10, 0x43, 0x61, 0x78, 0x61, 0x45, 0x62, 0x46}, 
	{0x08, 0x6f, 0x72, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77},
	{0x08, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 
	{0x07, 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00, 0x00}, 
	{0x04, 0x52, 0x61, 0x72, 0x21, 0x00, 0x00, 0x00, 0x00},//30
	
	{0x08, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x00}, 
	{0x08, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x02, 0x00},
	{0x08, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x03, 0x00}, 
	{0x06, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x00, 0x00}, 
	{0x08, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00}, 
	{0x08, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x02, 0x00},//36
	
	{0x08, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x03, 0x00}, 
	{0x06, 0x50, 0x4b, 0x03, 0x04, 0x0a, 0x00, 0x00, 0x00}, 
	{0x04, 0x50, 0x4b, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00}, 
	{0x04, 0x1f, 0x8b, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00}, 
	{0x04, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 
	{0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},//42
	
	{0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 
	{0x06, 0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x00, 0x00}, 
	{0x06, 0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x00, 0x00}, 
	{0x08, 0x4e, 0x45, 0x54, 0x53, 0x43, 0x41, 0x50, 0x45}, 
	{0x08, 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}, //.png
	{0x08, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52},//48
	
	{0x06, 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x00, 0x00},  
	{0x04, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x00}, 
	{0x08, 0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46}, //.jpg 
	{0x08, 0xff, 0xd8, 0xff, 0xe1, 0x00, 0x10, 0x4a, 0x46},  
	{0x04, 0xff, 0xd8, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0xff, 0xd8, 0xff, 0xe1, 0x00, 0x00, 0x00, 0x00},//54 

	{0x04, 0x00, 0x10, 0x4a, 0x46, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x63, 0x70, 0x70, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x43, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x74, 0x78, 0x74, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x54, 0x58, 0x54, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x64, 0x6f, 0x63, 0x00, 0x00, 0x00, 0x00},//60
	
	{0x04, 0x2e, 0x44, 0x4f, 0x43, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x70, 0x64, 0x66, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x50, 0x44, 0x46, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x65, 0x78, 0x62, 0x00, 0x00, 0x00, 0x00}, 
	{0x04, 0x2e, 0x45, 0x58, 0x42, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x6a, 0x70, 0x67, 0x00, 0x00, 0x00, 0x00},//66
	
	{0x04, 0x2e, 0x4a, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00},   
	{0x04, 0x2e, 0x67, 0x69, 0x66, 0x00, 0x00, 0x00, 0x00},  
	{0x04, 0x2e, 0x47, 0x49, 0x46, 0x00, 0x00, 0x00, 0x00},   
	{0x04, 0x2e, 0x62, 0x6d, 0x70, 0x00, 0x00, 0x00, 0x00},   
	{0x04, 0x2e, 0x42, 0x4d, 0x50, 0x00, 0x00, 0x00, 0x00},   
	{0x04, 0x49, 0x49, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00},//72 

	{0x04, 0x4d, 0x4d, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x00},   
	{0x04, 0x41, 0x43, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00},   
	{0x08, 0x3b, 0x3b, 0x20, 0x48, 0x53, 0x46, 0x20, 0x56},   
	{0x06, 0x53, 0x74, 0x75, 0x64, 0x69, 0x6f, 0x00, 0x00},   
	{0x08, 0x41, 0x75, 0x74, 0x6f, 0x64, 0x65, 0x73, 0x6b},   
	{0x07, 0x57, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x73, 0x00},//78
	
	{0x07, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x00},   
	{0x07, 0x72, 0x65, 0x6c, 0x65, 0x61, 0x73, 0x65, 0x00},  
	{0x08, 0x03, 0xcc, 0x54, 0x00, 0x13, 0x08, 0x00, 0x00},   
	{0x08, 0x03, 0xcc, 0x54, 0x00, 0x23, 0x08, 0x00, 0x00},  
	{0x04, 0x03, 0xcc, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00},   
	{0x06, 0xff, 0xfb, 0x90, 0x74, 0x00, 0x00, 0x00, 0x00},//84 

	{0x06, 0xff, 0xfb, 0x90, 0x44, 0x00, 0x00, 0x00, 0x00},    
	{0x06, 0xff, 0xfb, 0x90, 0x40, 0x00, 0x00, 0x00, 0x00},   
	{0x06, 0xff, 0xfb, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00},    
	{0x03, 0xff, 0xfb, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00},    
	{0x06, 0x6b, 0x72, 0x63, 0x31, 0x38, 0x46, 0x00, 0x00},    
	{0x08, 0x23, 0x21, 0x41, 0x4d, 0x52, 0x2d, 0x57, 0x42},//90 

	{0x07, 0x5f, 0x4d, 0x43, 0x31, 0x2e, 0x30, 0x0a, 0x00},    
	{0x06, 0x23, 0x21, 0x41, 0x4d, 0x52, 0x0a, 0x00, 0x00},    
	{0x05, 0x23, 0x21, 0x41, 0x4d, 0x52, 0x00, 0x00, 0x00},    
	{0x03, 0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00},    
	{0x04, 0x46, 0x57, 0x53, 0x09, 0x00, 0x00, 0x00, 0x00},    
	{0x03, 0x46, 0x57, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00},//96
	
	{0x06, 0x2e, 0x4d, 0x43, 0x41, 0x44, 0x20, 0x00, 0x00},    
	{0x06, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00},   
	{0x05, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00},    
	{0x04, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00},   
	{0x03, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00},    
	{0x08, 0x00, 0x06, 0x86, 0x60, 0x81, 0x22, 0xe7, 0xcd},//102
	
	{0x06, 0x00, 0x06, 0x86, 0x60, 0x81, 0x22, 0x00, 0x00},    
	{0x04, 0x2e, 0x6d, 0x64, 0x62, 0x00, 0x00, 0x00, 0x00},   
	{0x04, 0x2e, 0x4d, 0x44, 0x42, 0x00, 0x00, 0x00, 0x00},    
	{0x06, 0x04, 0x00, 0x04, 0x00, 0x00, 0x60, 0x00, 0x00},    
	{0x08, 0x0a, 0x9d, 0x30, 0x6e, 0xc8, 0x84, 0x91, 0x43},    
	{0x03, 0x42, 0x5a, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00},//108
	
	{0x06, 0x31, 0x41, 0x59, 0x26, 0x53, 0x59, 0x00, 0x00},    
	{0x08, 0x46, 0x4c, 0x59, 0x08, 0x00, 0x00, 0x00, 0x01},    
	{0x07, 0x43, 0x6f, 0x44, 0x65, 0x53, 0x79, 0x73, 0x00},    
	{0x08, 0x14, 0x4b, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00},   
	{0x08, 0x43, 0x58, 0x50, 0x46, 0x42, 0x4c, 0x43, 0x4b},    
	{0x08, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38},//114 

	{0x08, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48},    
	{0x08, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68},   
	{0x03, 0x54, 0x4f, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00},    
	{0x08, 0x43, 0x4f, 0x4d, 0x50, 0x52, 0x45, 0x53, 0x53},   
	{0x08, 0x64, 0x65, 0x78, 0x0a, 0x30, 0x33, 0x35, 0x00},    
	{0x08, 0x6f, 0x6d, 0x70, 0x72, 0x65, 0x73, 0x73, 0x65},//120

	{0x08, 0x6c, 0x65, 0x63, 0x74, 0x72, 0x6f, 0x6e, 0x69},     
	{0x04, 0x38, 0x42, 0x50, 0x53, 0x00, 0x00, 0x00, 0x00},    
	{0x07, 0x50, 0x41, 0x43, 0x4b, 0x00, 0x00, 0x00, 0x00},    
	{0x08, 0x23, 0x4c, 0x63, 0x55, 0x53, 0x74, 0x72, 0x65},    
	{0x08, 0x47, 0x54, 0x41, 0x50, 0x50, 0x14, 0x00, 0x00},     
	{0x08, 0x73, 0x74, 0x5f, 0x47, 0x4c, 0x4a, 0x48, 0x5a},//126
	
	{0x07, 0x78, 0x6f, 0x66, 0x20, 0x30, 0x33, 0x30, 0x00},//127  127*9=1143
};

int FsDataCompress6364(BYTE* pucData, DWORD dwDataLength, BYTE* pucCompress, DWORD* pdwCompressLength)
{
    int     nRtn                = 0;
	BYTE*   result              = 0;
	BYTE*   pCharacterPointer   = NULL;
	DWORD*  pdwLength           = NULL;
	BYTE*   nFindCharacter      = NULL;
	BYTE*   pCharacterOffsect   = NULL;
	int     nData2              = 0;
	__int16 wCompressValue      = 0;
	BYTE*   pucTempCompress2    = NULL;
	WORD    wCharacter          = 0;
	int     nValidCharacterNum  = 0;
	int     nValidCharacterNum2 = 0;
	BYTE*   pucTempCompress     = 0;
	DWORD   dwCount             = 0;
	int     nData1              = 0;
	DWORD   nVertical           = 0;
	int     noffset             = 0;
    int     a                   = 0;
    int     b                   = 0;
    int     c                   = 0;
    int     d                   = 0;

	result = AnalyseFileType6364(pucData, dwDataLength, 500, &dwCount);
	pCharacterPointer = result;

	if ( result != NULL )
	{//0表示没有搜索到特征值
		pdwLength = pdwCompressLength;
		*pdwCompressLength = dwDataLength;
		if ( result != pucData )
		{//特征码不在文件开始处
			pucTempCompress2 = pucCompress;
			pucCompress += 2;
			wCharacter = ((WORD)(result - pucData) | 0xFE00);//
			*(WORD *)pucTempCompress2 = wCharacter;//第一个字节，特征串与数据头的偏移 | 0xFE00
			*pdwLength += 2;
			nData1 = wCharacter;
		}

		do
		{
            //数据头到特征码之间的数据，直接复制到压缩区
			nValidCharacterNum = (int)(pCharacterPointer - pucData);
			memcpy(pucCompress, pucData, nValidCharacterNum);

            //复制一段，data长度变小
			pucCompress += nValidCharacterNum;
			dwDataLength += (unsigned int)(pucData - pCharacterPointer);

            //指向搜索到的特征数组
			pCharacterOffsect = (BYTE*)gFileTypeTable + dwCount * 9;

            //获取有效特征值数目
			nValidCharacterNum = *pCharacterOffsect;
			nValidCharacterNum2 = *pCharacterOffsect;

            //调节数据区信息，dwDataLength减去特征值数，pucData指向特征值之后
			dwDataLength -= nValidCharacterNum2;
			pucData = pCharacterPointer + nValidCharacterNum2;

            //执行压缩算法

            // nData1低6位有效    或
            //
            // 纵坐标左移3位               或
            //
            // 1，有效特征数目-2
            // 2，取低3位为有效位
            // 3，左移6位

            //计算出的特征值包含 纵坐标 和 有效特征数目 两种信息
            //
            //       xxxx      xxxx          xxx0           00|00        0000
            //                  有效特征数目-2|纵坐标（高3位）     Data
            //
            a=(unsigned __int16)(8 * (WORD)dwCount);
            b=(((WORD)nValidCharacterNum - 2) & 7);
            c= nData1 & 0x3F;
            d=a|b|c;

			nData2 = nData1 & 0x3F | (unsigned __int16)(((unsigned __int16)(8 * (WORD)dwCount) | ((WORD)nValidCharacterNum - 2) & 7) << 6);
			nData1 = nData2;
			nFindCharacter = AnalyseFileType6364(pucData, dwDataLength, 63, &nVertical);

            //找到了，并且数据长度大于7
			if ( nFindCharacter!=NULL && dwDataLength > 7 )
            {//（特征码 异或 偏移的低6位有效）  与 （特征码 异或 0x3F）
				wCompressValue = ((BYTE)nData1 ^ (unsigned __int8)(nFindCharacter - pucData)) & 0x3F ^ nData2;
                //效果，偏移放入低6位
            }
			else
            {//没有找到，或数据太短
				wCompressValue = nData2 | 0x3F; //
            }

			pucTempCompress = pucCompress;
			pucCompress += 2;
			*(WORD *)pucTempCompress = wCompressValue;//特征码存入压缩缓存
            /*解密，读取一个字节，*/

			*(DWORD *)pdwCompressLength += 2 - (*pCharacterOffsect);
			nData1 = wCompressValue; //本次特征码，作为下次特征码的计算参数
			pCharacterPointer = nFindCharacter;
			dwCount = nVertical;
		}while ( nFindCharacter != NULL );

        //复制最后一段数据到压缩区
		memcpy(pucCompress, pucData, dwDataLength);

		nRtn = 1;
	}

	return nRtn;
}

/*
小于等于8，返回0
函数作用：用数据的前8位去匹配每一个表，

dwLength 数据长度
nMaxLength 搜索的最大长度
pdwCount 返回特征码纵坐标

返回值：搜索到的特征码在Data中的位置
**/
BYTE* AnalyseFileType6364(BYTE* pucData, DWORD dwLength, int nMaxLength, DWORD* pdwCount)
{
	int     dDataLength             = 0;
	BYTE*   nRtn                    = NULL;
	WORD    wCharacter              = 0;
	BYTE*   pVertical               = NULL;
	BYTE*   pValidCharacterPointer  = 0;
	int     nValidDataNum           = 0;
	BOOL    bResult                 = FALSE;
	BYTE*   pDataPointer            = NULL;
	int     i                       = 0;
	int     nValidLength            = 0;
	int     nCounter                = 0;
	
	dDataLength = dwLength;
	if ( (unsigned int)dwLength >= 8 )
	{//小于等于8不压缩？
		dwLength = 0;
		i = 0;
		nValidLength = dDataLength - 7;
		if ( dDataLength != 7 )
		{
			do
			{
				nCounter = 0; 
				wCharacter = *(WORD *)pucData;
				pVertical = (BYTE*)gFileTypeTable;
				do
				{
					pValidCharacterPointer = pVertical + 1;
					if ( *(WORD *)(pVertical + 1) == wCharacter )
					{//比较数组的第二个值
						nValidDataNum = *pVertical;
						if ( *pVertical )
						{
							if ( (_BYTE)nValidDataNum <= 8 )
							{
								pDataPointer = pucData;
								nValidDataNum = (unsigned __int8)nValidDataNum;
								bResult = 1;
								do
								{
									if ( !nValidDataNum )
                                    {//0，退出循环
										break;
                                    }

                                    if(*pDataPointer++ == *pValidCharacterPointer++)
                                    {
                                        bResult = TRUE;
                                    }
                                    else
                                    {
                                        bResult = FALSE;
                                    }

									--nValidDataNum;
								}while ( bResult );

								if ( bResult )
								{
									*(_DWORD *)pdwCount = nCounter;
									return pucData;
								}
							}
						}
					}
					++nCounter;
					pVertical += 9;
				}while ( pVertical < (BYTE*)gFileTypeTable+127*9+1 );

				++pucData;
				++dwLength;
				if ( dwLength >= (unsigned int)nMaxLength )
                {
					break;
                }
				++i;
			}while ( i < nValidLength );
		}
		nRtn = NULL;
	}
	else
	{
		nRtn = NULL;
	}
	return nRtn;
}

int gLength = 7;
BYTE gData[8] = {0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x60 };

signed int FsDataDecompress6364(BYTE* pucCompress, DWORD dwCompressLength, BYTE* pucData, DWORD* pdwLength)
{
	BYTE*           pucDataPointer              = NULL;
	DWORD*          pdwDataLength               = NULL;
	BYTE*           pucCompressPointer          = NULL;
	int             nValidCharacterLength       = 0;
	BYTE*           pucTempCompress             = NULL;
	unsigned int    FileTypeTableCounter        = 0;
	int             nValidCharacterLength2      = 0;
	WORD            wData1                      = 0;
	unsigned int    nCharacterOffset            = 0;
	BYTE*           pucTempDate                 = NULL;
	WORD            wLabel                      = 0;
	WORD            cData2                      = 0;
	
	pucCompressPointer = pucCompress;
	pdwDataLength = pdwLength;
	*pdwLength = dwCompressLength;

	wLabel = *(WORD *)pucCompressPointer & 0xFE00;
	pucDataPointer = pucData;
	pucCompress = pucCompressPointer;

	if (wLabel != 0xfe00 )
    {//压缩数据的第一个字符前7个bit为不都是1
		goto LABEL_13;
    }

	*pdwLength -= 2;
	cData2 = *(WORD *)pucCompressPointer;
	dwCompressLength -= 2;
	pucTempCompress = pucCompressPointer + 2;

    //低9位为有效数字
	memcpy((void *)pucDataPointer, (VOID*)pucTempCompress, cData2 & 0x1FF);

	nValidCharacterLength = *(WORD *)pucCompress & 0x1FF;
	while ( 1 )
	{
		pucDataPointer = pucDataPointer + nValidCharacterLength;
		if ( dwCompressLength < (unsigned int)nValidCharacterLength )
        {
			return 0;
        }

        //调整压缩数据的信息
		dwCompressLength -= nValidCharacterLength;
		pucCompressPointer = pucTempCompress + nValidCharacterLength;
		pucCompress = pucCompressPointer;

LABEL_13:
		wData1 = *(WORD *)pucCompressPointer;
		pucData = (BYTE*)*(WORD *)pucCompressPointer;//todo
		if ( (wData1 & 0x3F) == 63 )
        {
			break;
        }

		FileTypeTableCounter = 9 * ((unsigned int)wData1 >> 9);

		nValidCharacterLength2 = (wData1 >> 6) & 7;
		if ( nValidCharacterLength2 == *(_BYTE *)((BYTE*)gFileTypeTable + FileTypeTableCounter) - 2 )
		{//压缩数据中的值与表中的值对应
			memcpy((VOID*)pucDataPointer, (VOID*)((BYTE*)gFileTypeTable + FileTypeTableCounter + 1), nValidCharacterLength2 + 2);
			*pdwDataLength += nValidCharacterLength2;
			pucDataPointer = pucDataPointer + (nValidCharacterLength2 + 2);
		}
		else
		{//不对应，gdata数据
			if ( nValidCharacterLength2 != gLength - 2 || (wData1 & 0xFE00) != 0xD200 )
            {
				return 0;
            }

			memcpy((VOID*)pucDataPointer, (VOID*)gData, gLength);
			*pdwDataLength += gLength - 2;
			pucDataPointer = pucDataPointer + gLength;
		}

		if ( (unsigned int)dwCompressLength < 2 )
        {
			return 0;
        }

		cData2 = *(WORD *)pucCompress;
		dwCompressLength -= 2;
		pucTempCompress = pucCompressPointer + 2;
		memcpy((VOID*)pucDataPointer, (VOID*)pucTempCompress, cData2 & 0x3F); //低6位有效，复制非特征数据
		nValidCharacterLength = *(WORD *)pucCompress & 0x3F;
	}


	wData1 = *(WORD *)pucCompressPointer;
	//pucData = *(WORD *)pCompressPointer;
	pucData = (BYTE*)*(WORD *)pucCompressPointer;
	nCharacterOffset = 9 * ((unsigned int)wData1 >> 9);//某一个位含有文件类型表的纵坐标，向右移动9位解密
	nValidCharacterLength2 = (wData1 >> 6) & 7;
	if ( nValidCharacterLength2 == *(_BYTE *)((BYTE*)gFileTypeTable + nCharacterOffset) - 2 )
	{
        //从文件类型表中复制数据？
		memcpy((VOID*)pucDataPointer, (VOID*)((BYTE*)gFileTypeTable + nCharacterOffset + 1), nValidCharacterLength2 + 2);
		*pdwDataLength += ((unsigned int)*(WORD *)pucCompressPointer >> 6) & 7;
		pucTempDate = pucDataPointer + (((unsigned int)*(WORD *)pucCompressPointer >> 6) & 7) + 2;
	}
	else
	{
		if ( nValidCharacterLength2 != gLength - 2 || (wData1 & 0xFE00) != 0xD200 )
        {
			return 0;
        }

        //为什么复制gData中的数据
		memcpy((VOID*)pucDataPointer, (VOID*)gData, gLength);
		*pdwDataLength += gLength - 2;
		pucTempDate = pucDataPointer + gLength;
	}

	if ( (unsigned int)dwCompressLength < 2 )
    {
		return 0;
    }

	memcpy((VOID*)pucTempDate, (VOID*)(pucCompressPointer + 2), dwCompressLength - 2);

	return 1;
}

// 6364压缩算法 参考了文件格式

// f83ea8b0 08 00 00 00 00 00 00 00 00  .........
// f83ea8b9 07 00 00 00 00 00 00 00 00  .........
// f83ea8c2 06 00 00 00 00 00 00 00 00  .........
// f83ea8cb 05 00 00 00 00 00 00 00 00  .........
// f83ea8d4 04 00 00 00 00 00 00 00 00  .........
// f83ea8dd 03 00 00 00 00 00 00 00 00  .........
// f83ea8e6 08 ff ff ff ff ff ff ff ff  .........
// f83ea8ef 07 ff ff ff ff ff ff ff 00  .........
// f83ea8f8 06 ff ff ff ff ff ff 00 00  .........
// f83ea901 05 ff ff ff ff ff 00 00 00  .........
// f83ea90a 04 ff ff ff ff 00 00 00 00  .........
// f83ea913 03 ff ff ff 00 00 00 00 00  .........
// f83ea91c 08 20 20 20 20 20 20 20 20  .        
// f83ea925 07 20 20 20 20 20 20 20 00  .       .
// f83ea92e 06 20 20 20 20 20 20 00 00  .      ..
// f83ea937 05 20 20 20 20 20 00 00 00  .     ...
// f83ea940 04 20 20 20 20 00 00 00 00  .    ....
// f83ea949 03 20 20 20 00 00 00 00 00  .   .....
// f83ea952 07 25 50 44 46 2d 31 2e 00  .%PDF-1..
// f83ea95b 07 25 50 44 46 2d 32 2e 00  .%PDF-2..
// f83ea964 08 2f 4c 65 6e 67 74 68 20  ./Length 
// f83ea96d 08 2f 46 69 6c 74 65 72 20  ./Filter 
// f83ea976 08 2f 46 6c 61 74 65 44 65  ./FlateDe
// f83ea97f 08 63 6f 64 65 3e 3e 0a 73  .code>>.s
// f83ea988 06 74 72 65 61 6d 0a 00 00  .tream...
// f83ea991 08 10 43 61 78 61 45 62 46  ..CaxaEbF
// f83ea99a 08 6f 72 57 69 6e 64 6f 77  .orWindow
// f83ea9a3 08 00 0d 00 00 00 00 00 00  .........
// f83ea9ac 07 52 61 72 21 1a 07 00 00  .Rar!....
// f83ea9b5 04 52 61 72 21 00 00 00 00  .Rar!....
// f83ea9be 08 50 4b 03 04 14 00 00 00  .PK......
// f83ea9c7 08 50 4b 03 04 14 00 02 00  .PK......
// f83ea9d0 08 50 4b 03 04 14 00 03 00  .PK......
// f83ea9d9 06 50 4b 03 04 14 00 00 00  .PK......
// f83ea9e2 08 50 4b 03 04 0a 00 00 00  .PK......
// f83ea9eb 08 50 4b 03 04 0a 00 02 00  .PK......
// f83ea9f4 08 50 4b 03 04 0a 00 03 00  .PK......
// f83ea9fd 06 50 4b 03 04 0a 00 00 00  .PK......
// f83eaa06 04 50 4b 03 04 00 00 00 00  .PK......
// f83eaa0f 04 1f 8b 08 08 00 00 00 00  .........
// f83eaa18 04 28 00 00 00 00 00 00 00  .(.......
// f83eaa21 04 01 00 00 00 00 00 00 00  .........
// f83eaa2a 04 02 00 00 00 00 00 00 00  .........
// f83eaa33 06 47 49 46 38 39 61 00 00  .GIF89a..
// f83eaa3c 06 47 49 46 38 37 61 00 00  .GIF87a..
// f83eaa45 08 4e 45 54 53 43 41 50 45  .NETSCAPE
// f83eaa4e 08 89 50 4e 47 0d 0a 1a 0a  ..PNG....
// f83eaa57 08 00 00 00 0d 49 48 44 52  .....IHDR
// f83eaa60 06 89 50 4e 47 0d 0a 00 00  ..PNG....
// f83eaa69 04 49 48 44 52 00 00 00 00  .IHDR....
// f83eaa72 08 ff d8 ff e0 00 10 4a 46  .......JF
// f83eaa7b 08 ff d8 ff e1 00 10 4a 46  .......JF
// f83eaa84 04 ff d8 ff e0 00 00 00 00  .........
// f83eaa8d 04 ff d8 ff e1 00 00 00 00  .........
// f83eaa96 04 00 10 4a 46 00 00 00 00  ...JF....
// f83eaa9f 04 2e 63 70 70 00 00 00 00  ..cpp....
// f83eaaa8 04 2e 43 50 50 00 00 00 00  ..CPP....
// f83eaab1 04 2e 74 78 74 00 00 00 00  ..txt....
// f83eaaba 04 2e 54 58 54 00 00 00 00  ..TXT....
// f83eaac3 04 2e 64 6f 63 00 00 00 00  ..doc....
// f83eaacc 04 2e 44 4f 43 00 00 00 00  ..DOC....
// f83eaad5 04 2e 70 64 66 00 00 00 00  ..pdf....
// f83eaade 04 2e 50 44 46 00 00 00 00  ..PDF....
// f83eaae7 04 2e 65 78 62 00 00 00 00  ..exb....
// f83eaaf0 04 2e 45 58 42 00 00 00 00  ..EXB....
// f83eaaf9 04 2e 6a 70 67 00 00 00 00  ..jpg....
// f83eab02 04 2e 4a 50 47 00 00 00 00  ..JPG....
// f83eab0b 04 2e 67 69 66 00 00 00 00  ..gif....
// f83eab14 04 2e 47 49 46 00 00 00 00  ..GIF....
// f83eab1d 04 2e 62 6d 70 00 00 00 00  ..bmp....
// f83eab26 04 2e 42 4d 50 00 00 00 00  ..BMP....
// f83eab2f 04 49 49 2a 00 00 00 00 00  .II*.....
// f83eab38 04 4d 4d 2a 00 00 00 00 00  .MM*.....
// f83eab41 04 41 43 31 30 00 00 00 00  .AC10....
// f83eab4a 08 3b 3b 20 48 53 46 20 56  .;; HSF V
// f83eab53 06 53 74 75 64 69 6f 00 00  .Studio..
// f83eab5c 08 41 75 74 6f 64 65 73 6b  .Autodesk
// f83eab65 07 57 69 6e 64 6f 77 73 00  .Windows.
// f83eab6e 07 76 65 72 73 69 6f 6e 00  .version.
// f83eab77 07 72 65 6c 65 61 73 65 00  .release.
// f83eab80 08 03 cc 54 00 13 08 00 00  ...T.....
// f83eab89 08 03 cc 54 00 23 08 00 00  ...T.#...
// f83eab92 04 03 cc 54 00 00 00 00 00  ...T.....
// f83eab9b 06 ff fb 90 74 00 00 00 00  ....t....
// f83eaba4 06 ff fb 90 44 00 00 00 00  ....D....
// f83eabad 06 ff fb 90 40 00 00 00 00  ....@....
// f83eabb6 06 ff fb 90 04 00 00 00 00  .........
// f83eabbf 03 ff fb 90 00 00 00 00 00  .........
// f83eabc8 06 6b 72 63 31 38 46 00 00  .krc18F..
// f83eabd1 08 23 21 41 4d 52 2d 57 42  .#!AMR-WB
// f83eabda 07 5f 4d 43 31 2e 30 0a 00  ._MC1.0..
// f83eabe3 06 23 21 41 4d 52 0a 00 00  .#!AMR...
// f83eabec 05 23 21 41 4d 52 00 00 00  .#!AMR...
// f83eabf5 03 1f 8b 08 00 00 00 00 00  .........
// f83eabfe 04 46 57 53 09 00 00 00 00  .FWS.....
// f83eac07 03 46 57 53 00 00 00 00 00  .FWS.....
// f83eac10 06 2e 4d 43 41 44 20 00 00  ..MCAD ..
// f83eac19 06 30 30 30 30 30 30 00 00  .000000..
// f83eac22 05 30 30 30 30 30 00 00 00  .00000...
// f83eac2b 04 30 30 30 30 00 00 00 00  .0000....
// f83eac34 03 30 30 30 00 00 00 00 00  .000.....
// f83eac3d 08 00 06 86 60 81 22 e7 cd  ....`."..
// f83eac46 06 00 06 86 60 81 22 00 00  ....`."..
// f83eac4f 04 2e 6d 64 62 00 00 00 00  ..mdb....
// f83eac58 04 2e 4d 44 42 00 00 00 00  ..MDB....
// f83eac61 06 04 00 04 00 00 60 00 00  ......`..
// f83eac6a 08 0a 9d 30 6e c8 84 91 43  ...0n...C
// f83eac73 03 42 5a 68 00 00 00 00 00  .BZh.....
// f83eac7c 06 31 41 59 26 53 59 00 00  .1AY&SY..
// f83eac85 08 46 4c 59 08 00 00 00 01  .FLY.....
// f83eac8e 07 43 6f 44 65 53 79 73 00  .CoDeSys.
// f83eac97 08 14 4b 08 08 00 00 00 00  ..K......
// f83eaca0 08 43 58 50 46 42 4c 43 4b  .CXPFBLCK
// f83eaca9 08 31 32 33 34 35 36 37 38  .12345678
// f83eacb2 08 41 42 43 44 45 46 47 48  .ABCDEFGH
// f83eacbb 08 61 62 63 64 65 66 67 68  .abcdefgh
// f83eacc4 03 54 4f 50 00 00 00 00 00  .TOP.....
// f83eaccd 08 43 4f 4d 50 52 45 53 53  .COMPRESS
// f83eacd6 08 64 65 78 0a 30 33 35 00  .dex.035.
// f83eacdf 08 6f 6d 70 72 65 73 73 65  .ompresse
// f83eace8 08 6c 65 63 74 72 6f 6e 69  .lectroni
// f83eacf1 04 38 42 50 53 00 00 00 00  .8BPS....
// f83eacfa 07 50 41 43 4b 00 00 00 00  .PACK....
// f83ead03 08 23 4c 63 55 53 74 72 65  .#LcUStre
// f83ead0c 08 47 54 41 50 50 14 00 00  .GTAPP...
// f83ead15 08 73 74 5f 47 4c 4a 48 5a  .st_GLJHZ
// f83ead1e 07 78 6f 66 20 30 33 30 00  .xof 030.

// f83ead27 00 
// f83ead28 07 
// f83ead29 00 04 00 04 00 00 60  ........`

// f83ead30 00 00 00 00 00 00 00 00 07  .........
// f83ead39 56 45 52 53 49 4f 4e 00 08  VERSION..
// f83ead42 43 41 4c 43 55 4c 41 54 04  CALCULAT.
// f83ead4b 4e 41 4d 45 00 00 00 00 04  NAME.....
// f83ead54 44 41 54 45 00 00 00 00 08  DATE.....
// f83ead5d 50 41 53 53 57 4f 52 44 05  PASSWORD.
// f83ead66 3c 3f 78 6d 6c 00 00 00 07  <?xml....
// f83ead6f 76 65 72 73 69 6f 6e 00 05  version..
// f83ead78 65 6e 63 6f 64 00 00 00 05  encod....
// f83ead81 68 74 74 70 3a 00 00 00 04  http:....
// f83ead8a 77 77 77 2e 00 00 00 00 00  www......
// f83ead93 00 00 00 00 00 00 00 00 00  .........
// f83ead9c 00 00 00 00 00 00 00 00 00  .........
// f83eada5 00 00 00 00 00 00 00 00 00  .........
// f83eadae 00 00 00 00 00 00 00 00 00  .........
// f83eadb7 00 00 00 00 00 00 00 00 00  .........
// f83eadc0 00 00 00 00 00 00 00 00 00  .........
// f83eadc9 00 00 00 00 00 00 00 00 00  .........
// f83eadd2 00 00 00 00 00 00 00 00 00  .........
// f83eaddb 00 00 00 00 00 00 00 00 00  .........
// f83eade4 00 00 00 00 00 00 00 00 00  .........
// f83eaded 00 00 00 00 00 00 00 00 00  .........
// f83eadf6 00 00 00 00 00 00 00 00 00  .........