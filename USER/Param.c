/********************************************************************************
  * 文 件 名: Param.c
  * 版 本 号: 初版
  * 修改作者: Fly & ZJ & Workman
  * 修改日期: 2017年08月06日
  * 功能介绍: 该文件用于骑行参数的获取与计算，提供可靠的数据给上层显示或传输            
  ******************************************************************************
  * 注意事项:
  *
  * 							版权归迪太科技所有.
  *
*********************************************************************************/

/*******************************************************************************
 *                                  头文件	                                   *
********************************************************************************/
#include "Param.h"
#include "iap.h"
#include "timer.h"
#include "uartDrive.h"
#include "dtconfig.h"
#include "powerCtl.h"
#include "backlight.h"
#include "state.h"

// 函数声明
void NVM_Load(void);
void NVM_Save(BOOL saveAll);
void NVM_Save_FactoryReset(BOOL saveAll);
void NVM_Load_FactoryReset(void);
void NVM_Format(void);
BOOL NVM_IsFormatOK(void);
void PARAM_SetDefaultRunningData(void);
void PARAM_CaculateTripDistance(void);
uint8 PARAM_CmpareN(const uint8 *str1, const uint8 *str2, uint8 len);


/*******************************************************************************
 *                                  变量定义                                   *
********************************************************************************/
PARAM_CB paramCB;

const uint32 PARAM_MCU_VERSION_ADDRESS[] = {PARAM_MCU_DT_SN_ADDEESS, PARAM_MCU_CUSTOMER_SN_ADDEESS, PARAM_MCU_HW_ADDEESS, \
											PARAM_MCU_BOOT_ADDEESS, PARAM_MCU_APP_ADDEESS, PARAM_MCU_BLE_UPDATA_FLAG_ADDEESS, \
											PARAM_MCU_BLE_UPDATA_CRC_ADDEESS, PARAM_MCU_QR_ADDEESS, PARAM_MCU_TEST_FLAG_ADDEESS	};
// NVM格式化标识
const uint8 formatFlag[] = NVM_FORMAT_FLAG_STRING;

uint8 flashBuf[PARAM_MCU_PAGE_SIZE];
uint8 userSetFlashBuf[PARAM_MCU_PAGE_SIZE];
uint32 flashBackupFlag;

// 加载系统参数
void NVM_Load(void)
{
	//==========================================================================================
	// 读取备份区的标识数据是否有效标志
	IAP_FlashReadWordArray(PARAM_NVM_BACKUP_START_ADDRESS + (PARAM_MCU_PAGE_SIZE-4), 
							(uint32*)&flashBackupFlag, 
							1);

	// 备份数据有效
	if (0x12345678 == flashBackupFlag)
	{
		// 拷贝备份区数据到运行区
		IAP_FlashReadWordArray(PARAM_NVM_BACKUP_START_ADDRESS, 
								(uint32*)flashBuf, 
								PARAM_MCU_PAGE_SIZE / 4);

		// 将拷贝的数据写入数据运行区
		IAP_FlashWriteWordArrayWithErase(PARAM_NVM_START_ADDRESS, 
								(uint32*)flashBuf, 
								PARAM_MCU_PAGE_SIZE / 4);

		// 清备份数据标志
		flashBackupFlag = 0x00000000;
		IAP_FlashWriteWordArrayWithErase(PARAM_NVM_BACKUP_START_ADDRESS + (PARAM_MCU_PAGE_SIZE-4), 
								(uint32*)&flashBackupFlag, 
								1);
	}

	// 读取NVM中的数据
	IAP_FlashReadWordArray(PARAM_NVM_START_ADDRESS, 
							(uint32 *)paramCB.nvm.array, 
							(PARAM_NVM_DATA_SIZE + 3) / 4);
}

// 保存系统参数
void NVM_Save(BOOL saveAll)
{

	//==============================================================================	
	// 先从最后一个扇区读取数据拷贝到倒数第二个扇区备份，然后写入一个标志到备区，接着更新最后一个扇区，最后擦除备区的标志
	IAP_FlashReadWordArray(PARAM_NVM_START_ADDRESS, 
							(uint32*)flashBuf, 
							PARAM_MCU_PAGE_SIZE / 4);

	flashBuf[PARAM_MCU_PAGE_SIZE-4] = 0x78;
	flashBuf[PARAM_MCU_PAGE_SIZE-3] = 0x56;
	flashBuf[PARAM_MCU_PAGE_SIZE-2] = 0x34;
	flashBuf[PARAM_MCU_PAGE_SIZE-1] = 0x12;
	
	IAP_FlashWriteWordArrayWithErase(PARAM_NVM_BACKUP_START_ADDRESS, 
							(uint32*)flashBuf, 
							PARAM_MCU_PAGE_SIZE / 4);
						
	// 只保存变化的数据
	IAP_FlashWriteWordArrayWithErase(PARAM_NVM_START_ADDRESS, 
									(uint32*)paramCB.nvm.array, 
									(PARAM_NVM_DATA_SIZE + 3) / 4);

	// 清标志
	flashBackupFlag = 0x00000000;
	
	IAP_FlashWriteWordArrayWithErase(PARAM_NVM_BACKUP_START_ADDRESS + (PARAM_MCU_PAGE_SIZE-4), 
							(uint32*)&flashBackupFlag, 
							1);
}

// 初始化设置搬参数
void NVM_Load_FactoryReset(void)
{ 
	uint16 value, i =0;
	
	// 从备区地址里面取出来数据拷贝到系统参数区域
	IAP_FlashReadWordArray(PARAM_NVM_FACTORY_RESET_ADDRESS, 
							(uint32*)paramCB.nvm.array, 
							(PARAM_NVM_DATA_SIZE + 3) / 4);

	// 判断数据区是否都为空
	for (i = 0;i < PARAM_NVM_DATA_SIZE; i++)
	{
		if (0xFF == paramCB.nvm.array[i])
		{
			value ++;
		}
	}

	// 数据区全部都是空，直接返回，不写入，防止死机
	if (PARAM_NVM_DATA_SIZE == value)
	{
		return;
	}
	// 只保存变化的数据
	IAP_FlashWriteWordArrayWithErase(PARAM_NVM_START_ADDRESS, 
									(uint32*)paramCB.nvm.array, 
									(PARAM_NVM_DATA_SIZE + 3) / 4);
}



