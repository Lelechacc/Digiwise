#include "dtconfig.h"

// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// ■■■■■■■■■■■■■	 以下内容非嵌软人员禁止修改    ■■■■■■■■
// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// 公制转英制表
const uint8 limitKmhToMph[100] ={//0 1 2 3 4 5 6 7 8 9
									0,0,1,2,2,3,4,4,5,5,
									
									6,7,7,8,9,9,10,10,11,12,
									
									12,13,13,14,15,16,16,17,17,18,
					
									18,19,20,20,21,22,22,23,23,24,

									25,25,26,27,27,28,28,29,30,30,

									31,31,32,33,33,34,35,35,36,36,

									37,38,38,39,40,40,41,41,42,43,

									43,44,45,45,46,46,47,48,48,49,

									49,50,51,51,52,53,53,54,54,55,

									56,56,57,58,58,59,59,60,61,62,
									
									};


// 英制转公制表
const uint8 limitMphToKmp[63] ={	//0 1 2 3 4 5 6  7	8  9
									0,2,3,5,6,8,10,11,13,14,
									
									16,18,19,21,23,24,25,27,29,31,
									
									32,34,35,37,39,40,42,43,45,47,
									
									48,50,52,53,55,56,58,60,61,63,
									
									64,66,68,69,71,72,74,76,77,79,
									
									81,82,84,85,87,89,90,92,93,95,
									
									97,98,99,									
									};
// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
/*******************************************************************************
********************************************************************************
******************************菜单库配置****************************************
********************************************************************************
********************************************************************************/
// 菜单配置
uint8 Config_menu[CONFIG_MENU_MAX_ID] =
										{	
											MENU_ID_CLEARTRIP,			// 清除里程
											MENU_ID_UNIT,					// 单位
											MENU_ID_SPEED_LIMIT,			// 速度
											MENU_ID_AUTO_POWER_OFF_TIME,	// 关机时间
											MENU_ID_BACKLIGHT_BRIGHTNESS,	// 背光
										   // MENU_ID_LEVEL_PER,			// 每档助力百分比
										   // MENU_ID_STEEL,				// 测试磁钢
											//MENU_ID_CRUIS,				// 定速巡航
											//MENU_ID_RESET,				// 恢复出厂	
						
								  		};

// 高级菜单配置
uint8 Config_advMenu[CONFIG_ADVMENU_MAX_ID] =
											{	
												MENU_ID_SIZE,				// 轮径 
												MENU_ID_CY,					// 周长
												MENU_ID_BAT, 				// 额定电压
												MENU_ID_BAT_U,				// 欠压
												MENU_ID_EU_A,				// 限流
												MENU_ID_RANGE,				// 最大档位
																				
											};



// 关机时间
uint8 Config_PowerOffTime[CONFIG_POWER_OFFTIME_MAX]=
													 {
													  	0,
													 	3,
													 	5,
													  	10,
													  	//15,
													  	30,
													 	60
													 };


// 关机时间
uint8 Config_lightLevel[CONFIG_LIGHTLEVEL_MAX]=
												{
												 	BACK_LIGHT_LEVEL_20_100,
												 	BACK_LIGHT_LEVEL_40_100,
												 	BACK_LIGHT_LEVEL_60_100,
												 	BACK_LIGHT_LEVEL_80_100,
												 	BACK_LIGHT_LEVEL_ON,
												};

// 轮径ID
uint16 Config_wheelIdSize[CONFIG_WHELLID_MAX][CONFIG_WHELLSIZE_MAX]=  
													{
														PARAM_WHEEL_SIZE_16_INCH,160, 
														PARAM_WHEEL_SIZE_18_INCH,180,
														PARAM_WHEEL_SIZE_20_INCH,200,
														PARAM_WHEEL_SIZE_22_INCH,220,		
												
												  	};



// 电压等级 欠压值
uint16 Config_VoltageLevel[CONFIG_VOLTLEVET_X_MAX][CONFIG_VOLTLEVET_Y_MAX] =
																			{
																			  	24,210,
																			  	36,315,
																			  	48,420,
																			 	52,480,
																			  	60,520,	
																			  	72,600,
															 				 };
// 最大档位
uint8 Config_MaxAssit[CONFIG_MAXASSIT_NUM]=
											{
											  3,
											  4,
											  5,
											  9,					
											 };


