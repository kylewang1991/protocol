/*******************************************************************************
Copyright(C),  上海温尔信息科技有限公司
File name:      protocol_port.c
Author:         王政伟
Mail:           wangzw@hleh.org
Publish time:   2017-07-18
Description:    统一协议移植层
*******************************************************************************/



/*********************** constant *********************************************/
#define ID_deviceType 	0x0003
#define ID_deviceBatch 	0x0004
#define ID_hardwareVer 	0x0006
#define ID_softwareVer 	0x0007
#define ID_protocolVer 	0x0008
#define CaliObj1Param	0x0141

#define timeCheckLowerLimit 2015
#define timeCheckHigerLimit 2050

/*********************** macro ************************************************/


/*********************** data type definition *********************************/
//参数值读取回调函数，posToWrite指定参数值的存放地址，
typedef void (*coreCallBack)(TYP_ProtocolItem *ptrItem);

typedef struct _Type_caliPoint_
{
	uint8_t NUM_caliPoint;
	uint16_t realValue;
	uint16_t measuredValue;
} TYP_caliPoint;


typedef struct _Type_Protocol_Item_
{
	uint16_t ID;
	uint8_t  attribute;
	uint8_t	 valueLenth;
	uint8_t* ptrValue;
	coreCallBack getValueFunc;
	coreCallBack setValueFunc;
} TYP_ProtocolItem;


