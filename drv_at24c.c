
#include <stdio.h>

#include "util.h"
#include "OSAL.h"
#include "hal_i2c.h"
#include "hal_mcu.h"
#include "OnBoard.h"
#include "drv_temp.h"
#include "drv_at24c.h"
#include "hal_board.h"
#include "devinfocfg.h"
/*********************** constant *********************************************/
//EEPROM(E2P)
//器件的IIC地址
#define E2P_ADDR					0x50 
//单次读写如果不成功，要重试的次数
#define E2P_RETRY_NUM				75
//检查eeprom时读取的数据长度
#define E2P_CHECK_SIZE				5 
//eeprom页长度
#define E2P_PAGE_SIZE				128
//单次读写操作是否需要切页，
//当页长度是离线数据结构体长度的整数倍时该值为假
//否则为真
#define E2P_PAGE_SWITCH				TRUE				

// 离线数据Off line data defintion(OLD)
#define OLD_EMPTY					0xFE
#define OLD_INVALID_ADDR			0x0000
#define OLD_START_ADDR				0x0080
#define OLD_END_ADDR				0xffff

//程序加载方式
#define OAD_LAUNCH       0xD5
#define EVER_LAUCH       0x7E

//EEPROM错误标志
//读写失败，发器件地址无响应，写入后无ACK等总线错误
#define E2P_RW_ERROR  				(1 << 0)				
//EEPROM未初始化,读取出magic数值和程序中的默认值不相同
#define E2P_STORAGE_INVALID  		(1 << 1)

//EEPROM第一页存储数据
//数据信息在列表中的位置
#define INDEX_MAGIC					0
#define INDEX_NAME					1
#define INDEX_CALIDATA				2
#define INDEX_LAUNCHFLAG			3
#define INDEX_OLDPTR				4
#define INDEX_LOCKFLAG				5
#define INDEX_DEVICETYPE			6
#define INDEX_DEVICEBATCH			7
#define INDEX_HARDVER				8
#define INDEX_SOFTVER				9
#define INDEX_PROTOCOLVER			10

//设备名称
//#define NAME_ADDR 		       		0x0022
//校准数据calibration(CALI)
//#define CALI_1_ADDR 		        0x0040 
//#define CALI_2_ADDR 		        0x0048
//离线数据指针offline data pointer(OLDP)
//#define OLDP_ADDR        			0x0050//离线数据指针存放地址
#define OLDP_DEF_VAL				OLD_START_ADDR
//魔术值magic magic(MAGC)
//#define MAGC_ADDR                 	0x0078
//#define MAGC_VAL_LEN            	4
#define MAGC_VAL_OLD                0x53B829C6//0x53B829CC	
#define MAGC_VAL_NEW                0x01010101
#define CALI_DEF_VAL				0X0000
#define LAUNCH_FLAG_DEF				EVER_LAUCH






/*********************** macro ************************************************/
#define PAGE_INDEX(addr)	        ((addr) >> 7)
#define PAGE_OFFSET(addr)           ((addr) & 0x007F)

/*********************** data type definition *********************************/
typedef struct {
	uint16 head;
	uint16 tail;
}offlinedataPtr_t;

typedef struct {
	uint16 addr;
	uint8 lenth;
	uint8* ptrValue;
}deviceInfo_t;

typedef struct {
	uint8 NUM_caliPoint;
	uint16 realValue;
	uint16 measureValue;
}caliObj_t;

typedef uint8 (*ptr_func)(uint8, uint8 *, uint8, uint8 *);

/*********************** external variable definition *************************/
const deviceInfo_t deviceInfo[]=
{
	{0x0000, 4,					(uint8_t*)&MAGC_VAL_Read	};//magic
	{0x0004, MAX_NAME_LEN,		deviceNameCore				};//name
	{0x0018, sizeof(caliObj_t),	(uint8_t*)&MainBoardCali	};//cali
	{0x0028, 1,					&launchFlag 				};//launch
	{0x002A, 4,					(uint8_t*)&offlinedataPtr 	};//offlinedataPtr
	{0x002C, 1,					&lockFlag 					};//lock
	{0x002E, 1,					&deviceType					};//deciveType
	{0x0030, 15,				deviceBatch					};//deviceBatch
	{0x0040, 4,					hardwareVer 				};//hardwareVersion
	{0x0044, 4,					softwareVer 				};//softwareVersion
	{0x0048, 4,					protocolVer 				};//protocolVersion
}

