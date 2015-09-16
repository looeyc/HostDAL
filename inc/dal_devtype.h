/*
 * dal_devtype.h
 *
 *  Created on: 2014-2-19
 *      Author: chengm
 */

#ifndef DAL_DEVTYPE_H_
#define DAL_DEVTYPE_H_

//DAL_UNKNOWNDEV----------------------------------------------------------------------------
//设备类型编码
#define DAL_UNKNOWNDEV								(unsigned int)0x00000000

//DAL_ONOFFLIGHT----------------------------------------------------------------------------
//设备类型编码
#define DAL_ONOFFLIGHT								(unsigned int)0xCAFE0001
//设备操作编码
#define DAL_ONOFFLIGHT_ONOFFCTRL					(unsigned char)0x00
//设备事件编码
#define DAL_ONOFFLIGHT_EVENT_ONOFF					(unsigned char)0x00

//DAL_IRINTRUSIONALARM----------------------------------------------------------------------
//设备类型编码
#define DAL_IRINTRUSIONALARM						(unsigned int)0xCAFE0502
//设备操作编码
#define DAL_IRINTRUSIONALARM_TEST					(unsigned char)0x00
//设备事件编码
#define DAL_IRINTRUSIONALARM_INTRUSION				(unsigned char)0x00
#define DAL_IRINTRUSIONALARM_LOWBATTERY				(unsigned char)0x01

//DAL_VEHICLE_PRESENCE_SENSOR_GEOMAG--------------------------------------------------------
//设备类型编码
#define DAL_VEHICLEPRESENCESENSORGEOMAG				(unsigned int)0xCBFE0001
//设备操作编码
#define DAL_VEHICLEPRESENCESENSORGEOMAG_ADJUST		(unsigned char)0x00
#define DAL_VEHICLEPRESENCESENSORGEOMAG_CONFIG		(unsigned char)0x01
//设备事件编码
#define DAL_VEHICLEPRESENCESENSORGEOMAG_PARKING		(unsigned char)0x00
#define DAL_VEHICLEPRESENCESENSORGEOMAG_LOWBETTERY	(unsigned char)0x01
#define DAL_VEHICLEPRESENCESENSORGEOMAG_STATEREPORT	(unsigned char)0x02

//DAL_ROUTER_REPEATER
//设备类型编码
#define DAL_ITS_ROUTER_REPEATER						(unsigned int)0xCBFE0101
//设备操作编码
//无
//设备事件编码
#define DAL_ITS_ROUTER_REPEATER_STATEREPORT			(unsigned char)0x00

#endif /* DAL_DEVTYPE_H_ */




