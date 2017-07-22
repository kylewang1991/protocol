
#ifndef __DRV_TEMP_H__
#define __DRV_TEMP_H__

#include "hal_types.h"

/*********************** constant *********************************************/

/*********************** macro ************************************************/

/*********************** data type definition *********************************/

/*********************** external variable declare ****************************/

/*********************** external function declare ****************************/
extern void initTempDriver(void);
extern uint16 convertTemp(void);
extern uint16 modifyRawTemp(uint16 raw);


#endif /*__DRV_TEMP_H__*/