// 保存初始化设置参数
void NVM_Save_FactoryReset(BOOL saveAll)
{
	// 只保存变化的数据
	IAP_FlashWriteWordArrayWithErase(PARAM_NVM_FACTORY_RESET_ADDRESS, 
						(uint32*)paramCB.nvm.array, 
						(PARAM_NVM_DATA_SIZE + 3) / 4);
}



// nvm写模式
void NVM_CALLBACK_EnableWrite(uint32 param)
{
	paramCB.nvmWriteEnable = TRUE;
}


// 返回出厂设置
void PARAM_FactoryReset(void)
{
	// 自Trip清零以来的参数初始化
	paramCB.nvm.param.common.record.trip.ridingTime = 0;				// 总骑行时间秒
	paramCB.nvm.param.common.record.trip.distance = 0;					// Trip，单位:0.1Km
	paramCB.nvm.param.common.record.trip.calories = 0;					// 卡路里，单位:KCal
	paramCB.nvm.param.common.record.trip.speedMax = 0;					// 最大速度	
	paramCB.nvm.param.common.record.trip.speedAverage = 0;		  // 平均速度，单位:0.1Km/h	

	// 自0点以来的参数初始化
	paramCB.nvm.param.common.record.today.trip = 0;						// 今日里程，单位:0.1Km		
	paramCB.nvm.param.common.record.today.calories = 0;					// 今日卡路里，单位:KCal
	paramCB.nvm.param.common.record.today.ridingTime = 0;				// 今日骑行时间清零

	// 保存参数
	NVM_Load_FactoryReset();
	
}


// 默认运行参数
void PARAM_SetDefaultRunningData(void)
{
	uint8 i;
	uint8 * pBuff = NULL;

	/*****************************************************************
	******************************系统参数初始化**********************
	******************************************************************/

	// APP版本号
	pBuff = (uint8 *)&(paramCB.runtime.appVersion);

	IAP_FlashReadWordArray(PARAM_MCU_APP_ADDEESS,(uint32 *)pBuff, 8);
	
	// 如果读取的版本号不符，则强制更新为当前APP版本
	if ((APP_VERSION_LENGTH != pBuff[0]) || (!PARAM_CmpareN((const uint8*)&pBuff[1], (const uint8*)APP_VERSION, APP_VERSION_LENGTH))) 
	{
		pBuff[0] = APP_VERSION_LENGTH;
		for(i = 0; i < APP_VERSION_LENGTH ; i++)
		{
			pBuff[i + 1] = APP_VERSION[i];
		}
		IAP_FlashWriteWordArrayWithErase(PARAM_MCU_APP_ADDEESS, (uint32*)pBuff, (APP_VERSION_LENGTH + 4) / 4);
	}

	// BOOT版本号
	pBuff = (uint8 *)&(paramCB.runtime.bootVersion);
	IAP_FlashReadWordArray(PARAM_MCU_BOOT_ADDEESS, (uint32 *)pBuff, 8);

	// PCB版本号
	pBuff = (uint8 *)&(paramCB.runtime.SnCode);
	IAP_FlashReadWordArray(PARAM_MCU_DT_SN_ADDEESS, (uint32 *)pBuff, 8);

	// 客户版本号
	pBuff = (uint8 *)&(paramCB.runtime.LimeSnCode);
	IAP_FlashReadWordArray(PARAM_MCU_CUSTOMER_SN_ADDEESS, (uint32 *)pBuff, 8);

	// PARAM_MCU_HW_ADDEESS
	pBuff = (uint8 *)&(paramCB.runtime.hwVersion);
	IAP_FlashReadWordArray(PARAM_MCU_HW_ADDEESS, (uint32 *)pBuff, 8);
	
	// 测试标识版本地址
	pBuff = (uint8 *)&(paramCB.runtime.testflag);
	IAP_FlashReadWordArray(PARAM_MCU_TEST_FLAG_ADDEESS, (uint32 *)pBuff, 16);


	// ■■ 不需要保存至NVM的参数 ■■
	paramCB.runtime.fwVer = FW_VERSION;
	paramCB.runtime.hwVer = HW_VERSION;
	paramCB.runtime.btCapacity = BT_CAPACITY;
	
	// 固件版本号
	paramCB.runtime.devVersion[0] = DEV_VERSION[0];
	paramCB.runtime.devVersion[1] = DEV_VERSION[1];
	paramCB.runtime.devVersion[2] = DEV_VERSION[2];
	
	paramCB.runtime.fwBinNum = FW_BINNUM;
	paramCB.runtime.fwBinVersion.devVersionRemain = DEV_VERSION_REMAIN;
	paramCB.runtime.fwBinVersion.devReleaseVersion = DEV_RELEASE_VERSION;
	paramCB.runtime.fwBinVersion.fwUpdateVersion = FW_UPDATE_VERSION;
	paramCB.runtime.fwBinVersion.fwDebugReleaseVersion = FW_DEBUG_RELEASE_VERSION;
	
	// 仪表唯一序列号
	paramCB.runtime.watchUid.sn0 = *(uint32 *)(0x1FFFF7F0);
	paramCB.runtime.watchUid.sn1 = *(uint32 *)(0x1FFFF7F4); 
	paramCB.runtime.watchUid.sn2 = *(uint32 *)(0x1FFFF7F8); 
	
	// 驱动器运行参数
	// 巡航开关，0:表示关闭；1:表示打开 
	paramCB.runtime.cruiseEnabe = FALSE;			
	// 当前巡航状态，1:表示进入巡航状态 
	paramCB.runtime.cruiseState = FALSE; 			
	
	// 6km/h推行功能，0:表示关闭；1:表示打开
	paramCB.runtime.pushAssistOn = FALSE; 			
	
	// 有推车助力功能(APP读取系统信息用)
	paramCB.runtime.isHavePushAssist = TRUE;		
	// 大灯开关	
	paramCB.runtime.lightSwitch = FALSE;			
	
	// 当前电池电压，单位:mV 
	paramCB.runtime.battery.voltage = 40000;			
	
	// 当前电池电流，单位:mA 
	paramCB.runtime.battery.current = 0;			
	
	// 电池容量低告警
	paramCB.runtime.battery.lowVoltageAlarm = 0;	
	
	// 当前速度，单位:0.1Km/h
	paramCB.runtime.speed = 0; 	

	// 滤波值清零
	paramCB.runtime.speedFilter.realSpeed = 0;
	paramCB.runtime.speedFilter.proSpeed = 0;
	paramCB.runtime.speedFilter.difSpeed = 0;

	// 当前错误代码
	paramCB.runtime.errorType = ERROR_TYPE_NO_ERROR;	
	
	// 上一次错误码
	paramCB.runtime.lastErrorType = ERROR_TYPE_NO_ERROR;
	
	// 超速告警，1:表示超速 
	paramCB.runtime.overSpeedAlarm = FALSE; 				

	// 欠压告警，0:表示不欠压；1:表示欠压
	paramCB.runtime.underVoltageAlarm = FALSE;				

	// 电池百分比
	paramCB.runtime.battery.percent = 0;							
	// 骑行统计数据条数
	paramCB.runtime.recordCount = 0;								
	// 电机功率,单位:W
	paramCB.runtime.power = 0;		
	// 蓝牙是否连接，1:表示连接；0:表示未连接
	paramCB.runtime.bleConnectState = FALSE;	
	
	// 电量计算方式
	paramCB.runtime.battery.batteryDataSrc = (BATTERY_DATA_SRC_E)paramCB.nvm.param.common.percentageMethod; 	
	
	// 默认助力档位 
	paramCB.runtime.assist = (ASSIST_ID_E)paramCB.nvm.param.common.defaultAssist;

	/*****************************************************************
	*****************************开发者参数初始化*********************
	******************************************************************/




	
}

