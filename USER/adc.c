/********************************************************************************
  * 文 件 名: adc.C
  * 版 本 号: 初版 v0.0 
  * 负责人 ：刘鑫旭          
*******************************************************************************/

/********************************* 头文件****************************************/

#include "adc.h"
#include "timer.h"
#include "param.h"

 ADC_CB adcCB;


// ADC通道映射 定义了一个数组用于映射或指定ADC通道号
const uint8 adcChannelMapping[] = {1};


/*********************************************************************
*
* 注：描述电池电量，通用版本采用，每次需要向客户确定具体的电池放电曲线
*
**********************************************************************/
const uint32 batteryPercentTable[8][12] = 
{
	// 24V
	{
		29000,
		27700,		// ≥该值，100%
		27000,		// ≥该值，90%
		26300,		// ≥该值，80%
		25600,		// ≥该值，70%
		25200,		// ≥该值，60%
		25000,		// ≥该值，50%
		24500,		// ≥该值，40%
		24200,		// ≥该值，30%
		23800,		// ≥该值，20%
		23100,		// ≥该值，10%
		21000		// ≥该值，0%，＜该值为欠压
	},

	// 36V
	{
		40800,		// 电池原定义为41000
		39500,
		38500,
		37500,
		36500,
		36000,
		35600,
		35000,
		34500,
		34000,
		33000,
		31500
	},
   
	
	// 48V
	{
		53800,//100%	// 电池原定义为54000
		51400,//90%
		50100,//75%
		48800,//60%
		47500,//45%
		46800,//37%
		46300,//31%
		45500,//20%
		44900,//10%
		44200,//8%
		42900,//5%
		42000
	},
	
	// 52V
	{
		56000,		//100%  // 电池原定义为58800
		55000,		//90%
		53800,		//75%
		52300,		//60%
		50800,		//45%
		50000,		//37%
		49500,		//31%
		48800,		//20%
		48000,		//10%
		47000,		//8%
		46500,		//5%
		45500
	},

	//60V
	{
		63700,//100%	// 电池原定义为54000
		62860,//90%
		61600,//75%
		60660,//60%
		58420,//45%
		57810,//37%
		57280,//31%
		56480,//20%
		55680,//10%
		54000,//8%
		52800,//5%
		52000
	},
			
	// 72V
	{
		80000,//100%	// 电池原定义为80200
		78600,//90%
		75400,//75%
		72500,//60%
		69400,//45%
		68100,//37%
		67000,//31%
		64900,//20%
		63000,//10%
		62600,//8%
		62000,//5%
		60000
	},

};

const uint16 batteryPercent[12] = 
{
	10000,
	9000,
	7500,
	6000,
	4500,
	3692,
	3115,
	2000,
	1000,
	800,
	500,
	0
};


// 电流恒定在10A的时候放电表
// 电压档位放电表，需要根据实测数据填充，数值乘以3(由于由于消抖算法连续三个值不一样才行更新最新的一个值)才是真实时间
// 时间扩展50%
const uint16 batteryPercentTimeTable[6][12] = 
{
	// 24V
	{
		10,
		14,
		9,
		14,
		10,
		10,
		12,
		10,
		50,
		53,
		24,
		1
	},

	// 36V
	{
		10,
		14,
		9,
		14,
		10,
		10,
		12,
		10,
		50,
		53,
		24,
		1
	},

	// 48V
	{
		15,//100%--90%  //(3min * 60) / (10 * 3)
		21,
		14,
		21,
		15,
		15,
		18,
		15,
		75,
		78,
		36,
		1
	},
	
	// 52V
	{
		15,//100%--90%  //(3min * 60) / (10 * 3)
		21,
		14,
		21,
		15,
		15,
		18,
		15,
		75,
		78,
		36,
		1
	},

	// 60V
	{
		15,//100%--90%	//(3min * 60) / (10 * 3)
		21,
		14,
		21,
		15,
		15,
		18,
		15,
		75,
		78,
		36,
		1
	},
	
	// 72V
	{
		15,//100%--90%	//(3min * 60) / (10 * 3)
		21,
		14,
		21,
		15,
		15,
		18,
		15,
		75,
		78,
		36,
		1
	},

};


