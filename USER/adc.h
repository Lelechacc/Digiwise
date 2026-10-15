/********************************************************************************
  * 文 件 名: adc.h
  * 版 本 号: V0.0
	* 负责人 ：刘鑫旭
  *******************************************************************************/


#ifndef __ADC_H__
#define __ADC_H__

/********************************* 头文件************************************/
#include "common.h"
#include "timer.h"


/********************************* 宏定义************************************/

#define ADC_SCAN_TIME							10		// AD转换间隔时间，单位:ms
#define ADC_BATTERY_DEFAULT_FILTER_TIME			1		// 电池百分比滤波时间，单位:s，为0时相当于不处理

//采样通道采集数据的缓冲区大小定义
#define ADC_BATTERY_DETECTOR_BUFF_SIZE			10		// 电池数据结构中的缓冲区大小



//=============================================================================================
//宏定义 便于不同开发平台使用，被调用的函数可能根据开发平台的不同做出部分修改

//判断ADC是否转换结束的标志
#define ADC_CONVERT_IS_BUSY()   				ADC_GetFlagStatus(ADC, ADC_FLAG_ENDC) 
//启用所选ADC软件转换
#define ADC_CONVERT_START()     				ADC_EnableSoftwareStartConv(ADC, ENABLE)
//禁用所选ADC软件转换
#define ADC_CONVERT_STOP()      				ADC_EnableSoftwareStartConv(ADC, DISABLE)
//ADCX配置通道 排序 采样周期
#define ADC_CONVERT_CHANNEL(X)  				ADC_ConfigRegularChannel(ADC, X, 1, ADC_SAMP_TIME_55CYCLES5)
//返回常规通道最后一次ADCx转换结果数据
#define ADC_CONVERT_RESULT()   					ADC_GetDat(ADC)
//清除ADC转换结束标志
#define ADC_CONVERT_CLEAR_EOC()   			ADC_ClearFlag(ADC, ADC_FLAG_ENDC)


// 通道定义，与硬件ADC的模拟通道一致
//采集的通道会有很多 如油门、刹把等 增加新的采样通道就在这里定义
typedef enum
{
	ADC_CHANNEL_BATTERY = 0,			// 36V主电
	/**   此处定义新的采样通道   **/
	
	ADC_CHANNEL_MAX
}ADC_CHANNEL_E;

// 采样通道数据结构
typedef struct
{
	volatile BOOL scanRequest;						// 转换请求
	ADC_CHANNEL_E currentChannel;					// 当前要扫描的通道

	// 主电池相关数据结构
	struct
	{
		uint16 adcBuff[ADC_BATTERY_DETECTOR_BUFF_SIZE];
		uint8  index;								// 当前处理的缓冲区下标
		uint16 adcAverage;							// 平均值

		uint32 realtimeVoltage;			// 检测结果电压

		uint8 realtimePercent;	// 实时电压百分比

		uint8 displayPercent;	// 电池过程百分比，用来防止大跳动
	}battery;
	
}ADC_CB;

extern ADC_CB adcCB;

/***************************** 函数外部申明*******************************/

// 初始化
void ADC_BaseInit(void);

// ADC处理，在main中调用
void ADC_Process(void);

#endif