uint8 E2P_status = 0;

uint8 deviceNameCore[MAX_NAME_LEN+1] = 
{
	0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0,};//名称的默认值在AT24C初始化函数中给予。

caliObj_t MainBoardCali = 
{
	0,
	CALI_DEF_VAL,
	CALI_DEF_VAL,
}

uint8_t launchFlag = LAUNCH_FLAG_DEF;
uint8_t lockFlag = FALSE;
uint8_t lockStatus[4] = "UNLK";
uint8_t deviceType = 0;
uint8_t deviceBatch[15] = {0,};
uint8_t hardwareVer[4] = {0,};
uint8_t softwareVer[4] = {0,};
uint8_t protocolVer[4] = {0,};
/*********************** internal/static variable definition & declare ********/
static uint32_t MAGC_VAL_Read = MAGC_VAL_NEW;

static offlinedataPtr_t offlinedataPtr = 
{
	OLDP_DEF_VAL, 
	OLDP_DEF_VAL
};

static uint16 offlinedataPtrSync = OLD_INVALID_ADDR;
const static uint8 nameDefaultValue[] = STR(DEVICE_NAME);

/*********************** external variable definition *************************/

/*********************** internal/static function declare *********************/
static uint16 OLDNextAddr(uint16 addr);
static uint8 At24cRead(deviceInfo_t* ptrDevInfo);
static uint8 At24cWrite(deviceInfo_t* ptrDevInfo);
static uint8 At24cReadWrite(ptr_func func, deviceInfo_t* ptrDevInfo);
static uint8 At24cRWExecute(ptr_func func, deviceInfo_t* ptrDevInfo);
static uint8 checkBlock(uint16 addr);
static uint8 OLDSkipStmt(void);
static uint8 OLDStepStmt(uint8* pOLD);
//static uint8 storeOfflineDataPtr(void);
//static void checkSetName(uint8 *pName, uint8 len);
static void SetBuffToDefault(void);
/*********************** external function definition *************************/
/*******************************************************************************
	Function:		E2P_init
	Input:			
	Output:			
	Return:			
	Description:	开机初始化eeprom中存储的参数，即：将eeprom中内容读入缓存
*******************************************************************************/
void E2P_init(void)
{
	uint8 errCode;
	uint8 i = 0;

	SetBuffToDefault();

	delay_nms(50);

	while(i<10)
	{
		errCode = At24cRead(deviceInfo+INDEX_MAGIC);
		if(errCode == SUCCESS)
			break;
		i++;		
	}

	if(errCode == SUCCESS)
	{
		if (MAGC_VAL != magic)
		{
			E2P_status |= E2P_STORAGE_INVALID;
		}
		else//
		{
			for(i=0, i<(sizeof(deviceInfo)/sizeof(deviceInfo_t)), i++)
				At24cRead(deviceInfo + i);

			if(lockFlag == TRUE)
				osalmemcpy(lockStatus, "LOCK", 4);
			else
				osalmemcpy(lockStatus, "UNLK", 4);
		}
	} 	
}

/*******************************************************************************
	Function:		E2P_check
	Input:			
	Output:			
	Return:			FAILURE：检查失败，SUCCESS：检查成功	
	Description:	检查AT24是否存在损坏
*******************************************************************************/
uint8 E2P_check(void)
{
	//注意三个地址分别对应EEPROM的低中高三个部分，
	//地址已页开始地址为好
	//如果不是页起始地址，
	//需要保证从改页开始都页结尾至少有E2P_CHECK_SIZE个字节的存储空间
	int16 addr[3] = {0X0000,0x6400,0XFF80};
	int8 i;

	for(i=0; i<3; i++)
	{
		if (checkBlock(addr[i]) != SUCCESS)
			return FAILURE;		
	}

	return SUCCESS;
}

/*******************************************************************************
	Function:		void E2P_SetDefault(void)
	Input:			
	Output:			
	Return:			
	Description:	将EEPROM恢复为默认值
*******************************************************************************/
void E2P_SetDefault(void)
{
	uint8 errorcode;
	uint8 i;	

	errorcode = 0;
	//设置缓存为默认值	
	SetBuffToDefault();

	//将缓存值写入EEPROM
	for(i=0, i<(sizeof(deviceInfo)/sizeof(deviceInfo_t)), i++)
		errorcode |= At24cWrite(deviceInfo + i);

	if(errorcode == 0x00)
		E2P_status &= (~E2P_STORAGE_INVALID); 
}

