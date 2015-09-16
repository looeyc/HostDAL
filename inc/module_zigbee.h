/*
 * module_zigbee.h
 *
 *  Created on: 2014-2-10
 *      Author: chengm
 */

#ifndef MODULE_ZIGBEE_H_
#define MODULE_ZIGBEE_H_

#include "dal_errno.h"
#include "devicedb.h"
#include "hostdal.h"
#include "znpzcl.h"

//ZigBee模块的全局参数
typedef struct{
	unsigned char LocalIEEEAddr[8];//本地的64位IEEE地址

	stZclInst *pZclInst_HA;//为HA协议注册的Zcl实例
	stZclInst *pZclInst_ITS;//为ITS协议注册的Zcl实例
}stZigBeeMStatus;
extern stZigBeeMStatus GlobalZigBeeMStatus;

/*******************************************************************************************************************/
//ZigBeeM_Init																										|
//功能：子模块初始化																									|
/*******************************************************************************************************************/
//子模块初始化参数
typedef struct{
	char *pZNPComPath;//Linux下的ZNP串口设备路径，例如Ubuntu下的“/dev/ttyS0”

	//底层的ZNP配置参数，如果不关心的话，可以全部取0--------------------------------------------------------------
	char isValid_IEEEAddr;		//0-未指定、1-指定
	unsigned char IEEEAddr[8];	//对应的isValid_xxx为1时有效，如果为全0xFF，则表示使用不使用NV参数中的IEEEAddr
	//以下网络参数中任何一个被指定时，ZNP启动的时候都会将所有之前的网络状态NV参数清空，“一切从0开始”，所有的字节点记录都会被清掉
	//指定网络参数PANID
	char isValid_PANID;			//0-未指定、1-指定
	unsigned short PANID;		//对应的isValid_xxx为1时有效，如果为0xFFFF，则表示由协调器自己选择
	//指定网络参数Extended PANID
	char isValid_ExtPANID;		//0-未指定、1-指定
	unsigned char ExtPANID[8];	//对应的isValid_xxx为1时有效，如果为全0xFF，则表示
}stZigBeeMCfg;

DALStatus ZigBeeM_Init(stZigBeeMCfg *pZigBeeMCfg);

/*******************************************************************************************************************/
//ZigBeeM_GetModuleDetails																							|
//功能：获取关于子模块的详细参数																							|
/*******************************************************************************************************************/
//pZigBeeMDetails - 用于返回获取结果，
DALStatus ZigBeeM_GetModuleDetails(stDALSubMDetails_ZigBee *pZigBeeMDetails);

/*******************************************************************************************************************/
//ZigBeeM_AddDevice																									|
//功能：添加设备																										|
/*******************************************************************************************************************/
//pDeviceDesc_ZigBee - 需要添加的ZigBee设备描述符
DALStatus ZigBeeM_AddDevice(stDeviceDesc_ZigBee *pDeviceDesc_ZigBee);

/*******************************************************************************************************************/
//ZigBeeM_RemoveDevice																								|
//功能：删除指定设备																									|
/*******************************************************************************************************************/
//pDeviceNode - 设备节点数据
//pDevDB_DeviceKeyWords - 需要移除的设备关键字
DALStatus ZigBeeM_RemoveDevice(stDevDB_DeviceNode *pDeviceNode, stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords);

/*******************************************************************************************************************/
//ZigBeeM_DeviceCtrl																								|
//功能：控制指定设备																									|
/*******************************************************************************************************************/
DALStatus ZigBeeM_DeviceCtrl(stDevDB_DeviceNode *pDeviceNode, unsigned int DALDevType, stDALCtrl *pDALCtrl);

/*******************************************************************************************************************/
//ZigBeeM_GetDeviceStatus																							|
//功能：获取指定设备的DAL状态参数																							|
/*******************************************************************************************************************/
//pDeviceNode - 从数据库中获取的设备节点
//DALDevType - 设备所属的DAL设备类型
//pDALStatus - 调用者提供，用于返回获取到的状态参数，调用者通过AllocateDALStatus分配，并且在使用后通过DelocateDALStatus释放
//DAL_GetDeviceStatus为该接口的唯一使用者，其配套的AllocateDALStatus会分配pDALStatus作为该函数的传入参数，
DALStatus ZigBeeM_GetDeviceStatus(stDevDB_DeviceNode *pDeviceNode, unsigned int DALDevType, stDALStatus *pDALStatus);

#endif /* MODULE_ZIGBEE_H_ */


