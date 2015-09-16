/*
 * protocol.h
 *
 *  Created on: 2014-2-19
 *      Author: chengm
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "devicedb_global.h"
#include "hostdal.h"

//将数据库的设备关键字结构转换为DAL的设备ID
//pDALDeviceID - 调用者提供空间，用于返回转换得到的DAL设备ID
//pDevDB_DeviceKeyWords - 需要转换的设备关键字
//return 0-成功，else-失败
int fDevDBKeyWordsToDALDeviceID(stDALDeviceID *pDALDeviceID,stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords);

//将DAL的设备ID转换为数据库的设备关键字结构
//pDevDB_DeviceKeyWords - 调用者提供空间,用于返回转换得到的设备关键字
//pDALDeviceID - 需要转换的DAL设备ID
//return 0-成功、else-失败
int fDALDeviceIDToDevDBKeyWords(stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords,stDALDeviceID *pDALDeviceID);

#endif /* PROTOCOL_H_ */