/*********************** internal/static variable definition & declare ********/
//该列表应该从protocol_core.c 文件中复制来，并按照以下步骤进行本地化
//1，删除掉本产品不支持的参数，即该列表中的某些行
//2，对参数长度为0的参数，请指定在本设备中该参数的实际长度
//3，添加参数值的存放地址，该地址不可以NULL
//4，如果某参数在读出或写入是会带有某些附加操作，应该定义并实现getValueFunc
//		或setValueFunc函数，参数核心会在写入或读取参数是调用这些函数
const static TYP_ProtocolItem protocolItem[] =
{
	{0x0000, attr_R, 		6, 	MACAddr, 				NULL,				NULL, 				};
	{0x0001, attr_R|attr_W, 4, 	NULL, 					readTime,			writeTime,		 	};
	{0x0002, attr_R|attr_W, 20, deviceName, 			NULL,	 			writeName, 			};
	{0x0003, attr_R|attr_W1,1,	&deviceType,			NULL,				writeCoreEEPROM,	};
	{0x0004, attr_R|attr_W1,15, deviceBatch,			NULL,				writeCoreEEPROM,			};
//	{0x0005, attr_R|attr_W1,16, NULL, 					readWechatMD5,		writeWechatMD5,		};
	{0x0006, attr_R|attr_W1,4,	hardwareVer, 			NULL,				writeCoreEEPROM,		};
	{0x0007, attr_R|attr_W1,4,	softwareVer, 			NULL,				writeCoreEEPROM,		};
	{0x0008, attr_R|attr_W1,4,	protocolVer, 			NULL,				writeCoreEEPROM,	};
	{0x0010, attr_R|attr_W,	4,	lockStatus, 			NULL,				writeLock,			};
	{0x0100, attr_R|attr_P,	1,	&battValue, 			NULL,				NULL,		 		};
	{0x0101, attr_R|attr_P,	1,	&ChargeSta, 			NULL,				NULL,				};
	{0x0102, attr_R|attr_P,	1,	&EEPROMSta, 			NULL,				NULL,				};
	{0x0103, attr_R|attr_P,	1,	&RTSensorSta, 			NULL,				NULL,				};
//	{0x0104, attr_R|attr_P,	1,	NULL, 					readScreenSta,		NULL,				};
//	{0x0110, attr_R|attr_W,	1,	NULL, 					readLEDFlash,		writeLEDFlash,		};
//	{0x0111, attr_W1,		1,	NULL, 					NULL,				writeScreenAll,		};
//	{0x0112, attr_W1,		1,	NULL, 					NULL,				writeLEDBlinkOnce,	};
//	{0x0113, attr_W1,		1,	NULL, 					NULL,				writeScreenLightUp,	};
//	{0x0114, attr_W1,		1,	NULL, 					NULL,				writeBeepOnce,		};
	{0x0115, attr_W1,		1,	&EEPROMInit, 			NULL,				writeEEPROMInit		};
	{0x0116, attr_W1,		1,	&EEPROMCheck, 			NULL,				writeEEPROMCheck	};
//	{0x0120, attr_R|attr_P,	1,	NULL, 					readProbeStatus,	NULL,				};
//	{0x0121, attr_R,		1,	NULL, 					readProbeDataUpdata,NULL,				};
//	{0x0122, attr_R,		6,	NULL, 					readProbeMac,		NULL,				};
//	{0x0123, attr_R,		1,	NULL, 					readProbeType,		NULL,				};
//	{0x0124, attr_R,		12,	NULL, 					readProbeBatchNO,	NULL,				};
//	{0x0125, attr_R,		52,	NULL, 					readProbeRTTable,	NULL,				};
//	{0x0126, attr_R,		31, NULL, 					readProbeCali,		NULL,				};
	{0x0140, attr_R1,		2,	(uint8_t *)&CaliObj1,	readCaliObj1,		NULL,				};
	{0x0141, attr_R|attr_W1,5,  (uint8_t *)&CaliObj1Param, 	NULL,			writeCoreEEPROM,	};
	{0x0150, attr_W,		1,	&DegradeLinkFreq, 		NULL,				writeDegradeLinkFreq,};
	{0x0151, attr_W,		1,	&ClearHistoryData, 		NULL,				writeClearHistoryData,};
	{0x0152, attr_W,		1,	&DeviceClose, 			NULL,				writeDeviceClose,	};
	{0x0200, attr_R,		2,	&HistoryTotal, 			readHistoryTotal,	NULL,				};
	{0x0201, attr_R,		2,	&HistoryLenth, 			NULL,				NULL,				};
	{0x0202, attr_R,		1,	&HisDecodeType,		 	NULL,				NULL,				};
	{0x0203, attr_R|attr_W,	8,	UserID, 				NULL,				NULL,				};
	{0x0204, attr_W,		2,	&HistoryDataRsq,		NULL,				writeHistoryData,	};
	{0x0205, attr_P,		180,HistoryData, 			getHistoryData,		NULL,				};
	{0x0210, attr_R,		1,	&RealTimeDataDecoder, 	NULL,				NULL,				};
	{0x0211, attr_P,		10,	RealTimeData, 			NULL,				NULL,				};
//	{0x0220, attr_W,		0,	NULL, 					NULL,				writeFirmwareBlock, };
//	{0x0221, attr_P,		2,	NULL, 					getBlockConfirmNO,	NULL,				};
//	{0x0222, attr_W|attr_P,	2,  NULL, 					readFirmwareTotal,	writeFirmwareTotal,	};
}


/*********************** external variable definition *************************/
//todo：添加一个总的参数初始化函数
//todo:修改EEPROM驱动，移植MH70N4驱动
uint8_t MACAddr[6] = {0,0,0,0,0,0,};
uint8_t deviceName[18] = {0,};//todo
uint8_t deviceType = 0;
uint8_t deviceBatch[15] = {0,};
uint8_t hardwareVer[4] = {0,};
uint8_t softwareVer[4] = {0,};
uint8_t protocolVer[4] = {0,};
uint8_t lockStatus[4] = {0};
uint8_t battValue = 0;
uint8_t ChargeSta = FALSE;
uint8_t EEPROMSta = 0;
uint8_t RTSensorSta = 0;
uint8_t EEPROMInit = 0;
uint8_t EEPROMCheck = 0;
uint16_t CaliObj1 = 0;
TYP_caliPoint CaliObj1Param = {0,};
uint8_t DegradeLinkFreq = 0;
uint8_t ClearHistoryData = 0;
uint8_t DeviceClose = 0;
uint16_t HistoryTotal = 0;
uint8_t HistoryLenth = 10;
uint8_t HisDecodeType = 0;
uint8_t UserID[8] = {0,};
uint16_t HistoryDataRsq = 0;
uint8_t HistoryData[180] ={0};
uint8_t RealTimeDataDecoder = 0;
uint8_t RealTimeData[10] = {0};
uint8_t lockFlag = FALSE;

