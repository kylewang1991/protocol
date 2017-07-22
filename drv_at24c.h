
#ifndef _DRV_AT24C_H_
#define _DRV_AT24C_H_

#include "comdef.h"

/*********************** constant *********************************************/
#define MAX_NAME_LEN            		20

#define OLD_SIZE      					sizeof(offlineData_t)

#define E2P_STATUS_ERROR_MASK 			0x03

/*********************** macro ************************************************/

/*********************** data type definition *********************************/
typedef struct {
  uint32 rtc;
  uint8 typeOfProbe;
  uint8 RTTableType;
  uint16 bodyTemp1;
  uint16 bodyTemp2;
}offlineData_t;

/*********************** external variable declare ****************************/
extern uint8 E2P_status;
extern uint8 deviceNameCore[MAX_NAME_LEN+1];
extern caliObj_t MainBoardCali;
extern uint8_t launchFlag;
extern uint8_t lockFlag;
extern uint8_t deviceType;
extern uint8_t deviceBatch[15];
extern uint8_t hardwareVer[4];
extern uint8_t softwareVer[4];
extern uint8_t protocolVer[4];
extern uint8_t lockStatus[4];

/*********************** external function declare ****************************/
extern void E2P_init(void);
extern uint8 E2P_check(void);
extern void E2P_SetDefault(void);

extern uint8 E2P_clearOLD(void);
extern uint16 E2P_getOLDSum(void);

extern uint8 E2P_storeOLD(offlineData_t * pOLD);
extern uint16 E2P_loadOLD(uint16 seq, uint8* pBuf, uint16 bufLen);

#endif