// 根据助力档位数设定每个档位的PWM值上下限
void NVM_SetPWMLimitValue(void)
{
	switch (paramCB.nvm.param.common.assistMax)
	{
		// 这组数据没有测试，需要根据样品测试后补充
		//#warning "这组数据没有测试，需要根据样品测试后补充"
		// 根据KDS的样品测试得到的数据
		case ASSIST_3:
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_0] = 0;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_1] = 60;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_2] = 140;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_3] = 230;	
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_P] = 50;
			break;

		// 根据KDS的样品测试得到的数据
		case ASSIST_5:
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_0] = 0;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_1] = 102;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_2] = 140;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_3] = 179;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_4] = 217;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_5] = 255;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_P] = 50;
			break;

		// 这组数据没有测试，需要根据样品测试后补充
		#warning "这组数据没有测试，需要根据样品测试后补充"
		case ASSIST_6:
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_0] = 0;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_1] = 102;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_2] = 132;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_3] = 162;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_4] = 192;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_5] = 222;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_6] = 255;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_P] = 50;
			break;

		// 这组数据没有测试，需要根据样品测试后补充
		#warning "这组数据没有测试，需要根据样品测试后补充"
		case ASSIST_9:
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_0] = 0;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_1] = 102;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_2] = 121;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_3] = 137;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_4] = 156;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_5] = 175;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_6] = 194;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_7] = 213;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_8] = 232;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_9] = 255;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_P] = 50;
			break;
			
		#warning "默认五档PWM这组数据没有测试，需要根据样品测试后补充"
		default:

			paramCB.nvm.param.protocol.pwmFixed[ASSIST_0] = 0;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_1] = 102;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_2] = 140;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_3] = 179;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_4] = 217;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_5] = 255;
			paramCB.nvm.param.protocol.pwmFixed[ASSIST_P] = 50;

			break;
			
	}
}

