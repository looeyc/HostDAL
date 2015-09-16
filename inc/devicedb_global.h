/*
 * devicedb_global.h
 *
 *  Created on: 2013-9-30
 *      Author: chengm
 */

#ifndef DEVICEDB_GLOBAL_H_
#define DEVICEDB_GLOBAL_H_

#define KEYINFOLEN				16
#define MAXDEVKEYWORDSLEN		16
#define LENOFDEVICEKEYWORDS		(sizeof(stDevDB_DeviceKeyWords))

//设备类型
typedef enum{
	DEVTYPE_UNKNOWN,
	DEVTYPE_ZIGBEE,//ZigBee设备
	DEVTYPE_WEBCAM,//WebCam网络摄像机设备
	DEVTYPE_REPTEXT//转发器扩展设备
}enDeviceType;

//设备的唯一标识
typedef struct{
	enDeviceType DeviceType;//设备类型
	unsigned char DeviceKeyInfo[KEYINFOLEN];
}stDevDB_DeviceKeyWords;

#endif /* DEVICEDB_GLOBAL_H_ */
