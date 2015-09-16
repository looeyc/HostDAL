/*
 * devicedb_zigbee.h
 *
 *  Created on: 2013-6-18
 *      Author: chengm
 */

#ifndef DEVICEDB_ZIGBEE_H_
#define DEVICEDB_ZIGBEE_H_

#include "znpzcl.h"

#define MAXUSERDESCRIPTORLEN	16//用户描述符最大长度(znp设备只支持16字节)
#define MAXATTRIIDNUM			16//每个Cluster所允许拥有的最大属性数量
//#define MAXCLUSTERNUM			32

//Cluster数据结构
typedef struct{
	unsigned short ClusterID;//ClusterID
	//unsigned char AttriNum;//属性数量，属性列表中有效的属性ID数量
	//unsigned short AttriIDList[MAXATTRIIDNUM];//属性列表
}stCluster;

//ZigBee设备的数据结构
typedef struct{
	unsigned char IEEEAddr[8];	//64bit IEEE address
	char UserDescriptor[MAXUSERDESCRIPTORLEN];
	unsigned short NwkAddr;		//16bit address
	unsigned short ProfileID;	//0x0104 for HA
	unsigned short DeviceID;
	unsigned char EP;			//End point
	unsigned char NumInClusters;//输入Cluster数量
	unsigned char NumOutClusters;//输出Cluster数量
	stCluster InClusterList[MAXCLUSTERNUM];//输入Cluster列表
	stCluster OutClusterList[MAXCLUSTERNUM];//输入Cluster列表
	unsigned char isRouteAvailable;//0-Route不可用、1-Route可用
	stRoute Route;//设备路由
}stDevNode_ZigBee;

//ZigBee设备的关键信息
typedef struct{
	unsigned char IEEEAddr[8];
	unsigned char EP;
	unsigned short ProfileID;
	unsigned short DeviceID;
}stDevKeyInfo_ZigBee;

//能唯一确定一个ZigBee设备的设备特征
typedef struct{
	unsigned char AddrType;//unAddr类型：0-NwkAddr、1-IEEEAddr
	union{
		unsigned short NwkAddr;
		unsigned char IEEEAddr[8];
	}unAddr;
	unsigned char doMatchEP;//0-不需要比较EP、1-需要比较EP
	unsigned char EP;//doMatchEP为0时此字段无效
}stDevDB_SearchKey_ZigBee;

//创建一个ZigBee节点
//return 返回创建的节点指针，NULL-创建失败
//创建所得的stDevNode_ZigBee节点在不需要的时候要调用fDestroyDevNode_ZigBee进行销毁
stDevNode_ZigBee *fCreateDevNode_ZigBee();

//销毁一个由fCreateDevNode_ZigBee创建的ZigBee节点
void fDestroyDevNode_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee);

//克隆一个ZigBee设备节点
//pDevNode_ZigBee - 需要克隆的设备节点
//return 返回克隆的节点，NULL-克隆失败
//克隆所得的节点在使用之后要调用fDestroyDevNode_ZigBee销毁
stDevNode_ZigBee *fCloneDevNode_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee);

//将一个ZigBee节点的数据结构转换为字节流形式，便于传输和存储
//pDevNode_ZigBee - 待转换的ZigBee节点的数据结构指针
//BufferLen - 缓存区长度
//pBuffer - 输出缓存区指针
//return >0转换所得的数据长度，<=0失败
int fStToBytes_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee, int BufferLen, char *pBuffer);

//将一个ZigBee节点从字节流形式的数据转换为结构体形式，从传输和存储中恢复
//pDevNode_ZigBee - 调用者提供空间，事先由fCreateDevNode_ZigBee创建，用于返回转换结果
//BufferLen - 字节流数据长度
//pBuffer - 字节流存储区
//return 0-成功，else-失败
int fBytesToSt_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee, int BufferLen, char *pBuffer);

//生成ZigBee设备节点的关键字
//pDevNode_ZigBee - ZigBee设备节点
//pKeyInfo - 调用者提供空间，长度为16字节，用于返回计算得到的ZigBee关键字数据，该数据对于ZigBee而言是唯一的
//return 0-成功，else-失败
int fGenKeyInfo_ZigBee(unsigned char *pKeyInfo,stDevKeyInfo_ZigBee *pDevKeyInfo_ZigBee);

//解析ZigBee关键字数据，形成stDevKeyInfo_ZigBee数据结构
//pDevKeyInfo_ZigBee - 调用者提供空间，设备关键字信息
//pKeyInfo - 待转换结果，长度为16字节，ZigBee关键字数据，该数据对于ZigBee而言是唯一的
//return 0-成功，else-失败
int fParseKeyInfo_ZigBee(stDevKeyInfo_ZigBee *pDevKeyInfo_ZigBee, unsigned char *pKeyInfo);

//生成ZigBee设备节点的关键字
//pDevNode_ZigBee - ZigBee设备节点
//pKeyInfo - 调用者提供空间，长度为16字节，用于返回计算得到的ZigBee关键字数据，该数据对于ZigBee而言是唯一的
//return 0-成功，else-失败
int fGenKeyInfo_ZigBee2(stDevNode_ZigBee *pDevNode_ZigBee, unsigned char *pKeyInfo);

//将设备信息与设备节点进行比较
//pDevNode_ZigBee - 设备节点
//pSearchKey - 需要比较的设备信息
//return <0-出错，0-相符，else-不相符
int fMatchDeviceNode_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee, stDevDB_SearchKey_ZigBee *pSearchKey);

#endif /* DEVICEDB_ZIGBEE_H_ */