// 格式化参数
void NVM_Format(void)
{
	uint8 i;

	// 写格式化标识
	for (i=0; i<NVM_FORMAT_FLAG_SIZE; i++)
	{
		paramCB.nvm.param.common.nvmFormat[i] = formatFlag[i];
	}

	/**************************************************************************************
	************************************系统设置参数参数化*********************************
	***************************************************************************************/
	// 设置系统参数
	paramCB.nvm.param.common.battery.voltage = 36;										// 电池电压等级，单位:V
	paramCB.nvm.param.common.battery.filterLevel = 1; 									// 电量显示变化速度，单位:秒
	paramCB.nvm.param.common.speed.limitVal = 320;										// 限速值，单位:0.1Km/h
	paramCB.nvm.param.common.speed.filterLevel = 3; 									// 速度变化平滑度
	paramCB.nvm.param.common.unit = UNIT_METRIC;										// 单位, 公制与英制
	paramCB.nvm.param.common.brightness = BACK_LIGHT_LEVEL_60_100;						// 背光亮度
	paramCB.nvm.param.common.powerOffTime = 10; 										// 自动关机时间，单位:分钟；>=0min
	paramCB.nvm.param.common.assistMax = ASSIST_5;										// 支持最大档位
	paramCB.nvm.param.common.busAliveTime = 10000UL;									// 总线故障超时时间
	paramCB.nvm.param.common.workAssistRatio = 2;										// 机器与人做功助力比
	paramCB.nvm.param.common.existBle = FALSE;											// 是否有蓝牙
	paramCB.nvm.param.common.percentageMethod = BATTERY_DATA_SRC_ADC;					// 电量获取方式(0:控制器上报电压) (1:控制器上报电量) (2:仪表自己测量电压电量)

	// 自出厂以来的参数初始化
	paramCB.nvm.param.common.record.total.maxTripOf24hrs = 0;			// 个人记录，单位:0.1Km
	paramCB.nvm.param.common.record.total.distance = 0; 				// 总里程，单位:0.1Km
	paramCB.nvm.param.common.record.total.ridingTime = 0;				// 总骑行时间，单位:s
	paramCB.nvm.param.common.record.total.calories = 0; 				// 总卡路里，单位:KCal

	// 自Trip清零以来的参数初始化
	paramCB.nvm.param.common.record.trip.ridingTime = 0;				// 总骑行时间秒
	paramCB.nvm.param.common.record.trip.distance = 0;					// Trip，单位:0.1Km
	paramCB.nvm.param.common.record.trip.calories = 0;					// 卡路里，单位:KCal
	paramCB.nvm.param.common.record.trip.speedMax = 0;					// 最大速度 
	paramCB.nvm.param.common.record.trip.speedAverage = 0;								// 平均速度，单位:0.1Km/h	

	// 自0点以来的参数初始化
	paramCB.nvm.param.common.record.today.trip = 0; 					// 今日里程，单位:0.1Km 	
	paramCB.nvm.param.common.record.today.calories = 0; 				// 今日卡路里，单位:KCal
	paramCB.nvm.param.common.record.today.ridingTime = 0;				// 今日骑行时间清零
	paramCB.nvm.param.common.record.today.month = 1;					// 今日参数对应的日期
	paramCB.nvm.param.common.record.today.day = 1;						// 今日参数对应的日期
	paramCB.nvm.param.common.record.today.year = 2019;					// 今日参数对应的日期


	paramCB.nvm.param.protocol.driver.currentLimit = 15000; 							// 电流门限，单位:mA
	paramCB.nvm.param.protocol.driver.lowVoltageThreshold = 31500;						// 欠压门限，单位:mV
	paramCB.nvm.param.common.pushSpeedSwitch = 1;										// 默认有推车助力

	paramCB.nvm.param.protocol.wheelSizeID = PARAM_WHEEL_SIZE_26_INCH;					// 轮径
	paramCB.nvm.param.protocol.powerMax = 250;											// 最大电机功率,单位:W

	paramCB.nvm.param.protocol.newWheelSizeInch = 260;									// 新轮径
	paramCB.nvm.param.protocol.newPerimeter = 2074;										// 新的轮径周长
	paramCB.nvm.param.common.uartProtocol = 0;											// 协议选择 0：KM5S	1：锂电2号		2：八方	 3：J协议

	paramCB.nvm.param.protocol.eBikeName = 0;											// 车名	0：PACE500 	1：PACE350	2：LEVEL	3：SINCH	4：AVENTURE  5：PACE
	paramCB.nvm.param.protocol.powerMax = 250;											// 最大电机功率,单位:W	
	paramCB.nvm.param.protocol.batteryCap = 0;											// 电池容量
	paramCB.nvm.param.protocol.showWheelSizeInch = 0;									// 显示周长
	paramCB.nvm.param.protocol.tiresSize = 0;											// 车轮宽度

	paramCB.nvm.param.common.defaultAssist = 0;											// 默认档位
	paramCB.nvm.param.common.logoMenu = 0;												// LOGO界面
	paramCB.nvm.param.common.uartLevel = 1;												// 串口电平		0:--3.3V       1:--5V
	paramCB.nvm.param.protocol.driver.cruiseSwitch = FALSE;								// 定速巡航功能开关
	paramCB.nvm.param.protocol.beepSwitch = FALSE;										// 蜂鸣器开关
	paramCB.nvm.param.protocol.limitSpeedBeepOn = 0;									// 高速蜂鸣器提醒
	paramCB.nvm.param.protocol.resFactorySet = TRUE;									// 恢复到出厂设置

	paramCB.nvm.param.protocol.menuPassword	= PARAM_NVM_MENU_PASSWORD;					// 菜单界面密码
	paramCB.nvm.param.protocol.powerOnPassword = PARAM_NVM_POWERON_PASSWORD;			// 开机界面密码
	paramCB.nvm.param.protocol.powerPasswordSwitch = 1;									// 关闭开机密码 0是有密码 1是无密码
	paramCB.nvm.param.protocol.menuPasswordSwitch = 0;									// 关闭菜单密码 0是有密码 1是无密码
	paramCB.nvm.param.protocol.carModel[0] = 'c';										// 车型
	paramCB.nvm.param.protocol.carModel[1] = 'A';


	/**************************************************************************************
	************************************协议参数参数化*************************************
	***************************************************************************************/	
	
	paramCB.nvm.param.protocol.driver.steelNumOfStartAssist = 2;						// 助力开始的磁钢数
	paramCB.nvm.param.protocol.driver.steelNumOfSpeedSensor = 1;						// 测速传感器一圈磁钢数
	paramCB.nvm.param.protocol.driver.assistDirectionForward = FALSE;					// 助力正反
	paramCB.nvm.param.protocol.driver.assistPercent = 64;								// 助力比例
	paramCB.nvm.param.protocol.driver.slowStart = 2;									// 缓启动参数
	paramCB.nvm.param.protocol.driver.turnBarSpeed6kmphLimit = FALSE;					// 转把是否限速6km/h
	paramCB.nvm.param.protocol.driver.turnBarLevel = FALSE; 							// 转把是否分档
	
	paramCB.nvm.param.protocol.driver.controlMode = 0;									// 控制器驱动模式
	paramCB.nvm.param.protocol.driver.zeroStartOrNot = FALSE;							// 零启动或非零启动
	paramCB.nvm.param.protocol.driver.switchCruiseMode = FALSE; 						// 巡航切换模式
	paramCB.nvm.param.protocol.driver.switchCruiseWay = FALSE;							// 切换巡航的方式
	paramCB.nvm.param.protocol.driver.assistSensitivity = 3;							// 助力灵敏度
	paramCB.nvm.param.protocol.driver.assistStartIntensity = 3; 						// 助力启动强度
	paramCB.nvm.param.protocol.driver.assistSteelType = 5;								// 助力磁钢类型
	paramCB.nvm.param.protocol.driver.reversalHolzerSteelNum = 0;						// 限速电机换向霍尔磁钢数(未使用)
	paramCB.nvm.param.protocol.driver.turnBarSpeedLimit = 55;							// 转把限速
	paramCB.nvm.param.protocol.driver.motorFuture = 100;								// 电机特性参数
	paramCB.nvm.param.protocol.driver.turnBarFunction = 0;								// 转把功能
	paramCB.nvm.param.protocol.driver.motorPulseCode = 0;								// 电机相位编码
	paramCB.nvm.param.protocol.driver.assistSensorSignalNum = 6;						// 助力传感器信号数
	paramCB.nvm.param.protocol.driver.assistTrim = 1;									// 助力微调
	paramCB.nvm.param.protocol.driver.absBrake = 2; 									// ABS刹车
	paramCB.nvm.param.protocol.driver.hmiProperty = 70; 								// 仪表属性
	paramCB.nvm.param.protocol.driver.assistSwitch = TRUE;								// 助力开关
	paramCB.nvm.param.protocol.driver.speedLimitSwitch = FALSE; 						// 限速开关
	
	// 根据最大档位，设定每个档位PWM值的上下限
	NVM_SetPWMLimitValue();

	/**************************************************************************************
	************************************开发者新增*****************************************
	***************************************************************************************/


	



	// 保存参数
	NVM_Save(TRUE);

}