//计算每个电压等级对应的电量百分比门限的数量
#define BATTERY_PERCENT_GATE_SIZE		(sizeof(batteryPercentTable[0])/sizeof(batteryPercentTable[0][0]))
//计算从一个电池电量等级到下一个电量等级的百分比变化量
#define BATTERY_PERCENT_GRADE			(100.0f/(BATTERY_PERCENT_GATE_SIZE-1))

//*********************************/
// 内部函数申明
//*********************************/
// 硬件ADC0初始化
void ADC_HwInit(void);

// 初始化结构体
void ADC_InitStr(void);

// 向缓冲区中添加一个数据
void ADC_AddDataToBuff(ADC_CHANNEL_E channel, uint16 data);

// 设置读取请求回调函数，由定时器调用
void ADC_CALLBACK_SetScanRequest(uint32 param);

// 启动定时器任务，周期性地读取
void ADC_HwStart(void);

// 停止检测
void ADC_Stop(void);

// 设置电池连续显示变化值
void ADC_TortoiseProcess(void);

// 获取电池百分比
uint8 ADC_GetBatteryPercent(uint32 param);


//*********************************/
// 函数主体
//*********************************/

/*********************************************************************
* 函 数 名: ADC_BaseInit
* 函数入参: 无
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 初始化ADC 
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_BaseInit(void)
{
	// ADC硬件初始化
	ADC_HwInit();

	// 初始化结构体
	ADC_InitStr();

	// 开始扫描
	ADC_HwStart();
}

/*********************************************************************
* 函 数 名: ADC_Process
* 函数入参: 无
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 采样处理，在main中调用
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_Process(void)
{
	uint16 adcData;
	
	// 有检测请求时，执行一次转换
	if (adcCB.scanRequest)
	{
		// 上次转换未完成，退出，等待转换完成
		// 注意不是所有平台开发都需要判断ADC转换标志 根据情况决定是否需要判断
		if (!ADC_CONVERT_IS_BUSY())
		{
			return;
		}
		ADC_CONVERT_CLEAR_EOC();  //清除ADC转换完成标志
		
		// 上次转换结束，清除转换请求
		adcCB.scanRequest = FALSE;       

		// 将数据添加到对应的缓冲区中
		switch(adcCB.currentChannel)
		{
			// 电池电压通道
			case ADC_CHANNEL_BATTERY: 
				adcData = ADC_CONVERT_RESULT();
				ADC_AddDataToBuff(ADC_CHANNEL_BATTERY, adcData);
		

				// ADC不作为电量数据源时，不进行计算
				if (BATTERY_DATA_SRC_ADC != PARAM_GetBatteryDataSrc())
				{
					break;
				}
				
								// 计算检测实时电压
				adcCB.battery.realtimeVoltage = adcCB.battery.adcAverage;
				adcCB.battery.realtimeVoltage *= 28;
				adcCB.battery.realtimeVoltage *= 3300;
				adcCB.battery.realtimeVoltage /= 4096;
				adcCB.battery.realtimeVoltage += 300;

				// 将当前电压放入参数层
				PARAM_SetBatteryVoltage(adcCB.battery.realtimeVoltage);

				// 计算实时百分比
				adcCB.battery.realtimePercent = ADC_GetBatteryPercent(adcCB.battery.realtimeVoltage);

				// 电池容量的百分比显示平滑处理
				ADC_TortoiseProcess();
				break;
				
			// 非法通道
			default:
				break;
		}

		// 切换至下一个通道  可能不止一个采样通道 
		adcCB.currentChannel ++;
		adcCB.currentChannel %= ADC_CHANNEL_MAX;
			
/*********************************************************************
*注：调用我们的定义的宏获取结构体内的参数选择当前要转换的ADC通道
*	
* 假设有多个ADC通道需要被映射到硬件ADC模块的具体通道上。可以使用一个数组来存储这些映射关系
* 数组的每个元素对应一个索引  索引的值则是对应硬件ADC通道号
*	所以对某个硬件ADC通道启动采样，只需要传参相对应的索引即可选择采样通道
*********************************************************************/
		ADC_CONVERT_CHANNEL(adcChannelMapping[adcCB.currentChannel]);
		
		// 启动下一次转换 其实就是对ADC采样的使能
		ADC_CONVERT_START();
	}
}

