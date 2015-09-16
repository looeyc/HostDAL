/*
 * devicedb_zigbee.c
 *
 *  Created on: 2014-2-19
 *      Author: chengm
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "debuglog.h"
#include "alige.h"
#include "devicedb_global.h"
#include "devicedb_zigbee.h"

//创建一个ZigBee节点
//return 返回创建的节点指针，NULL-创建失败
stDevNode_ZigBee *fCreateDevNode_ZigBee()
{
	stDevNode_ZigBee *pRet=NULL;

	pRet=(stDevNode_ZigBee *)malloc(sizeof(stDevNode_ZigBee));
	if(pRet==NULL){
		DEBUGLOG0(3,"Error, malloc failed.\n");
		return NULL;
	}
	memset(pRet,0,sizeof(stDevNode_ZigBee));
	return pRet;
}

//销毁一个ZigBee节点
void fDestroyDevNode_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee)
{
	if(pDevNode_ZigBee!=NULL){
		free(pDevNode_ZigBee);
	}
}

//克隆一个ZigBee设备节点
//pDevNode_ZigBee - 需要克隆的设备节点
//return 返回克隆的节点，NULL-克隆失败
//克隆所得的节点在使用之后要调用fDestroyDevNode_ZigBee销毁
stDevNode_ZigBee *fCloneDevNode_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee)
{
	stDevNode_ZigBee *pDevNode_ZigBeeClone=NULL;

	if(pDevNode_ZigBee==NULL){
		return NULL;
	}

	pDevNode_ZigBeeClone=fCreateDevNode_ZigBee();
	if(pDevNode_ZigBeeClone==NULL){
		return NULL;
	}

	//复制所有内容，目前不存在级联复制
	*pDevNode_ZigBeeClone=*pDevNode_ZigBee;

	return pDevNode_ZigBeeClone;
}

////将一个ZigBee节点的数据结构转换为字节流形式，便于传输和存储
////pDevNode_ZigBee - 待转换的ZigBee节点的数据结构指针
////BufferLen - 缓存区长度
////pBuffer - 输出缓存区指针
////return >0转换所得的数据长度，<=0失败
//int fStToBytes_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee, int BufferLen, char *pBuffer)
//{
//	int i,Len=0;
//	char *pWrite=pBuffer;
//
//	if(pDevNode_ZigBee==NULL){
//		return -1;
//	}
//	if((BufferLen<=0)||(pBuffer==NULL)){
//		return -1;
//	}
//
//	//计算转换所得的字节流数据所需要占用的存储区长度
//	Len=8+MAXUSERDESCRIPTORLEN+9;
//	for(i=0;i<pDevNode_ZigBee->NumInClusters;i++){
//		Len+=3;
//		Len+=pDevNode_ZigBee->InClusterList[i].AttriNum*2;
//	}
//	for(i=0;i<pDevNode_ZigBee->NumOutClusters;i++){
//		Len+=3;
//		Len+=pDevNode_ZigBee->OutClusterList[i].AttriNum*2;
//	}
//	if(Len>BufferLen){
//		DEBUGLOG2(3,"Error, not enough buffer space %d<%d.\n",BufferLen,Len);
//		return -1;
//	}
//
//	//转换
//	memcpy(pWrite,pDevNode_ZigBee->IEEEAddr,8);pWrite+=8;
//	memcpy(pWrite,pDevNode_ZigBee->UserDescriptor,MAXUSERDESCRIPTORLEN);pWrite+=MAXUSERDESCRIPTORLEN;
//	UNALIGEWRITE_16BIT(pWrite,pDevNode_ZigBee->NwkAddr);pWrite+=2;
//	UNALIGEWRITE_16BIT(pWrite,pDevNode_ZigBee->ProfileID);pWrite+=2;
//	UNALIGEWRITE_16BIT(pWrite,pDevNode_ZigBee->DeviceID);pWrite+=2;
//	memcpy(pWrite,&pDevNode_ZigBee->EP,1);pWrite+=1;
//	memcpy(pWrite,&pDevNode_ZigBee->NumInClusters,1);pWrite+=1;
//	memcpy(pWrite,&pDevNode_ZigBee->NumOutClusters,1);pWrite+=1;
//	for(i=0;i<pDevNode_ZigBee->NumInClusters;i++){
//		UNALIGEWRITE_16BIT(pWrite,pDevNode_ZigBee->InClusterList[i].ClusterID);pWrite+=2;
//		memcpy(pWrite,&pDevNode_ZigBee->InClusterList[i].AttriNum,1);pWrite+=1;
//		memcpy(pWrite,pDevNode_ZigBee->InClusterList[i].AttriIDList,pDevNode_ZigBee->InClusterList[i].AttriNum*2);pWrite+=pDevNode_ZigBee->InClusterList[i].AttriNum*2;
//	}
//	for(i=0;i<pDevNode_ZigBee->NumOutClusters;i++){
//		UNALIGEWRITE_16BIT(pWrite,pDevNode_ZigBee->OutClusterList[i].ClusterID);pWrite+=2;
//		memcpy(pWrite,&pDevNode_ZigBee->OutClusterList[i].AttriNum,1);pWrite+=1;
//		memcpy(pWrite,pDevNode_ZigBee->OutClusterList[i].AttriIDList,pDevNode_ZigBee->OutClusterList[i].AttriNum*2);pWrite+=pDevNode_ZigBee->OutClusterList[i].AttriNum*2;
//	}
//
//	return Len;
//}
//
////将一个ZigBee节点从字节流形式的数据转换为结构体形式，从传输和存储中恢复
////pDevNode_ZigBee - 调用者提供空间，事先由fCreateDevNode_ZigBee创建，用于返回转换结果
////BufferLen - 字节流数据长度
////pBuffer - 字节流存储区
////return 0-成功，else-失败
//int fBytesToSt_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee, int BufferLen, char *pBuffer)
//{
//	int i,Len=0;
//	char *pRead=pBuffer;
//
//	if(pDevNode_ZigBee==NULL){
//		return -1;
//	}
//	if((BufferLen<=0)||(pBuffer==NULL)){
//		return -1;
//	}
//
//	//计算转换长度
//	Len=8+MAXUSERDESCRIPTORLEN+9;
//	for(i=0;i<pDevNode_ZigBee->NumInClusters;i++){
//		Len+=3;
//		Len+=pDevNode_ZigBee->InClusterList[i].AttriNum*2;
//	}
//	for(i=0;i<pDevNode_ZigBee->NumOutClusters;i++){
//		Len+=3;
//		Len+=pDevNode_ZigBee->OutClusterList[i].AttriNum*2;
//	}
//	if(Len!=BufferLen){
//		DEBUGLOG2(3,"Error, bad len if data %d!=%d.\n",Len,BufferLen);
//		return -1;
//	}
//
//	//转换
//	memcpy(&pDevNode_ZigBee->IEEEAddr,pRead,8);pRead+=8;
//	memcpy(pDevNode_ZigBee->UserDescriptor,pRead,MAXUSERDESCRIPTORLEN);pRead+=MAXUSERDESCRIPTORLEN;
//	pDevNode_ZigBee->NwkAddr=UNALIGEREAD_16BIT(pRead);pRead+=2;
//	pDevNode_ZigBee->ProfileID=UNALIGEREAD_16BIT(pRead);pRead+=2;
//	pDevNode_ZigBee->DeviceID=UNALIGEREAD_16BIT(pRead);pRead+=2;
//	pDevNode_ZigBee->EP=*pRead;pRead+=1;
//	pDevNode_ZigBee->NumInClusters=*pRead;pRead+=1;
//	pDevNode_ZigBee->NumOutClusters=*pRead;pRead+=1;
//	for(i=0;i<pDevNode_ZigBee->NumInClusters;i++){
//		pDevNode_ZigBee->InClusterList[i].ClusterID=UNALIGEREAD_16BIT(pRead);pRead+=2;
//		pDevNode_ZigBee->InClusterList[i].AttriNum=*pRead;pRead+=1;
//		memcpy(pDevNode_ZigBee->InClusterList[i].AttriIDList,pRead,pDevNode_ZigBee->InClusterList[i].AttriNum*2);pRead+=pDevNode_ZigBee->InClusterList[i].AttriNum*2;
//	}
//	for(i=0;i<pDevNode_ZigBee->NumOutClusters;i++){
//		pDevNode_ZigBee->OutClusterList[i].ClusterID=UNALIGEREAD_16BIT(pRead);pRead+=2;
//		pDevNode_ZigBee->OutClusterList[i].AttriNum=*pRead;pRead+=1;
//		memcpy(pDevNode_ZigBee->OutClusterList[i].AttriIDList,pRead,pDevNode_ZigBee->OutClusterList[i].AttriNum*2);pRead+=pDevNode_ZigBee->OutClusterList[i].AttriNum*2;
//	}
//
//	return 0;
//}

//将pDevKeyInfo_ZigBee结构数据转换为ZigBee关键字数据
//pDevKeyInfo_ZigBee - 数据结构
//pKeyInfo - 调用者提供空间，返回转换结果，长度为16字节，ZigBee关键字数据，该数据对于ZigBee而言是唯一的
//return 0-成功，else-失败
int fGenKeyInfo_ZigBee(unsigned char *pKeyInfo,stDevKeyInfo_ZigBee *pDevKeyInfo_ZigBee)
{
	if((pDevKeyInfo_ZigBee==NULL)||(pKeyInfo==NULL)){
		return -1;
	}

	//对于ZigBee设备，其关键信息的16各字节分别为IEEE[0]:...:IEEE[7]:EP:PROFILE[0]:PROFILE[1]:DEVICE[0]:DEVICE[1]，剩下的空余部分必须填为0
	memset(pKeyInfo,0,KEYINFOLEN);
	memcpy(pKeyInfo,pDevKeyInfo_ZigBee->IEEEAddr,8);
	memcpy(pKeyInfo+8,&pDevKeyInfo_ZigBee->EP,1);
	memcpy(pKeyInfo+9,&pDevKeyInfo_ZigBee->ProfileID,2);
	memcpy(pKeyInfo+11,&pDevKeyInfo_ZigBee->DeviceID,2);

	return 0;
}

//解析ZigBee关键字数据，形成stDevKeyInfo_ZigBee数据结构
//pDevKeyInfo_ZigBee - 调用者提供空间，设备关键字信息
//pKeyInfo - 待转换结果，长度为16字节，ZigBee关键字数据，该数据对于ZigBee而言是唯一的
//return 0-成功，else-失败
int fParseKeyInfo_ZigBee(stDevKeyInfo_ZigBee *pDevKeyInfo_ZigBee, unsigned char *pKeyInfo)
{
	if((pDevKeyInfo_ZigBee==NULL)||(pKeyInfo==NULL)){
		return -1;
	}

	memcpy(pDevKeyInfo_ZigBee->IEEEAddr,pKeyInfo,8);
	memcpy(&pDevKeyInfo_ZigBee->EP,pKeyInfo+8,1);
	memcpy(&pDevKeyInfo_ZigBee->ProfileID,pKeyInfo+9,2);
	memcpy(&pDevKeyInfo_ZigBee->DeviceID,pKeyInfo+11,2);

	return 0;
}

//生成ZigBee设备节点的关键字
//pDevNode_ZigBee - ZigBee设备节点
//pKeyInfo - 调用者提供空间，长度为16字节，用于返回计算得到的ZigBee关键字数据，该数据对于ZigBee而言是唯一的
//return 0-成功，else-失败
int fGenKeyInfo_ZigBee2(stDevNode_ZigBee *pDevNode_ZigBee, unsigned char *pKeyInfo)
{
	int rt;
	stDevKeyInfo_ZigBee DevKeyInfo_ZigBee;

	if((pDevNode_ZigBee==NULL)||(pKeyInfo==NULL)){
		return -1;
	}

	memcpy(DevKeyInfo_ZigBee.IEEEAddr,pDevNode_ZigBee->IEEEAddr,8);
	DevKeyInfo_ZigBee.EP=pDevNode_ZigBee->EP;
	DevKeyInfo_ZigBee.ProfileID=pDevNode_ZigBee->ProfileID;
	DevKeyInfo_ZigBee.DeviceID=pDevNode_ZigBee->DeviceID;

	rt=fGenKeyInfo_ZigBee(pKeyInfo,&DevKeyInfo_ZigBee);
	return rt;
}

//将设备信息与设备节点进行比较
//pDevNode_ZigBee - 设备节点
//pSearchKey - 需要比较的设备信息
//return <0-出错，0-相符，else-不相符
int fMatchDeviceNode_ZigBee(stDevNode_ZigBee *pDevNode_ZigBee, stDevDB_SearchKey_ZigBee *pSearchKey)
{
	if((pDevNode_ZigBee==NULL)||(pSearchKey==NULL)){
		return -1;
	}

	if(pSearchKey->AddrType==0){//0-NwkAddr、1-IEEEAddr
		if(pSearchKey->unAddr.NwkAddr!=pDevNode_ZigBee->NwkAddr){
			return 1;
		}
	}else{
		if(	(pSearchKey->unAddr.IEEEAddr[0]!=pDevNode_ZigBee->IEEEAddr[0])||(pSearchKey->unAddr.IEEEAddr[1]!=pDevNode_ZigBee->IEEEAddr[1])||(pSearchKey->unAddr.IEEEAddr[2]!=pDevNode_ZigBee->IEEEAddr[2])||(pSearchKey->unAddr.IEEEAddr[3]!=pDevNode_ZigBee->IEEEAddr[3])||(pSearchKey->unAddr.IEEEAddr[4]!=pDevNode_ZigBee->IEEEAddr[4])||(pSearchKey->unAddr.IEEEAddr[5]!=pDevNode_ZigBee->IEEEAddr[5])||(pSearchKey->unAddr.IEEEAddr[6]!=pDevNode_ZigBee->IEEEAddr[6])||(pSearchKey->unAddr.IEEEAddr[7]!=pDevNode_ZigBee->IEEEAddr[7])){
			return 2;
		}
	}

	if(pSearchKey->doMatchEP){
		if(pSearchKey->EP!=pDevNode_ZigBee->EP){
			return 3;
		}
	}

	return 0;
}











