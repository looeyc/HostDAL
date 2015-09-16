/*
 * protocol.c
 *
 *  Created on: 2014-2-19
 *      Author: root
 */
#include <stdio.h>
#include <memory.h>
#include "debuglog.h"
#include "protocol.h"
#include "devicedb_zigbee.h"
#include "dal_devtype.h"
#include "profiles.h"

//将数据库的设备关键字结构转换为DAL的设备ID
//pDALDeviceID - 调用者提供空间，用于返回转换得到的DAL设备ID
//pDevDB_DeviceKeyWords - 需要转换的设备关键字
//return 0-成功，else-失败
int fDevDBKeyWordsToDALDeviceID(stDALDeviceID *pDALDeviceID, stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords)
{
	int rt,Err=0;
	unsigned int DALDevType=DAL_UNKNOWNDEV;
	stDevKeyInfo_ZigBee DevKeyInfo_ZigBee;

	if((pDALDeviceID==NULL)||(pDevDB_DeviceKeyWords==NULL)){
		return -1;
	}

	//按照DAL设备归类进行分类，确定DALDevType取值
	switch(pDevDB_DeviceKeyWords->DeviceType){
	case DEVTYPE_ZIGBEE:
		rt=fParseKeyInfo_ZigBee(&DevKeyInfo_ZigBee,pDevDB_DeviceKeyWords->DeviceKeyInfo);
		if(rt!=0){
			DEBUGLOG1(3,"Error, fParseKeyInfo_ZigBee failed %d.\n",rt);
			Err=1;
		}else{
			switch(DevKeyInfo_ZigBee.ProfileID){
			case PROFILE_HA:
				switch(DevKeyInfo_ZigBee.DeviceID){
				case DEV_HA_ON_OFF_LIGHT:	DALDevType=DAL_ONOFFLIGHT;			break;
				case DEV_HA_IAS_ZONE:		DALDevType=DAL_IRINTRUSIONALARM;	break;
				default:
					DEBUGLOG1(3,"Error, unknown DeviceID 0x%04X.\n",DevKeyInfo_ZigBee.DeviceID);
					Err=1;
					break;
				}
				break;
			case PROFILE_ITS:
				switch(DevKeyInfo_ZigBee.DeviceID){
				case DEV_ITS_VEHICLE_PRESENCE_SENSOR_GEOMAG:	DALDevType=DAL_VEHICLEPRESENCESENSORGEOMAG;		break;
				case DEV_ITS_ROUTER_REPEATER:					DALDevType=DAL_ITS_ROUTER_REPEATER;				break;
				default:
					DEBUGLOG1(3,"Error, unknown DeviceID 0x%04X.\n",DevKeyInfo_ZigBee.DeviceID);
					Err=1;
					break;
				}
				break;
			default:
				DEBUGLOG1(3,"Error, unknown ProfileID 0x%04X.\n",DevKeyInfo_ZigBee.ProfileID);
				Err=1;
				break;
			}
		}
		break;
	default:
		DEBUGLOG1(3,"Error, unknown DeviceType 0x%08X.\n",pDevDB_DeviceKeyWords->DeviceType);
		Err=1;
		break;
	}

	memcpy(pDALDeviceID->DeviceID,&DALDevType,4);
	memcpy(pDALDeviceID->DeviceID+4,&pDevDB_DeviceKeyWords->DeviceType,4);
	memcpy(pDALDeviceID->DeviceID+8,pDevDB_DeviceKeyWords->DeviceKeyInfo,KEYINFOLEN);

	return Err;
}

//将DAL的设备ID转换为数据库的设备关键字结构
//pDevDB_DeviceKeyWords - 调用者提供空间,用于返回转换得到的设备关键字
//pDALDeviceID - 需要转换的DAL设备ID
int fDALDeviceIDToDevDBKeyWords(stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords, stDALDeviceID *pDALDeviceID)
{
	if((pDALDeviceID==NULL)||(pDevDB_DeviceKeyWords==NULL)){
		return -1;
	}

	memcpy(&pDevDB_DeviceKeyWords->DeviceType,pDALDeviceID->DeviceID+4,4);
	memcpy(pDevDB_DeviceKeyWords->DeviceKeyInfo,pDALDeviceID->DeviceID+8,KEYINFOLEN);

	return 0;
}
