// 判断NVM是否已格式化
BOOL NVM_IsFormatOK(void)
{
	uint16 i;

	// 逐个比较，只要发现一个不同，即认为未格式化
	for (i=0; i<NVM_FORMAT_FLAG_SIZE; i++)
	{
		if(formatFlag[i] != paramCB.nvm.param.common.nvmFormat[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}

// 设置脏标识
void NVM_SetDirtyFlag(BOOL writeAtOnce)
{
	paramCB.nvmWriteRequest = TRUE;

	if (writeAtOnce)
	{
		paramCB.nvmWriteEnable = TRUE;
	}
}

// 新的一天开始啦!!
// 当前的数据与历史记录比较，并且清除今日数据
void PARAM_NewDayStart(void)
{
	// 当前数据与历史记录比较
	// 今日里程＞历史最佳，更新之
	if (paramCB.nvm.param.common.record.today.trip > paramCB.nvm.param.common.record.total.maxTripOf24hrs)
	{
		paramCB.nvm.param.common.record.total.maxTripOf24hrs = paramCB.nvm.param.common.record.today.trip;
	}

	// 今日里程清零
	paramCB.nvm.param.common.record.today.trip = 0;

	// 今日骑行时间清零
	paramCB.nvm.param.common.record.today.ridingTime = 0;

	// 今日骑行卡路里清零
	paramCB.nvm.param.common.record.today.calories = 0;

	// 保存新日期
	paramCB.nvm.param.common.record.today.day = paramCB.runtime.rtc.day;
	paramCB.nvm.param.common.record.today.month = paramCB.runtime.rtc.month;
	paramCB.nvm.param.common.record.today.year = paramCB.runtime.rtc.year;

	// NVM更新，非立即写入
	NVM_SetDirtyFlag(FALSE);	
	
}


/*********************************************************************
* 函 数 名: PARAM_CaculateCaloriesByParameters
* 函数入参: ASSIST_ID_E assistLevel 		示例: 档位
			uint16 workAssistRatio		示例: 助力比	
			uint16 motorPower			示例: 电机功率,单位:W
			uint16 speed				示例: 速度，单位:0.1km/h
			uint16 torque				示例: 力矩, 单位:N.m
			uint16 time					示例: 时间, 单位:s
* 函数出参: 无
* 返 回 值: uint16	示例: 返回采样时间内的卡路里
* 功能描述:   根据档位、电机功率、助力比、速度等信息计算出卡路里
***********
* 修改历史:
*   1.修改作者: ZJ
*     修改日期: 2017年06月21日
*     修改描述: 新函数 		   
**********************************************************************/
uint16 PARAM_CaculateCaloriesByParameters(ASSIST_ID_E assistLevel, uint16 workAssistRatio, uint16 motorPower, uint16 speed, uint16 torque, uint16 time)
{
	uint32 uwPowerByPeople = 0;			// 单位:W
	uint16 uwCalories = 0;				// 单位:卡
	float fCalCoff = 1 / 4.184;			// 1 W = 1 kj/ms = 1 j/s; 1 卡 = 4.184 j
	
	// 计算人产生的功率
	if ( ASSIST_0 == assistLevel )
	{
		// 无助力，根据速度进行计算， P=F*V; F=G*f; G是人与车的重量，f是摩擦系数
		uwPowerByPeople = 750UL * speed / 3600;		// 750 * 0.01 * speed * 100.f / 3600
	}
	else
	{
		// 有助力，根据助力比与电机功率来进行计算，示例:助力比为3/1，表示电机出3份力，人出1份力		
		uwPowerByPeople = workAssistRatio ? (motorPower / workAssistRatio) : 0;
	}

	// 积分计算卡路里,单位:cal
	uwCalories = uwPowerByPeople * (time / 1000.0) * fCalCoff;

	return uwCalories;
}

/*********************************************************************
* 函 数 名: PARAM_CaculateTripDistance
* 函数入参: PARAM_CB* pParamCB
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 参数计算过程，包括速度、里程、卡路里等等
***********
* 修改历史:
*   1.修改作者: ZJ
*     修改日期: 2017年08月06日
*     修改描述: 新函数 		   
**********************************************************************/
void PARAM_CaculateTripDistance(void)
{
	float fTempAvgSpeed = 0.0;
	float fTempTrip = 0.0;
	
	static float tripBuff = 0.0;
		
	// 计算单次小里程，单位:0.1KM，公式: 速度*采样时间
	fTempTrip = (float)paramCB.runtime.speed * SET_PARAMCYCLETIME / 3600000.f;

	tripBuff += fTempTrip;		// Trip缓冲区

	// 自Trip清零以来的里程、今天骑行里程刷新
	if (tripBuff > 1.0f)
	{
		tripBuff -= 1.0f;

		// 自Trip清零以来的里程刷新
		paramCB.nvm.param.common.record.trip.distance ++;

		// 今天骑行里程刷新
		paramCB.nvm.param.common.record.today.trip ++;

		// 个人单日骑行最大里程刷新
		if ( paramCB.nvm.param.common.record.total.maxTripOf24hrs < paramCB.nvm.param.common.record.today.trip )	// 刷新个人最佳
		{
			paramCB.nvm.param.common.record.total.maxTripOf24hrs = paramCB.nvm.param.common.record.today.trip;
		}

		// 自出厂以来总里程刷新
		paramCB.nvm.param.common.record.total.distance ++;
		
		// 计算自Trip清零以来的平均速度，公式:Trip清零以来 / Trip清零以来骑行时间
		// 骑行时间为0时的处理
		if(0 == paramCB.nvm.param.common.record.trip.ridingTime)
		{
			fTempAvgSpeed = 0;
		}
		// 骑行时间不为0时的处理
		else
		{
			fTempAvgSpeed = (float)paramCB.nvm.param.common.record.trip.distance * 3600.0f  / paramCB.nvm.param.common.record.trip.ridingTime;
		}

		if (abs(fTempAvgSpeed - paramCB.nvm.param.common.record.trip.speedAverage) >= 1)
		{
			paramCB.nvm.param.common.record.trip.speedAverage= fTempAvgSpeed;

			// 平均速度不能大于最大速度
			if (paramCB.nvm.param.common.record.trip.speedAverage >= paramCB.nvm.param.common.record.trip.speedMax)
			{
				paramCB.nvm.param.common.record.trip.speedAverage = paramCB.nvm.param.common.record.trip.speedMax * 0.9f;
			}
		}

		// 硬件修改无E2PPROM，里程数据修改为1KM写入Flash一次
		if ((paramCB.nvm.param.common.record.total.distance - paramCB.nvm.param.common.record.total.preDistance) >= 10)
		{
			// 更新比较值
			paramCB.nvm.param.common.record.total.preDistance = paramCB.nvm.param.common.record.total.distance;
			
			// NVM更新,立即写入
			NVM_SetDirtyFlag(TRUE);
		}
	}
}

// 卡路里计算
void PARAM_CaculateCalories(void)
{
	static uint16 calorieBuff = 0;

	// 获取单个参数上报周期的卡路里，添加到缓冲区中
	calorieBuff += PARAM_CaculateCaloriesByParameters(paramCB.runtime.assist,
												PARAM_GetWorkAssistRatio(), 
												paramCB.runtime.power, 
												paramCB.runtime.speed, 
												paramCB.runtime.torque, 
												SET_PARAMCYCLETIME);

	// 缓存的卡路里超过一定值时，保存到NVM中
	if (calorieBuff >= 1000)
	{
		calorieBuff -= 1000;

		// 自出厂以来的卡路里增加
		paramCB.nvm.param.common.record.total.calories ++;
		
		// 自Trip清零以来的卡路里增加
		paramCB.nvm.param.common.record.trip.calories ++;

		// 自0点以来的卡路里增加
		paramCB.nvm.param.common.record.today.calories ++;

		// NVM更新，非立即写入
		NVM_SetDirtyFlag(FALSE);
	}
}

// 计算速度类
void PARAM_CaculateSpeed(void)
{
	// 计算自Trip清零以来最高速度
	if ( paramCB.nvm.param.common.record.trip.speedMax < paramCB.runtime.speed )
	{
		paramCB.nvm.param.common.record.trip.speedMax = paramCB.runtime.speed;

		// NVM更新，非立即写入
		NVM_SetDirtyFlag(FALSE);
	}

	// 设置超速告警
	if ( paramCB.runtime.speed >= paramCB.nvm.param.common.speed.limitVal )
	{		
		paramCB.runtime.overSpeedAlarm = TRUE;
	}
	else
	{
		paramCB.runtime.overSpeedAlarm = FALSE;
	}

	// 计算用户最大速度
	if (paramCB.runtime.speed > paramCB.runtime.record.userMaxSpeed)
	{
		paramCB.runtime.record.userMaxSpeed = paramCB.runtime.speed;
	}
}

// 计算骑行时间
void PARAM_CALLBACK_CaculateRidingTime(uint32 param)
{
	// 当前时间与系统保存的今日骑行日期不一致，认为是新的一天开始了
	if ((paramCB.runtime.rtc.year != paramCB.nvm.param.common.record.today.year)
	|| (paramCB.runtime.rtc.day != paramCB.nvm.param.common.record.today.day)
	|| (paramCB.runtime.rtc.month != paramCB.nvm.param.common.record.today.month))
	{
		PARAM_NewDayStart();
	}

	// 速度为0，骑行时间不计时
	if (0 == paramCB.runtime.speed)
	{
		// 运动时间清零，静止时间递增
		paramCB.runtime.record.moveKeepTime = 0;

		if (paramCB.runtime.record.stopKeepTime < 0xFFFF)
		{
			paramCB.runtime.record.stopKeepTime ++;
		}
		
		return ;
	}
	// 速度不为0，说明在运动
	else
	{
		// 运动时间递增，静止时间清零
		if (paramCB.runtime.record.moveKeepTime < 0xFFFF)
		{
			paramCB.runtime.record.moveKeepTime ++;
		}
		paramCB.runtime.record.stopKeepTime = 0;
	}

	// 非记录状态不进行计算
	if (RECORD_STATE_STANDBY == RECORD_GetState())
	{
		return;
	}

	// 自出厂以来的骑行时间增加
	paramCB.nvm.param.common.record.total.ridingTime ++;

	// 自Trip清零以来的骑行时间增加
	paramCB.nvm.param.common.record.trip.ridingTime ++;

	// 自0点以来的骑行时间增加
	paramCB.nvm.param.common.record.today.ridingTime ++;

	// NVM更新，非立即写入，默认用定时器定时写入，否则存在很快把Flash写坏的风险
	NVM_SetDirtyFlag(FALSE);

	// 根据电池电压，计算电池电量百分比
	// 电量计算由ADC模块完成
}


/*********************************************************************
* 函 数 名: PARAM_CallBack_CaculateTripDistance
* 函数入参: void
* 函数出参: 无
* 返 回 值: uint32 
* 功能描述: 初始化参数
***********
* 修改历史:
*   1.修改作者: ZJ
*     修改日期: 2017年08月06日
*     修改描述: 新函数 		   
**********************************************************************/
void PARAM_CallBack_CaculateTripDistance(uint32 param)
{

	if (ERROR_TYPE_COMMUNICATION_TIME_OUT == PARAM_GetErrorCode())
	{
		return;
	}
	
	// 赋值计算请求
	paramCB.recaculateRequest = TRUE;
	
}

/*********************************************************************
* 函 数 名: PARAM_CallBack_SpeedFilterProcess
* 函数入参: void
* 函数出参: 无
* 返 回 值: uint32 
* 功能描述: 初始化参数
***********
* 修改历史:
*   1.修改作者: ZJ
*     修改日期: 2017年08月06日
*     修改描述: 新函数 		   
**********************************************************************/
void PARAM_SpeedFilterDevValue(uint32 param)
{
	// 偏差值计算
 	paramCB.runtime.speedFilter.realSpeed  =  param;
 	
	if (paramCB.runtime.speedFilter.realSpeed >= paramCB.runtime.speedFilter.proSpeed)
	{
		paramCB.runtime.speedFilter.difSpeed = (paramCB.runtime.speedFilter.realSpeed - paramCB.runtime.speedFilter.proSpeed) / 5.0f;
	}
	else
	{
		paramCB.runtime.speedFilter.difSpeed = (paramCB.runtime.speedFilter.proSpeed -  paramCB.runtime.speedFilter.realSpeed) / 5.0f;

		
		if ((0 == paramCB.runtime.speedFilter.difSpeed) && (0 == paramCB.runtime.speedFilter.realSpeed))
		{
			paramCB.runtime.speedFilter.proSpeed = 0;
		}
	}

}

void PARAM_CallBack_SpeedFilterProcess(uint32 param)
{

	if (paramCB.runtime.speedFilter.realSpeed > paramCB.runtime.speedFilter.proSpeed)
	{
		if (paramCB.runtime.speedFilter.difSpeed > 10)
		{
			paramCB.runtime.speedFilter.proSpeed = paramCB.runtime.speedFilter.realSpeed;
		}
		else
		{
			paramCB.runtime.speedFilter.proSpeed += paramCB.runtime.speedFilter.difSpeed;
		}

		if (paramCB.runtime.speedFilter.proSpeed >= paramCB.runtime.speedFilter.realSpeed)
		{
			paramCB.runtime.speedFilter.proSpeed = paramCB.runtime.speedFilter.realSpeed;
		}
	}
	else
	{
		if (paramCB.runtime.speedFilter.difSpeed > 10)
		{
			paramCB.runtime.speedFilter.proSpeed = paramCB.runtime.speedFilter.realSpeed;
		}
		else
		{
			if (paramCB.runtime.speedFilter.proSpeed >= paramCB.runtime.speedFilter.difSpeed)
			{
				paramCB.runtime.speedFilter.proSpeed -= paramCB.runtime.speedFilter.difSpeed;
			}
			else
			{
				paramCB.runtime.speedFilter.proSpeed = 0;
			}
		}	
	}

	PARAM_SetSpeed(paramCB.runtime.speedFilter.proSpeed);

	// 速度不为0，重置定时关机任务
	if (PARAM_GetSpeed() != 0)
	{
		STATE_ResetAutoPowerOffControl();
	}
	
}


/*********************************************************************
* 函 数 名: PARAM_Init
* 函数入参: void
* 函数出参: 无
* 返 回 值: uint32 
* 功能描述: 初始化参数
***********
* 修改历史:
*   1.修改作者: ZJ
*     修改日期: 2017年08月06日
*     修改描述: 新函数 		   
**********************************************************************/
void PARAM_Init(void)
{
	// 读取NVM
	NVM_Load();
	
	// 未格式化，则格式化
	if (!NVM_IsFormatOK())
	{
		// 保证擦写的时候有电
		POWER_ON();
		
		NVM_Format();
	}
	
	// 设置默认的运行数据
	PARAM_SetDefaultRunningData();

	// 注册计算骑行时间定时器
	TIMER_AddTask(TIMER_ID_PARAM_TIME,
					1000UL,
					PARAM_CALLBACK_CaculateRidingTime,
					0,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);
						
	// 注册里程计算定时器任务
	TIMER_AddTask(TIMER_ID_PARAM_CACULATETRIPDISTANCE,
					SET_PARAMCYCLETIME, 
					PARAM_CallBack_CaculateTripDistance,
					0,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

	// 启动速度显示滤波控制定时器
	TIMER_AddTask(TIMER_ID_PROTOCOL_SPEED_FILTER,
					SMOOTH_BASE_TIME,
					PARAM_CallBack_SpeedFilterProcess,
					TRUE,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

	// 注册间隔写数据到NVM定时器
	TIMER_AddTask(TIMTR_ID_NVM_WRITE_REQUEST,
					600000UL,
					NVM_CALLBACK_EnableWrite,
					0,
					TIMER_LOOP_FOREVER,
					ACTION_MODE_ADD_TO_QUEUE);

	// 擦写完恢复仪表上电时自锁先关的状态
	POWER_OFF();
	
}

/*********************************************************************
* 函 数 名: PARAM_Process
* 函数入参: void
* 函数出参: 无
* 返 回 值: void 
* 功能描述: 模块过程处理函数
***********
* 修改历史:
*   1.修改作者: ZJ
*     修改日期: 2017年08月06日
*     修改描述: 新函数 		   
**********************************************************************/
void PARAM_Process(void)
{
	// 参数需要重新计算时，计算
	if (paramCB.recaculateRequest && (RECORD_STATE_RECORDING == RECORD_GetState()))
	{
		// 清除计算请求
		paramCB.recaculateRequest = FALSE;
		
		// 计算速度类数据
		PARAM_CaculateSpeed();

		// 计算里程类数据
		PARAM_CaculateTripDistance();

		// 计算卡路里类数据
		PARAM_CaculateCalories();
		
		// 计算骑行时间类数据
		// 时间类数据由定时器调度
	}

	// 计算功率类数据
	paramCB.runtime.power = 1.0f * paramCB.runtime.battery.voltage * paramCB.runtime.battery.current / 1000000.0f;

	// NVM有写入请求，并且写入使能时，才可以写入
	if ((paramCB.nvmWriteRequest) && (paramCB.nvmWriteEnable))
	{
		paramCB.nvmWriteRequest = FALSE;
		paramCB.nvmWriteEnable = FALSE;

		// 重新复位写数据到NVM定时器
		TIMER_ResetTimer(TIMTR_ID_NVM_WRITE_REQUEST);
		
		// 差异化保存
		NVM_Save(FALSE);
	}
	
	// 判断是否需要恢复出厂设置
	if (!PARAM_GetResFactorySet())
	{
		PARAM_SetResFactorySet(TRUE);
		PARAM_FactoryReset();  
	}
	
}


// Trip清零
void PARAM_ClearTrip(void)
{
	// Trip里程清零
	paramCB.nvm.param.common.record.trip.distance = 0;

	// 平均速度和最高速度清零
	paramCB.nvm.param.common.record.trip.speedAverage = 0;
	paramCB.nvm.param.common.record.trip.speedMax = 0;

	// Trip骑行时间清零
	paramCB.nvm.param.common.record.trip.ridingTime = 0;

	// Trip骑行卡路里清零
	paramCB.nvm.param.common.record.trip.calories = 0;

	// NVM更新，立即写入
	NVM_SetDirtyFlag(TRUE);
	
}

uint8 PARAM_CmpareN(const uint8 *str1, const uint8 *str2, uint8 len)
{
	uint16 i = 0;

	while (i < len)
	{
		if (*(str1 +i) != *(str2 +i))
		{
			return 0;
		}
		i++;
	}
	return 1;
}



/*********************************************************************************
**********************************************************************************
************************获取相关里程数据******************************************
**********************************************************************************
**********************************************************************************/

// 获取速度接口
uint16 PARAM_GetSpeed(void)
{
	uint32 temp;
	
	switch (paramCB.nvm.param.common.unit)
	{
		case UNIT_METRIC:
			temp = paramCB.runtime.speed;
			break;

		case UNIT_INCH:
			temp = paramCB.runtime.speed;
			temp =(temp * METRIC_TO_INCH_COFF) +0.5f;
			break;

		default:
			temp = paramCB.runtime.speed;
			break;
	}

	return (uint16)temp;
}

// 获取Trip里程接口
uint32 PARAM_GetTrip(void)
{
	uint32 temp;
	
	switch (paramCB.nvm.param.common.unit)
	{
		case UNIT_METRIC:
			temp = paramCB.nvm.param.common.record.trip.distance;
			break;

		case UNIT_INCH:
			temp = paramCB.nvm.param.common.record.trip.distance;
			temp =(temp * METRIC_TO_INCH_COFF) +0.5f;
			break;

		default:
			temp = paramCB.nvm.param.common.record.trip.distance;
			break;
	}

	return temp;
}


// 获取Distance里程接口
uint32 PARAM_GetTotalDistance(void)
{
	uint32 temp;
	
	switch (paramCB.nvm.param.common.unit)
	{
		case UNIT_METRIC:
			temp = paramCB.nvm.param.common.record.total.distance;
			break;

		case UNIT_INCH:
			temp = paramCB.nvm.param.common.record.total.distance;
			temp =(temp * METRIC_TO_INCH_COFF) +0.5f;
			break;

		default:
			temp = paramCB.nvm.param.common.record.total.distance;
			break;
	}

	return temp;
}

// 获取平均速度里程接口
uint16 PARAM_GetTripAverageSpeed(void)
{
	uint32 temp;
	
	switch (paramCB.nvm.param.common.unit)
	{
		case UNIT_METRIC:
			temp = paramCB.nvm.param.common.record.trip.speedAverage;
			break;

		case UNIT_INCH:
			temp = paramCB.nvm.param.common.record.trip.speedAverage;
			temp =(temp * METRIC_TO_INCH_COFF) +0.5f;
			break;

		default:
			temp = paramCB.nvm.param.common.record.trip.speedAverage;
			break;
	}

	return (uint16)temp;
}


// 获取最大速度里程接口
uint16 PARAM_GetTripMaxSpeed(void)
{
	uint32 temp;
	
	switch (paramCB.nvm.param.common.unit)
	{
		case UNIT_METRIC:
			temp = paramCB.nvm.param.common.record.trip.speedMax;
			break;

		case UNIT_INCH:
			temp = paramCB.nvm.param.common.record.trip.speedMax;
			temp =(temp * METRIC_TO_INCH_COFF) +0.5f;
			break;

		default:
			temp = paramCB.nvm.param.common.record.trip.speedMax;
			break;
	}

	return (uint16)temp;
}

/*********************************************************************************
**********************************************************************************
**********************************************************************************
**********************************************************************************/
ASSIST_ID_E PARAM_GetAssistLevel(void)
{
	/*if (PARAM_IsPushAssistOn())
	{
		//return PROTOCOL_ASSIST_P;
	}
	else
	{
		return paramCB.runtime.assist;
	}*/
	return paramCB.runtime.assist;
}



// 获取档位PWM固定值
uint8 PARAM_GetPwmFixed(uint8 asist)
{
	if (asist >= ASSIST_ID_COUNT)
	{
		return 0;
	}

	return paramCB.nvm.param.protocol.pwmFixed[asist];
}

/*********************************************************************************
**********************************************************************************
**********************************************************************************
**********************************************************************************/

// 协议初始化选择
void PARAM_ProtocolInitSelect(void)
{
	
	#if  CONFIG_PROTOCOL_TYPE_KM5S	
	
			UART_DRIVE_Init(9600);

			// KM5S
			UART_PROTOCOL_Init();		
			
	#elif CONFIG_PROTOCOL_TYPE_LBNO_2

		
			UART_DRIVE_Init(9600);
			
			// 锂电2号
			UART_PROTOCOL1_Init();		
			
	#elif CONFIG_PROTOCOL_TYPE_BF 

		
			UART_DRIVE_Init(1200);
			
			// 八方V5.2
			UART_PROTOCOL2_Init();		
			
	#elif CONFIG_PROTOCOL_TYPE_J	

			UART_DRIVE_Init(9600);
			
			// J协议
			UART_PROTOCOL3_Init();		
			
	#else	

			UART_DRIVE_Init(9600);
			
			// 默认KM5S
			UART_PROTOCOL_Init();	
			
	#endif		
	
}

// 运行协议选择
void PARAM_ProtocolProcessSelect(void)
{

	#if  CONFIG_PROTOCOL_TYPE_KM5S	
			// KM5S
			UART_PROTOCOL_Process();
			
	#elif CONFIG_PROTOCOL_TYPE_LBNO_2
	
			// 锂电2号
			UART_PROTOCOL1_Process();		
			
	#elif CONFIG_PROTOCOL_TYPE_BF 

			// 八方V5.2
			UART_PROTOCOL2_Process();		


	#elif CONFIG_PROTOCOL_TYPE_J

			// J协议
			UART_PROTOCOL3_Process();	

	#else	
		    // 默认KM5S
			UART_PROTOCOL_Process();
			
	#endif		
			
}









