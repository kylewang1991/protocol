/*******************************************************************************
Copyright(C),  上海温尔信息科技有限公司
File name:      protocol_core.c
Author:         王政伟
Mail:           wangzw@hleh.org
Publish time:   2017-07-18
Description:    统一协议核心，该文件在MH70，MH90，MH80设备保持统一
*******************************************************************************/
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "OSAL.h"
#include "comdef.h"
#include "hal_board.h"
#include "hal_types.h"
#include "protocol_core.h"

/*********************** constant *********************************************/
#define attr_R 		(0X01<<0)
#define attr_W		(0x01<<1)
#define attr_R1		(0x01<<2)
#define attr_W1		(0x01<<3)
#define attr_P		(0x01<<4)

#define attr_R_R1	(attr_R|attr_R1)
#define attr_W_W1	(attr_W|attr_W1)
#define attr_R_R1_P	(attr_R|attr_R1|attr_P)

#define frameVersion 0x01
#define frameHeadLenth 3
// #define FIFOLenth		10	
#define invalidItemID	0xFFFF

/*********************** macro ************************************************/


/*********************** data type definition *********************************/




// typedef struct _Type_outFIFOItem_
// {
// 	uint8_t conInter;
// 	uint16_t IDOfItem;
// } TYP_outFIFOItem;

// typedef struct _Type_outFIFO_
// {
// 	uint8_t indexIn;
// 	uint8_t indexOut;
// 	uint8_t numInFIFO;
// 	TYP_outFIFOItem dataInFIFO[FIFOLenth];
// } TYP_outFIFO;


// /*********************** internal/static variable definition & declare ********/
// //在本地化之前请进行一下步骤
// //1，注释掉本产品不支持的参数，即该列表中的某些行
// //2，对参数长度为0的参数，请指定在本设备中该参数的实际长度
// //3，实现回调函数，请注意：必须实现所有该产品支持的参数对应的回调函数
// const static TYP_ProtocolItem protocolItem[] =
// {
// 	{0x0000, attr_R, 			6, 	NULL, 		readMac,			NULL, 				};
// 	{0x0001, attr_R|attr_W, 	4, 	NULL, 		readTime,			writeTime,		 	};
// 	{0x0002, attr_R|attr_W, 	18, NULL, 		readName, 			writeName, 			};
// 	{0x0003, attr_R|attr_W1,	1,	NULL, 		readDeviceType,		writeDeviceType,	};
// 	{0x0004, attr_R|attr_W1,	15, NULL, 		readBatch,			writeBatch，};
// 	{0x0005, attr_R|attr_W1,	16, NULL, 		readWechatMD5,		writeWechatMD5,		};
// 	{0x0006, attr_R|attr_W1,	4,	NULL, 		readHardVer,		writeHardVer,		};
// 	{0x0007, attr_R|attr_W1,	4,	NULL, 		readSoftVer,		writesoftVer,		};
// 	{0x0008, attr_R|attr_W1,	4,	NULL, 		readRrotocolVer,	writeRrotocolVer,	};
// 	{0x0010, attr_R|attr_W,		4,	NULL, 		readLock,			writeLock,			};
// 	{0x0100, attr_R|attr_P,		1,	NULL, 		readBatt,			NULL,		 		};
// 	{0x0101, attr_R|attr_P,		1,	NULL, 		readChargeSta,		NULL,				};
// 	{0x0102, attr_R|attr_P,		1,	NULL, 		readEEPROMSta,		NULL,				};
// 	{0x0103, attr_R|attr_P,		1,	NULL, 		readRTSensorSta,	NULL,				};
// 	{0x0104, attr_R|attr_P,		1,	NULL, 		readScreenSta,		NULL,				};
// 	{0x0110, attr_R|attr_W,		1,	NULL, 		readLEDFlash,		writeLEDFlash,		};
// 	{0x0111, attr_W1,			1,	NULL, 		NULL,				writeScreenAll,		};
// 	{0x0112, attr_W1,			1,	NULL, 		NULL,				writeLEDBlinkOnce,	};
// 	{0x0113, attr_W1,			1,	NULL, 		NULL,				writeScreenLightUp,	};
// 	{0x0114, attr_W1,			1,	NULL, 		NULL,				writeBeepOnce,		};
// 	{0x0115, attr_W1,			1,	NULL, 		NULL,				writeEEPROMInit		};
// 	{0x0116, attr_W1,			1,	NULL, 		NULL,				writeEEPROMCheck	};
// 	{0x0120, attr_R|attr_P,		1,	NULL, 		readProbeStatus,	NULL,				};
// 	{0x0121, attr_R,			1,	NULL, 		readProbeDataUpdata,NULL,				};
// 	{0x0122, attr_R,			6,	NULL, 		readProbeMac,		NULL,				};
// 	{0x0123, attr_R,			1,	NULL, 		readProbeType,		NULL,				};
// 	{0x0124, attr_R,			12,	NULL, 		readProbeBatchNO,	NULL,				};
// 	{0x0125, attr_R,			52,	NULL, 		readProbeRTTable,	NULL,				};
// 	{0x0126, attr_R,			31, NULL, 		readProbeCali,		NULL,				};
// 	{0x0140, attr_R1,			2,	NULL, 		readCaliObj1,		NULL,				};
// 	{0x0141, attr_R|attr_W1,	0,  NULL, 		readCaliObj1Param,	writeCaliObj1Param,	};
// 	{0x0150, attr_W,			1,	NULL, 		NULL,				writeDegradeLinkFreq,};
// 	{0x0151, attr_W,			1,	NULL, 		NULL,				writeClearHistoryData,};
// 	{0x0152, attr_W,			1,	NULL, 		NULL,				writeDeviceClose,	};
// 	{0x0200, attr_R,			2,	NULL, 		readHistoryTotal,	NULL,				};
// 	{0x0201, attr_R,			2,	NULL, 		readHistoryLenth,	NULL,				};
// 	{0x0202, attr_R,			1,	NULL, 		readHisDecodeType,	NULL,				};
// 	{0x0203, attr_R|attr_W,		8,	NULL, 		readUserID,			writeUserID,		};
// 	{0x0204, attr_W,			2,	NULL, 		NULL,				writeHistoryData,	};
// 	{0x0205, attr_P,			0,	NULL, 		getHistoryData,		NULL,				};
// 	{0x0210, attr_R,			1,	NULL, 		readNowDecodeType,	NULL,				};
// 	{0x0211, attr_P,			0,	NULL, 		readRealTimeData,	NULL,				};
// 	{0x0220, attr_W,			0,	NULL, 		NULL,				writeFirmwareBlock, };
// 	{0x0221, attr_P,			2,	NULL, 		getBlockConfirmNO,	NULL,				};
// 	{0x0222, attr_W|attr_P,		2,  NULL, 		readFirmwareTotal,	writeFirmwareTotal,	};
// }