/*******************************************************************************
	Function:		E2P_clearOLD
	Input:			
	Output:			
	Return:			成功与否，success成功 failure失败
	Description:	清空离线数据，首尾指针归零
*******************************************************************************/
uint8 E2P_clearOLD(void)
{
	uint8 errCode;
	offlinedataPtr.head = OLD_START_ADDR;
	offlinedataPtr.tail = OLD_START_ADDR;
	errCode = At24cWrite(deviceInfo + INDEX_OLDPTR);

	return errCode;
}

/*******************************************************************************
	Function:		E2P_getOLDSum
	Input:			
	Output:			
	Return:			离线数据的总量(单位:条)
	Description:	计算存储的温度记录的条数
*******************************************************************************/
uint16 E2P_getOLDSum(void)
{
	uint32 ReturnValue;

	if(offlinedataPtr.head >= offlinedataPtr.tail)
	{
		ReturnValue = offlinedataPtr.head - offlinedataPtr.tail;
	}
	else
	{
		ReturnValue = offlinedataPtr.head;
		ReturnValue+= ((OLD_END_ADDR - OLD_START_ADDR) + 1);
		ReturnValue-= offlinedataPtr.tail; 
	}
	ReturnValue /= OLD_SIZE;

	return ReturnValue;
}

/*******************************************************************************
	Function:		E2P_storeOLD
	Input:			pTemp 被存入的历史数据存放地址
	Output:			
	Return:			成功与否，success成功 failure失败
	Description:	插入一条历史记录
*******************************************************************************/
uint8 E2P_storeOLD(offlineData_t * pOLD)
{
	uint8 errCode = SUCCESS;
	deviceInfo_t devInfo;

	devInfo.addr = offlinedataPtr.head;
	devInfo.lenth = OLD_SIZE;
	devInfo.ptrValue = (uint8*)pOLD;

	errCode = At24cWrite(&devInfo);
	if (SUCCESS != errCode)
	{
		return errCode;
	}

	//更新地址
	offlinedataPtr.head = OLDNextAddr(offlinedataPtr.head);
	if (offlinedataPtr.head == offlinedataPtr.tail)
	{
		offlinedataPtr.tail = OLDNextAddr(offlinedataPtr.tail);
	}

	//保存当前的队列头尾地址
	errCode = At24cWrite(deviceInfo + INDEX_OLDPTR);

	if (SUCCESS != errCode)
	{
		return errCode;
	}

	return SUCCESS;
}

/*******************************************************************************
	Function:		E2P_loadOLD
	Input:			seq:要读取的块序号 
					pbuf:读出数据的存放地址，
					bufLen：期望读取的数据长度
	Output:			
	Return:			实际读取到的字节长度
	Description:	读取若干条离线数据
*******************************************************************************/
uint16 E2P_loadOLD(uint16 seq, uint8* pBuf, uint16 bufLen)
{
	uint16 index = 0;
	uint8 errCode = SUCCESS;
	static recordSeq = 0;

	if (0 == seq)
		offlinedataPtrSync = offlinedataPtr.tail;//初始化离线数据读取
	else if(recordSeq + 1 == seq)
	{
		OLDSkipStmt();
		recordSeq = seq;
	}
	else if(recordSeq == seq)
		return bufLen;
	else
	{
		osal_memset(pBuf,0xff,OLD_SIZE);
		offlinedataPtrSync = OLD_INVALID_ADDR;
		recordSeq = 0;
		return OLD_SIZE;		
	}

	while ((index + OLD_SIZE) <= bufLen)
	{
		errCode = OLDStepStmt(&pBuf[index]);
		if (SUCCESS != errCode)
			break;
		index += OLD_SIZE;
	}

	if((OLD_EMPTY == errCode)&&(index < bufLen)&&(index != 0))
	{
		osal_memset(&pBuf[index],0xff,OLD_SIZE);
		index += OLD_SIZE;
	}


	if ((SUCCESS != errCode) && ((OLD_EMPTY != errCode) || (0 == index)))
	{
		osal_memset(pBuf,0xff,OLD_SIZE);
		index = OLD_SIZE;
		offlinedataPtrSync = OLD_INVALID_ADDR;//结束历史数据读取
		recordSeq = 0
	}

	return index;
}

