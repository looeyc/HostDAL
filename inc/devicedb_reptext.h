/*
 * devicedb_reptext.h
 *
 *  Created on: 2013-9-30
 *      Author: chengm
 */

#ifndef DEVICEDB_REPTEXT_H_
#define DEVICEDB_REPTEXT_H_

#include "devicedb_global.h"

#define MAXREPTEXTCTRLENTRYNUM	64
#define MAXREPTEXTCTRLCMDLEN	32
#define MAXREPTEXTCTRLNAMELEN	32

//转发器扩展设备类型
typedef enum{
	AIR_CONDITIONER,//空调设备
	TV_SET,//电视机
	USER_DEFINED,//自定义设备
	UNKNOWN
}enReptExtType;

typedef struct{
	int CtrlCmdLen;//CtrlCmd中的实际cmd数据长度,应当<MAXREPTEXTCTRLCMDLEN
	char CtrlCmd[MAXREPTEXTCTRLCMDLEN];
	char CtrlName[MAXREPTEXTCTRLNAMELEN];//必须以'\0'结束
}stRepExtCtrlEntry;

//ReptExt(转发器扩展)设备的数据结构，转发器扩展设备是指由转发器扩展出来的一类虚拟设备，一个转发器可以扩展出多个这种设备
typedef struct{
	stDevDB_DeviceKeyWords ParentRepeaterKeyWords;//所属转发器的设备关键字
	enReptExtType ReptExtType;//扩展设备类型
	int UniqIndex;//唯一性索引编号，当网络中存在多个同类型的转发器扩展设备，例如多台空调，多台电视机等时，通过该索引值来区分
	int CtrlEntryNum;//实际拥有的控制入口数量
	stRepExtCtrlEntry CtrlEntry[MAXREPTEXTCTRLENTRYNUM];//控制入口列表
}stDevNode_ReptExt;

//销毁一个ReptExt节点
void fDestroyDevNode_ReptExt(stDevNode_ReptExt *pDevNode_ReptExt);

#endif /* DEVICEDB_REPTEXT_H_ */