// TYP_outFIFO outItemFIFO =
// {
// 	0,
// 	0,
// 	0,
// }

/*********************** external variable definition *************************/

/*********************** internal/static function declare *********************/
static TYP_ProtocolItem* findItem(uint16_t ItemID)
{
	TYP_ProtocolItem *ptrItem = protocolItem;
	uint8_t *listend = (uint8_t *)protocolItem + sizeof(protocolItem);

	while(ptrItem < listend)
	{
		if(ptrItem -> ID != ItemID)
			ptrItem++;
		else
			break;
	}

	if(ptrItem >= listend)
		return NULL;
	else
		return ptrItem;
	
}

// static void appendToOutFIFO(uint8_t connInter,uint16_t itemID)
// {
// 	if(outItemFIFO.numInFIFO < FIFOLenth)
// 	{
// 		outItemFIFO.dataInFIFO[outItemFIFO.indexIn].conInter = connInter;
// 		outItemFIFO.dataInFIFO[outItemFIFO.indexIn].IDOfItem = itemID;
// 		outItemFIFO.indexIn ++;
// 		if(outItemFIFO.indexIn >= FIFOLenth)
// 			outItemFIFO.indexIn = 0;
// 		outItemFIFO.numInFIFO ++;
// 	}
// }

// static void getFromOutFifo(TYP_outFIFOItem* outFIFOItem)
// {
// 	if(outItemFIFO.numInFIFO > 0)
// 	{
// 		outFIFOItem->conInter = outItemFIFO.dataInFIFO[outItemFIFO.indexOut].conInter;
// 		outFIFOItem->IDOfItem = outItemFIFO.dataInFIFO[outItemFIFO.indexOut].IDOfItem; 
// 		outItemFIFO.indexOut ++;
// 		if(outItemFIFO.indexOut >= FIFOLenth)
// 			outItemFIFO.indexOut = 0;
// 		outItemFIFO.numInFIFO --;
// 	}
// 	else
// 	{
// 		outFIFOItem->conInter = 0xff;
// 		outFIFOItem->IDOfItem = invalidItemID;
// 	}
// }