void writeCoreEEPROM(TYP_ProtocolItem *ptrItem)
{
	uint8 deviceInfoListIndex;
	
	switch(ptrItem->ID)
	{
		case ID_deviceName:
			deviceInfoListIndex = INDEX_NAME;
			break;
		case ID_CaliObj1Param:
			deviceInfoListIndex = INDEX_CALIDATA;		
			break;
		case ID_lockStatus:
			deviceInfoListIndex = INDEX_LOCKFLAG;
			break;						
		case ID_deviceType:
			deviceInfoListIndex = INDEX_DEVICETYPE;
			break;
		case ID_deviceBatch:
			deviceInfoListIndex = INDEX_DEVICEBATCH;
			break;
		case ID_hardwareVer:
			deviceInfoListIndex = INDEX_HARDVER;
			break;
		case ID_softwareVer:
			deviceInfoListIndex = INDEX_SOFTVER;
			break;
		case ID_protocolVer:
			deviceInfoListIndex = INDEX_PROTOCOLVER;
			break;
	}
	At24cWrite(deviceInfo + IdeviceInfoListIndex);
}

// /*******************************************************************************
// 	Function:		E2P_storeName
// 	Input:			name 要存入的名字的地址
// 	Output:			
// 	Return:			FAILURE：失败，SUCCESS：成功	
// 	Description:	存储设备名
// *******************************************************************************/
// uint8 E2P_storeName(uint8 *pName, uint8 len)
// {
// 	uint8 errCode; 
// 	errCode = At24cWrite(NAME_ADDR, deviceNameCore, MAX_NAME_LEN);
// 	return errCode;
// }

// /*******************************************************************************
// 	Function:		E2P_loadName
// 	Input:			name 读出设备名的存放地址
// 	Output:			
// 	Return:			FAILURE：失败，SUCCESS：成功	
// 	Description:	读取设备名
// *******************************************************************************/
// void E2P_loadName(uint8* pName, uint8 len)
// {
// 	osal_memcpy(pName, deviceNameCore, MIN(sizeof(deviceNameCore), len));
// }


// /*******************************************************************************
// 	Function:		E2P_storeCali
// 	Input:			
// 	Output:			
// 	Return:				
// 	Description:	存入温度校准值
// *******************************************************************************/
// void E2P_storeCali(void) 
// {
// 	At24cWrite(CALI_1_ADDR, (uint8*)&caliVal[0], LEN_CALI_VALUE);
// 	At24cWrite(CALI_2_ADDR, (uint8*)&caliVal[1], LEN_CALI_VALUE);	
// }

// void E2P_storeLaunchFlag(uint8 flag)
// {
// 	At24cWrite(LAUNCH_ADDR, &flag, sizeof(uint8));
// }


// void E2P_loadLaunchFlag(uint8 * flag)
// {
// 	At24cRead(LAUNCH_ADDR, flag, sizeof(uint8));
// }


/*********************** internal/static function definition ******************/
/*******************************************************************************
	Function:		At24cRWExecute
	Input:			func:读或者写的处理函数，
					RWdata：要读写的数据
	Output:			
	Return:			读写成功与否，success成功 failure失败
	Description:	读取或写入的底层执行函数
*******************************************************************************/
uint8 At24cRWExecute(ptr_func func, deviceInfo_t* ptrDevInfo)
{
	uint8 errCode;
	uint8 buf[2] = {HI_UINT16(ptrDevInfo->addr),LO_UINT16(ptrDevInfo->addr)};
	uint8 timeout = E2P_RETRY_NUM;

	while(timeout--)
	{
		errCode = func(sizeof(buf), buf, ptrDevInfo->lenth, ptrDevInfo->ptrValue);

		if(errCode == SUCCESS)
		{
			E2P_status &= (~E2P_RW_ERROR);
			return SUCCESS;
		}

		delay_100us();
	}

	E2P_status |= E2P_RW_ERROR;
	return FAILURE;
}

