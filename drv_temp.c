
#include <stdio.h>

#include "util.h"
#include "comdef.h"
#include "hal_adc.h"
#include "drv_temp.h"

/*********************** constant *********************************************/
#define TEMP_ADC_VREF				HAL_ADC_REF_AVDD
#define TEMP_ADC_CHN				HAL_ADC_CHANNEL_3
#define TEMP_ADC_RESOLUTION			HAL_ADC_RESOLUTION_12

#define TEMP_AD_TIMES				4

/*********************** macro ************************************************/

/*********************** data type definition *********************************/
typedef struct {
	uint16 resis;
	uint16 temp;	
}RTTable_t;

/*********************** external variable definition *************************/

/*********************** internal/static variable definition & declare ********/
//RT表中温度值做1000倍处理
const static RTTable_t RTTable[]=
{
{,25000};
{,26000};
{,27000};
{,28000};
{,29000};
{,30000};
{,31000};
{,32000};
{,33000};
{,34000};
{,35000};
{,35500};
{,36000};
{,36500};
{,37000};
{,37500};
{,38000};
{,38500};
{,39000};
{,39500};
{,40000};
{,41000};
{,42000};
{,43000};
{,44000};
{,45000};
};



static void prepareTempConversion(void)
{
  ADCCFG |= METER_AIN_BV;
  HAL_OPA_POW_ON();
  delay_nms(5);
}

static int16 performTempConversion(void)
{
  int16 val = 0;

  ADCCON3 = TEMP_ADC_CHN | TEMP_ADC_RESOLUTION | TEMP_ADC_VREF;
  while (!(ADCCON1 & 0x80));

  val = (int16)(ADCL);
  val |= (int16)(ADCH << 8);
  
  if (val < 0)
  {
	val = 0;
  }
  
  val >>= 4;
  
  return val;
}

static void finalizeTempConversion(void)
{
  HAL_OPA_POW_OFF();
  ADCCFG &= ~METER_AIN_BV;
}

//计算 u*v/z
static uint32 mul_mod(uint32 u, uint32 v, uint32 z)
{
	// 进行长乘法(结果为64位)
	uint32 u0, v0, w0, x, y;
	uint32 u1, v1, w1, w2, t;
	uint8 i;
	
	u0 = u & 0xFFFF;
	u1 = u >> 16;
	
	v0 = v & 0xFFFF;
	v1 = v >> 16;
	
	w0 = u0*v0;
	t  = u1*v0 + (w0 >> 16);
	w1 = t & 0xFFFF;
	w2 = t >> 16;
	w1 = u0*v1 + w1;

	// x为高32位, y为低32位
	x = u1*v1 + w2 + (w1 >> 16);
	y = u*v;
	

	// 进行长除法(被除数为64位)		 
	for (i = 0; i < 32; i++)
	{
		t = (int32)x >> 31;           // All 1's if x(31) = 1. 

		x = (x << 1) | (y >> 31);   // Shift x || y left 
		y <<= 1;                    // one bit.
		
		if((x|t) >= z) 
		{ 
			x -= z;
			y++; 
		} 		
	}

	return y;    // y为商, x为余数
}

//公式：（43000100+1298330220*AD）/(540970925-49305*AD)
static uint16_t convertADToRes( uint16_t ADValue)
{
	uint32_t divident;//被除数
	uint32_t divisor; //除数
	uint32_t res;
  
	divident = 430001*(uint32_t)ADValue;
	divident += 1298330220;
  
	divisor = 540970925;
	divisor -= 49305*(uint32_t)ADValue;
  
	res = mul_mod(divident, 10000, divisor);

	return (uint16_t)res;
}




uint16 convertTemp(void)
{

  uint16 val = 0;
  uint16 res;

  prepareTempConversion();

  //one or more times
  for (uint8 i = 0; i < TEMP_AD_TIMES; i ++)
  {
	val += performTempConversion();
	
	delay_100us(); 
  }

  finalizeTempConversion();

  val /= TEMP_AD_TIMES; 
	
  res = convertADToRes(val);
  
  return res;
}

uint16 modifyRawTemp(uint16 raw)
{ 
	int32 returnVal;
	int32 gap;

	returnVal = (int32)raw;

	gap = (int32)caliVal[0].measure - (int32)caliVal[0].real;
	returnVal -= gap;

	return (uint16)returnVal;
}

/*******************************************************************************
	Function:		tempCalc()
	Input:			uint16_t *RTTable.resis - 指向探头参数-标准电阻数组的指针
					uint8_t rstNum - 标准电阻数组元素个数
					int32_t ntcRst - NTC电阻值
	Output:			none
	Return:			int16_t - 温度值
	Description:	根据R-T表计算温度
*******************************************************************************/

static uint16_t tempCalc(uint16_t ntcRst)  
{  
    uint8_t indexLeft = 0;  
    uint8_t indexRight = 0;  
    uint8_t indexMid = 0;
	uint16_t temperature = 0;
	int32_t tmp32 = 0;
			
	indexRight = rstNum-1; 
	
	//NTC电阻值在标准电阻左侧（温度小于等于25度）
	if(ntcRst>=RTTable[0].resis)
	{	  
		return (RTTable[0].temp/10);	  
	}
	//NTC电阻值在标准电阻右侧（温度大于等于45度）
	else if(ntcRst<=RTTable[sizeof(RTTable)/sizeof(RTTable_t)-1].resis)
	{
		return (RTTable[sizeof(RTTable)/sizeof(RTTable_t)-1].temp/10)	  
	}
	else
	{      		
	  	while((indexRight-indexLeft)>1) 
		{ 			
			indexMid=indexLeft+(indexRight-indexLeft)/2;
			
			//NTC电阻与某个标准电阻相同
			if(ntcRst==RTTable[indexMid].resis)
			{
				return (RTTable[indexMid].temp/10);							  
			}
			//NTC电阻在RTTable.resis[indexMid]的右边  
			else if(ntcRst < RTTable[indexMid].resis)     
			{  
				indexLeft = indexMid;			    
			}
			//NTC电阻在RTTable.resis[indexMid]的左边 
			else
			{  
				indexRight = indexMid;  
			}
		}
	}
	
	//一度一段，根据RT表计算温度值
	tmp32 = (int32_t)(RTTable[indexRight].temp - RTTable[indexLeft].temp);
	tmp32 *= ntcRst - (int32_t)RTTable[indexLeft].resis;
	tmp32 /= (int32_t)(RTTable[indexRight].resis-RTTable[indexLeft].resis);
	temperature = (uint16_t)(RTTable[indexLeft].temp + tmp32);
	
	//当前温度缩放10倍
	temperature=(temperature+5)/10;
	
	return temperature;
}


void initTempDriver(void)
{
	METER_AIN_DDR &= ~METER_AIN_BV;
	METER_AIN_SEL |= METER_AIN_BV;
	OPA_POW_DDR |= OPA_POW_BV;

	HAL_OPA_POW_OFF();
}