/*********************** external function declare  ***************************/
void protocolCore_handleDataIn(uint8_t conInter, uint8_t *ptrDataIn, uint8_t inDataLenth)
{
	TYP_ProtocolItem *ptrProtocolItem;
	uint8_t* pBuf;

	//检查是否小于最小数据帧长度
	if(inDataLenth < frameHeadLenth)
		return;
	//检查数据指针是否为空，数据帧版本是否匹配
	if((ptrDataIn == NULL)||(*ptrDataIn != frameVersion))
		return;

	ptrDataIn ++;
	//查抄参数ID在参数表中的位置
	ptrProtocolItem = findItem(*((uint16*)ptrDataIn));
	//如果没有找到相对应的条目，即本参数不支持
	if(ptrProtocolItem == NULL)
		return;

	ptrDataIn += 2;
	//如果是读参数请求
	if(inDataLenth == frameHeadLenth)
	{
		//如果没有读属性
		if(ptrProtocolItem->attribute & attr_R_R1 == 0x00)
			return;
		//如果有attr_R1属性，但是参数被锁死
		if((ptrProtocolItem->attribute & attr_R1 != 0x00)&&(lockFlag == TRUE))
			return;

		if(ptrProtocolItem->getValueFunc != NULL)
			ptrProtocolItem->getValueFunc(ptrProtocolItem);

		if((ptrProtocolItem->ptrValue == NULL) || (ptrProtocolItem->valueLenth == 0))
			return;

		pBuf = allocSpace(conInter, ptrProtocolItem->valueLenth + frameHeadLenth);
		if(pBuf != NULL)
		{
			*pBuf++ = frameVersion;
			*((uint16_t*)pBuf) = ptrProtocolItem->ID;
			pBuf =+ 2;
			osal_memcpy(pBuf, ptrProtocolItem->ptrValue, ptrProtocolItem->valueLenth);
			pBuf -= frameHeadLenth;
			sendData(conInter, pBuf, ptrProtocolItem->valueLenth);					
		}			
	}
	//如果是写参数请求
	else
	{
		//如果没有写属性
		if(ptrProtocolItem->attribute & attr_W_W1 == 0x00)
			return;
		//如果有attr_W1属性，但是参数被锁死
		if((ptrProtocolItem->attribute & attr_W1 != 0x00)&&(lockFlag == TRUE))
			return;

		if((ptrProtocolItem->ptrValue == NULL) || \
		   (ptrProtocolItem->valueLenth != (inDataLenth - frameHeadLenth)))
			return;

		osal_memcpy(ptrProtocolItem->ptrValue, ptrDataIn, ptrProtocolItem->valueLenth);		

		if(ptrProtocolItem->setValueFunc != NULL)
		{
			ptrProtocolItem->setValueFunc(ptrDataIn);				
		}		
	}
	protocolCore_handleDataPush();
}

// void protocolCore_handleDataOut(void)
// {
// 	TYP_outFIFOItem outFIFOItem;
// 	TYP_ProtocolItem *ptrProtocolItem;
// 	uint8_t *pBuf;	

// 	getFromOutFifo(&outFIFOItem);

// 	while(outFIFOItem.IDOfItem != invalidItemID)
// 	{
// 		//查抄参数ID在参数表中的位置
// 		ptrProtocolItem = findItem(outFIFOItem.IDOfItem);
// 		//如果没有找到相对应的条目，即本参数不支持
// 		if((ptrProtocolItem == NULL) || \
// 			(ptrProtocolItem->attribute & attr_R_R1_P == 0x00) || \
// 			(ptrProtocolItem->getValueFunc == NULL) || \
// 			(ptrProtocolItem->valueLenth == 0))
// 		{
// 			getFromOutFifo(&outFIFOItem);				
// 			continue;			
// 		}
		
// 		pBuf = allocSpace(outFIFOItem.conInter, ptrProtocolItem->valueLenth);
// 		if(pBuf != NULL)
// 		{
// 			ptrProtocolItem->getValueFunc(pBuf);
// 			sendData(outFIFOItem.conInter, pBuf)					
// 		}
				
// 		getFromOutFifo(&outFIFOItem);
// 	}
// }









