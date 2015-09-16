/*
 * devicedb.h
 *
 *  Created on: 2013-6-18
 *      Author: chengm
 */

//设备数据库接口层，隔离具体的实现方式，例如简单的链表或者采用高级的mysql等专业数据库

#ifndef DEVICEDB_H_
#define DEVICEDB_H_

#include "devicedb_global.h"
#include "devicedb_zigbee.h"
#include "devicedb_webcam.h"
#include "devicedb_reptext.h"

#define MAXDEVICENAME		32//设备名称的最大长度（字节）
#define MAXUSRDESC			64//用户描述信息的最大长度（字节）

#define STATUS_OK		0	//正常
#define STATUS_FAILED	-1 //访问出错

//设备数据结构
typedef union{
	stDevNode_ZigBee DevNode_ZigBee;
	stDevNode_WebCam DevNode_WebCam;
	stDevNode_ReptExt DevNode_ReptExt;
}unDevNode;

//设备数据库中的设备节点数据结构
typedef struct{
	//设备唯一性标识
	stDevDB_DeviceKeyWords DeviceKeyWords;

	//共同部分-----------
	enDeviceType DeviceType;//设备类型
	char DeviceName[MAXDEVICENAME];//用户自定义的设备名称
	char UserDescript[MAXUSRDESC];//用户的描述信息

	//差别部分-----------
	unDevNode *pDeviceNode;//设备节点数据结构
}stDevDB_DeviceNode;

typedef struct{
	enDeviceType DeviceType;//设备类型
	union{
		stDevDB_SearchKey_ZigBee SearchKey_ZigBee;
	}un;
}stDevDB_SearchKey;

//----------------------------------------------------------------------------------------------------

//初始化
//return 0-成功，else-失败
int fDevDB_Init();

//创建一个设备节点
//DeviceType 需要创建的设备类型
//return 返回创建的设备节点，NULL-失败
//使用完毕后必须调用fDevDB_DestroyDeviceNode销毁
//所创建的设备节点不含任何实际有效的数据
stDevDB_DeviceNode *fDevDB_CreatDeviceNode(enDeviceType DeviceType);

//加入新设备
//pDeviceNode-待加入的设备节点数据,该函数会重新复制设备节点数据
//return 0-成功，else-失败
int fDevDB_AddDevice(stDevDB_DeviceNode *pDeviceNode);

//从数据库中删除设备
//pDevDB_DeviceKeyWords - 设备标识
//return 0-成功，else-失败
int fDevDB_RemoveDevice(stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords);

//查找设备，获取设备数据
//pDevDB_DeviceKeyWords - 设备标识
//return NULL-未找到，else-返回设备指针，这个函数将克隆找到的设备数据，使用后一定要调用fDevDB_DestroyDeviceNode销毁返回结果
stDevDB_DeviceNode *fDevDB_GetDeviceNode(stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords);

//返回指定类型的设备关键字列表
//DeviceType - 指定的设备类型
//StartIndex - 输入，开始位置，表示从第几个设备开始返回(从0开始)
//MaxNum - 输入，返回的最大数量
//ppKeyWordsList - 调用者提供空间，指向stDevDB_DeviceKeyWords pKeyWordsList[MaxNum]型的数组，用于返回获取到的关键字列表
//return <0-失败，else-返回的数量
int fDevDB_GetDeviceKeyWordsList(enDeviceType DeviceType, int StartIndex, int MaxNum, stDevDB_DeviceKeyWords *pKeyWordsList);

//克隆一个设备节点
//pDeviceNode - 指向设备节点
//return NULL-失败，else-返回复制的设备节点数据，使用后再不需要时，必须调用fDevDB_DestroyDeviceNode销毁
stDevDB_DeviceNode *fCloneDeviceNode(stDevDB_DeviceNode *pDeviceNode);

//销毁fDevDB_GetDeviceNode获得的一个设备节点，释放其所占用的存储资源
//pDeviceNode - 指向设备节点
void fDevDB_DestroyDeviceNode(stDevDB_DeviceNode *pDeviceNode);

//搜索符合指定特征的设备
//pSearchKey - 查找的设备特征
//Num - ppDevDB_DeviceNodeList能容纳的指针数量
//ppDevDB_DeviceNodeList - 调用者提供空间，用于返回找到的设备节点指针列表，指向(stDevDB_DeviceNode *)ppDevDB_DeviceNodeList[N]型指针数组
//return - 找到的设备节点数量，表示ppDevDB_DeviceNodeList数组中实际填入的指针数量，<0表示错误
//注意，
//1、ppDevDB_DeviceNodeList在使用后要调用fDevDB_DestroyDeviceNode逐一释放每一个元素指针，如下:
//	for(i=0;i<Num;i++) {fDevDB_DestroyDeviceNode(ppDevDB_DeviceNodeList[i]);}
//2、如果返回值等于Num，即提供的数组被填满，则有可能有更多的符合条件的设备
int fDevDB_FindDeviceNode(stDevDB_SearchKey *pSearchKey, int Num, stDevDB_DeviceNode *ppDevDB_DeviceNodeList[]);

//统计当前的设备总数量
//DeviceType - 指定统计的设备类型, DEVTYPE_UNKNOWN表示所有类型的设备
//return 统计结果，<0-表示失败
int fDevDB_Statistic_DeviceNum(enDeviceType DeviceType);

//用新数据替换原有设备数据
//pDevDB_DeviceKeyWords - 设备标识
//pDevDB_DeviceNode - 对应的数据将用于更新数据库中的设备数据
//return 0-成功，else-失败
int fDevDB_ReplaceDevice(stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords,stDevDB_DeviceNode *pDevDB_DeviceNode);

//保存数据，下次启动时会自动恢复
//return 0-成功，else-失败
int fDevDB_Save();

#endif /* DEVICEDB_H_ */