/*********************************************************************
* 函 数 名: ADC_HwStart
* 函数入参: 无
* 函数出参: 无
* 返 回 值: 无 
* 功能描述: 启动NTC，即启动定时器任务，周期性采样 
* 注意：采样转换间隔由宏定义确定，需要根据具体情况判断间隔时间
*				如果采样通道多的话就要缩短转换通道的时间，如油门、刹把等时效性比较强的采样转换时间也要缩短
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_HwStart(void)
{
	TIMER_KillTask(TIMER_ID_ADC_SCAN);

	adcCB.currentChannel = ADC_CHANNEL_BATTERY;

	// 注册定时器任务，周期性采样 如果有多个采样通道 会周期性转换通道进行采样 
	TIMER_AddTask(TIMER_ID_ADC_SCAN,
				ADC_SCAN_TIME,
				ADC_CALLBACK_SetScanRequest,
				0,
				TIMER_LOOP_FOREVER,
				ACTION_MODE_DO_AT_ONCE);
					
	// 启动下一次转换
	ADC_CONVERT_START();
}

/*********************************************************************
* 函 数 名: ADC_Stop
* 函数入参: 无
* 函数出参: 无
* 返 回 值: 无 
* 功能描述: 注销定时器任务，暂停周期性采样 
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_Stop(void)
{
	TIMER_KillTask(TIMER_ID_ADC_SCAN);
}

/*********************************************************************
* 函 数 名: ADC_HwInit
* 函数入参: 无
* 函数出参: 无
* 返 回 值: 无 
* 功能描述: ADC硬件初始化  
* 注意：移植时开发平台不同或硬件不同，初始化函数和引脚都会不同，根据情况作出修改
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_HwInit(void)
{
	GPIO_InitType GPIO_InitStructure;
	ADC_InitType ADC_InitStructure;

    // Enable GPIO clocks
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    // Enable ADC clocks
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_ADC, ENABLE);

		//初始化GPIO
    GPIO_InitStruct(&GPIO_InitStructure);
    GPIO_InitStructure.Pin            = GPIO_PIN_0;
    GPIO_InitStructure.GPIO_Current   = GPIO_DC_4mA;					// 指定引脚电流
    GPIO_InitStructure.GPIO_Slew_Rate = GPIO_Slew_Rate_Low;		// 指定速度
    GPIO_InitStructure.GPIO_Pull      = GPIO_No_Pull;					// 上拉/下拉
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_Analog;			// 模拟输入模式
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);

    // RCC_ADCHCLK_DIVx
    // ADC的输入时钟不得超过64MHz
    ADC_ConfigClk(ADC_CTRL3_CKMOD_AHB, RCC_ADCHCLK_DIV4); 			  //配置ADC指定预分频器
	  RCC_ConfigAdc1mClk(RCC_ADC1MCLK_SRC_HSE, RCC_ADC1MCLK_DIV8);  //selsect HSE as RCC ADC1M CLK Source

	// ADC configuration 初始化ADC外围设备
    ADC_InitStructure.MultiChEn      = DISABLE;   //指定 多通道/单通道 使能/失能
    ADC_InitStructure.ContinueConvEn = DISABLE;   //指定 单次转换/连续转换 使能/失能
    ADC_InitStructure.ExtTrigSelect  = ADC_EXT_TRIGCONV_NONE;  //指定外部触发器
    ADC_InitStructure.DatAlign       = ADC_DAT_ALIGN_R;		//数据对其 左/右
    ADC_InitStructure.ChsNumber      = 1;		//指定要转换的ADC通道
    ADC_Init(ADC, &ADC_InitStructure);
		
		//配置选定ADC采样通道，采样序列排名，采样时期
	  ADC_ConfigRegularChannel(ADC, ADC_CH_1, 1, ADC_SAMP_TIME_55CYCLES5);
    // Enable ADC
    ADC_Enable(ADC, ENABLE);
    // Check ADC Ready
    while(ADC_GetFlagStatusNew(ADC, ADC_FLAG_RDY) == RESET);
    // 建议每次上电执行一次校准
    // Start ADC1 calibration 启动ADC校准过程
    ADC_StartCalibration(ADC);
    // Check the end of ADC1 calibration
    while (ADC_GetCalibrationStatus(ADC));
    
	// 启动ADC转换
    ADC_EnableSoftwareStartConv(ADC, ENABLE);
	
}

/*********************************************************************
* 函 数 名: ADC_InitStr
* 函数入参: 无
* 函数出参: 无
* 返 回 值: 无 
* 功能描述: 初始化采样通道的结构体
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/

void ADC_InitStr(void)
{
	uint8 i;
	
	adcCB.scanRequest = FALSE;
	adcCB.currentChannel = (ADC_CHANNEL_E)0;

	// 相关数据结构初始化
	for ( i=0; i < ADC_BATTERY_DETECTOR_BUFF_SIZE; i++ )
	{
		adcCB.battery.adcBuff[i] = 0;
	}
	adcCB.battery.index = 0;
	adcCB.battery.adcAverage = 0;
	
}


/*********************************************************************
* 函 数 名: ADC_CALLBACK_SetScanRequest
* 函数入参: 无
* 函数出参: 无
* 返 回 值: 无 
* 功能描述: 设置读取请求回调函数，由定时器调用
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_CALLBACK_SetScanRequest(uint32 param)
{
	adcCB.scanRequest = TRUE;
}


/*********************************************************************
* 函 数 名: ADC_AddDataToBuff
* 函数入参: ADC_CHANNEL_E channel 采样通道,  uint16 data 采样数据
* 函数出参: 无
* 返 回 值: 无
* 功能描述: 向缓冲区中添加一个数据 对采集的数据进行处理，去掉最大最小值取平均值
* 注意：添加新的采样通道做法和电池电压采样通道相似，按照示例加入新的采样通道即可
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_AddDataToBuff(ADC_CHANNEL_E channel, uint16 data)
{
	uint8 i;
	uint32 total;
	uint16 min = 0xFFFF, max = 0;
	
	switch(channel)
	{
		// 电池电压采样通道
		case ADC_CHANNEL_BATTERY:
			adcCB.battery.adcBuff[adcCB.battery.index++] = data;
			//缓冲区满了之后要对缓冲区清0以便后续存放数据
			adcCB.battery.index %= ADC_BATTERY_DETECTOR_BUFF_SIZE;

			// 计算缓冲区的平均值
			total = 0;
			for (i=0; i<ADC_BATTERY_DETECTOR_BUFF_SIZE; i++)
			{
				// 更新最小值
				if (adcCB.battery.adcBuff[i] < min)
				{
					min = adcCB.battery.adcBuff[i];
				}

				// 更新最大值
				if (adcCB.battery.adcBuff[i] > max)
				{
					max = adcCB.battery.adcBuff[i];
				}
				
				total += adcCB.battery.adcBuff[i];
			}

			// 去掉最大最小值
			total -= min;
			total -= max;

			// 计算ADC平均值
			adcCB.battery.adcAverage = total / (ADC_BATTERY_DETECTOR_BUFF_SIZE-2);

			break;

		// 非法通道
		default:
			break;
	}
}

/*********************************************************************
* 函 数 名: ADC_GetBatteryPercent
* 函数入参: 电压值
* 函数出参: 返回采集电压对应当前电压范围的百分比
* 返 回 值: percent 
* 功能描述: 计算采集电压对应的电池百分比
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
uint8 ADC_GetBatteryPercent(uint32 param)
{
	uint8 i = 0;
	uint8 vLevel=0;  
	uint8 percent;
	float k = 0.0f, b = 0.0f;
	float fPercent;

//选择不同的电压来判断采集电压的百分比大小，电压选则根据要求而定
	switch (PARAM_GetBatteryVoltageLevel())
	{
		case 24:
			vLevel = 0;
			break;

		case 36:
			vLevel = 1;
			break;

		case 48:
			vLevel = 2;
			break;

		case 52:
			vLevel = 3;
			break;

		case 60:
			vLevel = 4;
			break;
			
		case 72:
			vLevel = 5;
			break;	

		default:
			vLevel = 1;
			break;
	}

	// 在对应电压的电量里程碑中搜索，计算电池百分比
	for (i = 0; i < BATTERY_PERCENT_GATE_SIZE; i++)
	{
		// 计算得到的百分比应能覆盖0%-100%
		// 从100%开始搜索
		if (param >= batteryPercentTable[vLevel][i])
		{
			if (0 == i)
			{
				return 100;
			}

			// y = kx+b，首先计算k
			k = (float)((batteryPercent[i-1] - batteryPercent[i])/100.0f) / (float)(batteryPercentTable[vLevel][i-1] - batteryPercentTable[vLevel][i]);
			b = batteryPercent[i]/100.0f - k * batteryPercentTable[vLevel][i];
			
			fPercent = k * param + b;

			if (fPercent >= 99.5f)
			{
				fPercent = 100;
			}

			percent = (uint8)fPercent;
			
			return percent;
		}
	}
	
	return 0;
}

/*********************************************************************
* 函 数 名: ADC_BatteryTortoiseProcess
* 函数入参: 无
* 函数出参: 无
* 返 回 值: 无 
* 功能描述: 设置电池百分比连续显示变化值
***********
* 修改历史:
*   1.修改作者:刘鑫旭
*     修改日期: 2024/7/22
*     修改描述: 新函数 	
**********************************************************************/
void ADC_TortoiseProcess(void)
{
	static uint16 adcScanCount = 0;		// 这里的处理与ADC扫描控制使用同一个定时器，这里使用该参数对定时器进行分频，分频数与设定的滤波等级有关
	static uint16 keynoteTimeCnt = 0;
	static BOOL keynoteFlag = FALSE;	

	// 在开机后的一个更新buf周期定下电池百分比基调 用于计数值和采样缓冲区里数据的比较
	if (!keynoteFlag)
	{
		keynoteTimeCnt ++; 
		if (keynoteTimeCnt < ((5*ADC_BATTERY_DETECTOR_BUFF_SIZE*ADC_SCAN_TIME*ADC_CHANNEL_MAX)/(ADC_SCAN_TIME*ADC_CHANNEL_MAX)))
		{
			adcCB.battery.displayPercent = ADC_GetBatteryPercent(adcCB.battery.realtimeVoltage);
			
			// 将实时的电池百分比放入参数层
			PARAM_SetBatteryPercent(adcCB.battery.displayPercent);
			
			return;
		}
		keynoteFlag = TRUE;
	}

	// 对ADC扫描定时器作分频处理
	adcScanCount ++;
	if (adcScanCount < ((uint32)PARAM_GetBatteryCapVaryTime()*1000UL/(ADC_SCAN_TIME*ADC_CHANNEL_MAX)))
	{
		return;
	}
	adcScanCount = 0;

	// 显示百分比 ＜ 实时百分比，则显示百分比增大一个百分点
	if(adcCB.battery.displayPercent < adcCB.battery.realtimePercent)
	{
		adcCB.battery.displayPercent++;
		if(adcCB.battery.displayPercent > 100)
		{
			adcCB.battery.displayPercent = 100;
		}
	}
	// 显示百分比 ＞ 实时百分比，则显示百分比降低一个百分点
	else if(adcCB.battery.displayPercent > adcCB.battery.realtimePercent)
	{
		if(adcCB.battery.displayPercent > 0)
		{
			adcCB.battery.displayPercent--;
		}
	}

	// 将处理后的百分比放入参数层
	PARAM_SetBatteryPercent(adcCB.battery.displayPercent);

	// 当前电量小于10，置位低电标志
	PARAM_SetBatteryLowAlarm((BOOL)(PARAM_GetBatteryPercent() < 10));
}