/*******************************************************************************
	Function:		At24cReadWrite
	Input:			func:读写函数
					rawRWData：要读写的数据
	Output:			
	Return:			读取成功与否，success成功 failure失败
	Description:	本函数判断读取或写入的数据长度，如果需要分页,就对实际需要读
					写的数据进行分割
*******************************************************************************/
uint8 At24cReadWrite(ptr_func func, deviceInfo_t* ptrDevInfo)
{
	uint8 errCode = SUCCESS;

	HalI2CInit(E2P_ADDR, i2cClock_533KHZ);	

#if (E2P_PAGE_SWITCH==TRUE)
	uint16 ind = PAGE_INDEX(ptrDevInfo->addr);
	uint8 ofs = PAGE_OFFSET(ptrDevInfo->addr);
	uint8 lenInCurPage = E2P_PAGE_SIZE - ofs;;	
	uint8 totalLenth = ptrDevInfo->lenth;

	if ((lenInCurPage < totalLenth)&&(ind > 0))
	{
		ptrDevInfo->lenth = lenInCurPage;
		errCode = At24cRWExecute(func, ptrDevInfo);

		ptrDevInfo->addr += lenInCurPage;
		ptrDevInfo->lenth = totalLenth - lenInCurPage;
		ptrDevInfo->ptrValue += lenInCurPage;
		if(errCode != SUCCESS)
		{
			HalI2CDisable();
			return errCode;			
		}

		errCode = At24cRWExecute(func, ptrDevInfo);		
	}
	else
	{
		errCode = At24cRWExecute(func, ptrDevInfo);
	}
#else
	errCode = At24cRWExecute(func, ptrDevInfo);	
#endif

	HalI2CDisable();
	return errCode;
}

/*******************************************************************************
	Function:		At24cRead
	Input:			addr 待读取数据在EEPROM上的地址
					pbuf 读出数据的存放地址
					len  读出的数据长度
	Output:			
	Return:			读取成功与否，success成功 failure失败
	Description:	读取eeprom上的一段数据，如果待读取的数据长度大于此页剩余的可用
					空间，本函数会自动切页，在下一页开始读取。注意，本函数无法从最
					后一页切到第0页，所以传入地址要做限制。
*******************************************************************************/
uint8 At24cRead(deviceInfo_t* ptrDevInfo ) 
{
	uint8 errCode;
	errCode = At24cReadWrite(HalI2CWriteRead, ptrDevInfo);
	return errCode;
}

/*******************************************************************************
	Function:		At24cWrite
	Input:			addr 待写入数据在EEPROM上的地址
					pbuf 写入数据的存放地址
					len  写入的数据长度
	Output:			
	Return:			写入成功与否，success成功 failure失败
	Description:	向eeprom写入一段数据，如果待写入的数据长度大于此页剩余的可用
					空间，本函数会自动切页，在下一页开始写入。注意，本函数无法从
					最后一页切到第0页，所以传入地址要做限制。
*******************************************************************************/
uint8 At24cWrite(deviceInfo_t* ptrDevInfo )
{
	uint8 errCode;
	errCode = At24cReadWrite(HalI2CWriteEx, ptrDevInfo);
	return errCode;
}

/*******************************************************************************
	Function:		initat24Storage
	Input:			
	Output:			
	Return:			
	Description:	恢复缓存为默认值
*******************************************************************************/
void SetBuffToDefault(void)
{
	//回复默认值
	MAGC_VAL_Read = MAGC_VAL_NEW;

	//恢复设备名为默认值	
	osal_memset(deviceNameCore, 0, sizeof(deviceNameCore));   
	osal_memcpy(deviceNameCore, nameDefaultValue, \
				MIN(sizeof(deviceNameCore) - 1, sizeof(nameDefaultValue) - 1));
	//恢复校准值为默认值	
	MainBoardCali.NUM_caliPoint = 0;
	MainBoardCali.realValue = CALI_DEF_VAL;
	MainBoardCali.measureValue = CALI_DEF_VAL;

	launchFlag = LAUNCH_FLAG_DEF;
	lockFlag = FALSE;
	osalmemcpy(lockStatus, "UNLK", 4);
	deviceType = 0;
	osal_memset(deviceBatch, 0, 15);
	osal_memset(hardwareVer, 0, 4);
	osal_memset(softwareVer, 0, 4);
	osal_memset(protocolVer, 0, 4);		
	//恢复离线数据指针为默认值
	offlinedataPtr.head = OLD_START_ADDR;
	offlinedataPtr.tail = OLD_START_ADDR;	
}

/*******************************************************************************
	Function:		OLDSkipStmt
	Input:			
	Output:			
	Return:			成功与否，success成功 failure失败
	Description:	尾指针前移，一个数据被读走后前移尾指针
*******************************************************************************/
uint8 OLDSkipStmt(void)
{
	if (OLD_INVALID_ADDR == offlinedataPtrSync)
	{
		return FAILURE;
	}

	offlinedataPtr.tail = offlinedataPtrSync;

	At24cWrite(deviceInfo + INDEX_OLDPTR);

	return SUCCESS;
}