/*********************** internal/static function declare *********************/
static void writeCoreEEPROM(TYP_ProtocolItem *ptrItem);
static void readTime(TYP_ProtocolItem *ptrItem);
static void writeTime(TYP_ProtocolItem *ptrItem);
static void writeName(TYP_ProtocolItem *ptrItem);
static void writeLock(TYP_ProtocolItem *ptrItem);
static void writeEEPROMInit(TYP_ProtocolItem *ptrItem);
static void writeEEPROMCheck(TYP_ProtocolItem *ptrItem);
static void writeDegradeLinkFreq(TYP_ProtocolItem *ptrItem);
static void writeClearHistoryData(TYP_ProtocolItem *ptrItem);
static void writeDeviceClose(TYP_ProtocolItem *ptrItem);
static void readHistoryTotal(TYP_ProtocolItem *ptrItem);
static void writeHistoryData(TYP_ProtocolItem *ptrItem);
static void getHistoryData(TYP_ProtocolItem *ptrItem);

/*********************** external function definition *************************/

/*******************************************************************************
	Function:		// 函数名称
	Input:			// 输入参数说明，包括每个参数的作
					// 用、取值说明及参数间关系。
	Output:			// 对输出参数的说明。指针和引用
	Return:			// 函数返回值的说明
	Description:	// 函数功能、性能等的描述
*******************************************************************************/




/*********************** internal/static function definition ******************/

/*******************************************************************************
	Function:		// 函数名称
	Input:			// 输入参数说明，包括每个参数的作
					// 用、取值说明及参数间关系。
	Output:			// 对输出参数的说明。指针和引用
	Return:			// 函数返回值的说明
	Description:	// 函数功能、性能等的描述
*******************************************************************************/
void writeCoreEEPROM(TYP_ProtocolItem *ptrItem)
{
	switch(ptrItem->ID)
	{
		case ID_deviceType:
			//todo
			break;
		case ID_deviceBatch:
			break;
		case ID_hardwareVer:
			break;
		case ID_softwareVer:
			break;
		case ID_protocolVer:
			break;
		case CaliObj1Param:
			break;
	}
}

void readTime(TYP_ProtocolItem *ptrItem)
{
	UTCTime time;
	time = osal_getClock();
	osal_memcpy(ptrItem->ptrValue, (uint8_t *)&time, ptrItem->valueLenth);
}

void writeTime(TYP_ProtocolItem *ptrItem)
{
	UTCTimeStruct time;
	UTCTime secTime;

	secTime = *((UTCTime *)ptrItem->ptrValue);

	osal_ConvertUTCTime(&time, secTime);
	if ((time.year >= timeCheckLowerLimit) && (time.year <= timeCheckHigerLimit))
	{
		osal_setClock(secTime);
		timeSetted = TRUE; //时间同步完成 
	}	
}

void writeName(TYP_ProtocolItem *ptrItem)
{
	uint8_t newName[GAP_DEVICE_NAME_LEN];

	osal_memset(newName, 0, GAP_DEVICE_NAME_LEN);
	osal_memcpy(newName, ptrItem->ptrValue, MIN(ptrItem->valueLenth,GAP_DEVICE_NAME_LEN));
	//todo:保存到EEPROM

	GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, newName);
	Adv_SetDevName(newName);
	Adv_ValidateData();	
}

void writeLock(TYP_ProtocolItem *ptrItem)
{
	uint8_t cmpResult;
	cmpResult = osal_memcmp(ptrItem->ptrValue, "UNLK", ptrItem->valueLenth);
	//todo：检查osal_memcmp的返回值，看TRUE是否可能
	if(cmpResult != TRUE)
	{
		osal_memcpy(ptrItem->ptrValue, "LOCK", ptrItem->valueLenth);
		lockFlag = TRUE;
	}
	else
	{
		lockFlag = FALSE;
	}
}