/*******************************************************************************
	Function:		OLDStepStmt
	Input:			读出数据的存放地址
	Output:			
	Return:			成功与否，success成功 failure失败
	Description:	读走尾指针所指向的数据
*******************************************************************************/
uint8 OLDStepStmt(uint8* pOLD)
{
	uint8 errCode = SUCCESS;
	deviceInfo_t devInfo;

	if (OLD_INVALID_ADDR == offlinedataPtrSync)
	{
		return FAILURE;
	}

	if (offlinedataPtrSync == offlinedataPtr.head)
	{
		return OLD_EMPTY;
	}

	devInfo.addr = offlinedataPtrSync;
	devInfo.lenth = OLD_SIZE;
	devInfo.ptrValue = (uint8*)pOLD;
	errCode = At24cRead(&devInfo);
	if (SUCCESS != errCode)
	{
		return errCode;
	}
	offlinedataPtrSync = OLDNextAddr(offlinedataPtrSync);

	return SUCCESS;
}

/*******************************************************************************
	Function:		OLDNextAddr
	Input:			addr:当前地址
	Output:			
	Return:			下一条记录的存放地址	
	Description:	得到下一条记录的存储地址
*******************************************************************************/
uint16 OLDNextAddr(uint16 curAddr)
{
	uint16 nextaddr;

	// 判断是否已经时最后一个有效地址，否则指针移至头部
	if( curAddr >= ( ( OLD_END_ADDR - OLD_SIZE ) + 1 ) )
	{
		nextaddr = OLD_START_ADDR;
	}
	else
	{
		nextaddr = curAddr + OLD_SIZE;
	}
	
	return nextaddr;
}

/*******************************************************************************
	Function:		checkAt24c
	Input:			
	Output:			
	Return:			FAILURE：检查失败，SUCCESS：检查成功	
	Description:	将整个EEPROM分为64块，检查每一块 是否能成功读取
*******************************************************************************/
uint8 checkBlock(uint16 addr)
{
	uint8 bkpBuf[E2P_CHECK_SIZE]; 
	uint8 rdBuf[E2P_CHECK_SIZE];
	uint8 wrBuf[E2P_CHECK_SIZE];
	deviceInfo_t devInfo;

	osal_memcpy(wrBuf, "01234", E2P_CHECK_SIZE);

	devInfo.addr = addr;
	devInfo.lenth = E2P_CHECK_SIZE;

	devInfo.ptrValue = bkpBuf;
	if (SUCCESS != At24cRead(&devInfo))
	{
		return FAILURE;
	}

	devInfo.ptrValue = wrBuf;
	if (SUCCESS != At24cWrite(&devInfo))
	{
		return FAILURE;
	}

	devInfo.ptrValue = rdBuf;
	if (SUCCESS != At24cRead(&devInfo))
	{
		return FAILURE;
	}

	devInfo.ptrValue = bkpBuf;
	if (SUCCESS != At24cWrite(&devInfo))
	{
		return FAILURE;
	}

	if (!osal_memcmp(wrBuf, rdBuf, E2P_CHECK_SIZE))
	{
		return FAILURE;
	}

	return SUCCESS;
}

/*******************************************************************************
	Function:		checkSetName
	Input:			pName 指向待检查设备名的指针
					len 待检查设备名的长度
	Output:			
	Return:			
	Description:	检查从外部传入的设备名称，如果传入的设备名为空，则恢复为默认
					值；如不为空，则设为新的设备名
*******************************************************************************/
// void checkSetName(uint8 *pName, uint8 len)
// {
// 	osal_memset(deviceNameCore, 0, sizeof(deviceNameCore));   
// 	if((len == 0)||(pName == NULL))
// 	{
// 		osal_memcpy(deviceNameCore, nameDefaultValue, \
// 				MIN(sizeof(deviceNameCore) - 1, sizeof(nameDefaultValue) - 1));
// 	}
// 	else
// 	{
// 		osal_memcpy(deviceNameCore, pName, MIN(sizeof(deviceNameCore) - 1, len));
// 	}	
// }

// ******************************************************************************
// 	Function:		storeOfflineDataPtr
// 	Input:			
// 	Output:			
// 	Return:			
// 	Description:	存储离线数据指针
// ******************************************************************************
// uint8 storeOfflineDataPtr(void)
// {
// 	uint8 errCode;
// 	errCode = At24cWrite(deviceInfo + INDEX_OLDPTR);
// 	return errCode;
// }
