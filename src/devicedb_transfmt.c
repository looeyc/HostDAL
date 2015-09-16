/*
 * devicedb_transfmt.c
 *
 *  Created on: 2013-6-29
 *      Author: chengm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debuglog.h"
#include "devicedb_zigbee.h"
#include "devicedb.h"
#include "devicedb_transfmt.h"

//将数据库链表节点转换为字节流，便于存储、传输
/**************************************************LIST********************************************************
fStToBin_stDevNode_ZigBee		//将stDevNode_ZigBee结构体数据转换为连续的二进制数据，方便存储和传输
fBinToSt_stDevNode_ZigBee		//将连续的二进制数据转换为stDevNode_ZigBee结构体数据，从传输或者存储的数据中恢复数据结构
fStToBin_stDevNode_WebCam		//将stDevNode_WebCam结构体数据转换为连续的二进制数据，方便存储和传输
fBinToSt_stDevNode_WebCam		//将连续的二进制数据转换为stDevNode_WebCam结构体数据，从传输或者存储的数据中恢复数据结构
fStToBin_stDevNode_ReptExt		//将stDevNode_ReptExt结构体数据转换为连续的二进制数据，方便存储和传输
fBinToSt_stDevNode_ReptExt		//将连续的二进制数据转换为stDevNode_ReptExt结构体数据，从传输或者存储的数据中恢复数据结构
fStToBin_stDevDB_DeviceNode		//将设备节点数据结构转换为连续的二进制形式，便于存储和传输
fBinToSt_stDevDB_DeviceNode		//将设备节点数据的连续二进制形式恢复为数据结构形式
***************************************************************************************************************/

//将stDevNode_WebCam结构体数据转换为连续的二进制数据，方便存储和传输
//pDevNode_WebCam - 待转换的结构体数据
//pBuffer - 调用者提供空间，存储转换结果
//BufferLen - 提供的存储空间长度（字节）
//return  <=0-出错，else-实际转换出来的数据长度（字节）
int fStToBin_stDevNode_WebCam(stDevNode_WebCam *pDevNode_WebCam, char *pBuffer, int BufferLen)
{
	int WriteLen,DataLen=0;

	if((pDevNode_WebCam==NULL)||(pBuffer==NULL)||(BufferLen<=0)){
		return -1;
	}

	WriteLen=sizeof(stDevNode_WebCam);
	if(BufferLen<WriteLen){
		DEBUGLOG0(3,"Error, not enough memory, need a bigger buffer.\n");
		return -2;
	}
	memcpy(pBuffer,pDevNode_WebCam,WriteLen);
	DataLen=WriteLen;

	return DataLen;
}

//将连续的二进制数据转换为stDevNode_WebCam结构体数据，从传输或者存储的数据中恢复数据结构
//pBuffer - 待转换的二进制数据存储区
//DataLen - 数据长度（字节）
//return - NULL-失败，else-恢复出来的设备数据结构，
//注意：该函数会分配存储空间，使用后一定要调用fDestroyDevNode_WebCam销毁！
stDevNode_WebCam *fBinToSt_stDevNode_WebCam(char *pData, int DataLen)
{
	int TempLen;
	stDevNode_WebCam *pDevNode_WebCam=NULL;

	if((pData==NULL)||(DataLen<sizeof(stDevNode_WebCam))){
		return NULL;
	}

	TempLen=sizeof(stDevNode_WebCam);
	pDevNode_WebCam=malloc(TempLen);
	if(pDevNode_WebCam==NULL){
		DEBUGLOG0(3,"Error, malloc failed.\n");
		return NULL;
	}
	memcpy(pDevNode_WebCam,pData,TempLen);

	return pDevNode_WebCam;
}

//将stDevNode_ReptExt结构体数据转换为连续的二进制数据，方便存储和传输
//pDevNode_ReptExt - 待转换的结构体数据
//pBuffer - 调用者提供空间，存储转换结果
//BufferLen - 提供的存储空间长度（字节）
//return  <=0-出错，else-实际转换出来的数据长度（字节）
int fStToBin_stDevNode_ReptExt(stDevNode_ReptExt *pDevNode_ReptExt, char *pBuffer, int BufferLen)
{
	int WriteLen;

	if((pDevNode_ReptExt==NULL)||(pBuffer==NULL)||(BufferLen<=0)){
		return -1;
	}

	WriteLen=sizeof(stDevNode_ReptExt)-(MAXREPTEXTCTRLENTRYNUM-pDevNode_ReptExt->CtrlEntryNum)*sizeof(stRepExtCtrlEntry);
	if(WriteLen<=0){
		DEBUGLOG0(3,"Error, the WriteLen should not be a negtive value.\n");
		return -2;
	}
	if(WriteLen>BufferLen){
		DEBUGLOG2(3,"Error, buffer is too small to hold all the data %d<%d.\n",BufferLen,WriteLen);
		return -3;
	}

	memcpy(pBuffer,pDevNode_ReptExt,WriteLen);
	return WriteLen;
}

//将连续的二进制数据转换为stDevNode_ReptExt结构体数据，从传输或者存储的数据中恢复数据结构
//pBuffer - 待转换的二进制数据存储区
//DataLen - 数据长度（字节）
//return - NULL-失败，else-恢复出来的设备数据结构，
//注意：该函数会分配存储空间，使用后一定要调用fDestroyDevNode_ReptExt销毁！
stDevNode_ReptExt *fBinToSt_stDevNode_ReptExt(char *pData, int DataLen)
{
	int Len;
	stDevNode_ReptExt *pDevNode_ReptExt=NULL;

	if((pData==NULL)||(DataLen<=0)){
		return NULL;
	}

	//检查数据长度
	if(DataLen>sizeof(stDevNode_ReptExt)){
		DEBUGLOG0(2,"Warning, the lenth of input data is bigger than the maxim lenth of a stDevNode_ReptExt, truncating!\n");
		Len=sizeof(stDevNode_ReptExt);
	}else{
		Len=DataLen;
	}
	//分配空间
	pDevNode_ReptExt=malloc(sizeof(stDevNode_ReptExt));
	if(pDevNode_ReptExt==NULL){
		DEBUGLOG0(3,"Error, malloc failed.\n");
		return NULL;
	}
	//拷贝
	memcpy(pDevNode_ReptExt,pData,Len);

	return pDevNode_ReptExt;
}

//将设备节点数据结构转换为连续的二进制形式，便于存储和传输
//DevDB_DeviceNode - 待转换的结构体数据
//pBuffer - 调用者提供空间，存储转换结果
//BufferLen - 提供的存储空间长度（字节）
//return  <=0-出错，else-实际转换出来的数据长度（字节）
int fStToBin_stDevDB_DeviceNode(stDevDB_DeviceNode *pDevDB_DeviceNode, char *pBuffer, int BufferLen)
{
//	int WriteLen=0,DataLen=0;
//
//	if((pDevDB_DeviceNode==NULL)||(pBuffer==NULL)||(BufferLen<=0)){
//		return -1;
//	}
//
//	WriteLen=sizeof(stDevDB_DeviceNode);
//	if(BufferLen<WriteLen){
//		DEBUGLOG0(3,"Error, not enough buffer memory.\n");
//		return -2;
//	}
//	memcpy(pBuffer,pDevDB_DeviceNode,WriteLen);
//	DataLen=WriteLen;
//
//	switch(pDevDB_DeviceNode->DeviceType){
//	case DEVTYPE_ZIGBEE:
//		WriteLen=fStToBytes_ZigBee(&pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee,pBuffer+DataLen,BufferLen-DataLen);
//		if(WriteLen<=0){
//			DEBUGLOG1(3,"Error, fStToBin_stDevNode_ZigBee failed %d.\n",WriteLen);
//			return -3;
//		}else{
//			DataLen+=WriteLen;
//		}
//		break;
//	case DEVTYPE_WEBCAM:
//		WriteLen=fStToBin_stDevNode_WebCam(&pDevDB_DeviceNode->pDeviceNode->DevNode_WebCam,pBuffer+DataLen,BufferLen-DataLen);
//		if(WriteLen<=0){
//			DEBUGLOG1(3,"Error, fStToBin_stDevNode_WebCam failed %d.\n",WriteLen);
//			return -4;
//		}else{
//			DataLen+=WriteLen;
//		}
//		break;
//	case DEVTYPE_REPTEXT:
//		WriteLen=fStToBin_stDevNode_ReptExt(&pDevDB_DeviceNode->pDeviceNode->DevNode_ReptExt,pBuffer+DataLen,BufferLen-DataLen);
//		if(WriteLen<=0){
//			DEBUGLOG1(3,"Error, fStToBin_stDevNode_ReptExt failed %d.\n",WriteLen);
//			return -5;
//		}else{
//			DataLen+=WriteLen;
//		}
//		break;
//	default:
//		DEBUGLOG1(3,"Error, undefined device type %d.\n",pDevDB_DeviceNode->DeviceType);
//		return -10;
//		break;
//	}
//
//	return DataLen;
	return 0;
}

//将设备节点数据的连续二进制形式恢复为数据结构形式
//pBuffer - 待转换的二进制数据存储区
//DataLen - 数据长度（字节）
//return - NULL-失败，else-恢复出来的设备数据结构，
//注意：该函数会分配存储空间，使用后一定要调用fDevDB_DestroyDeviceNode销毁！
stDevDB_DeviceNode *fBinToSt_stDevDB_DeviceNode(char *pData, int DataLen)
{
//	int TempLen,ReadLen=0;
//	stDevNode_ZigBee *pDevNode_ZigBee=NULL;
//	stDevDB_DeviceNode *pDevDB_DeviceNode=NULL;
//
//	if((pData==NULL)||(DataLen<=sizeof(stDevDB_DeviceNode))){
//		return NULL;
//	}
//
//	TempLen=sizeof(stDevDB_DeviceNode);
//	pDevDB_DeviceNode=malloc(TempLen);
//	if(pDevDB_DeviceNode==NULL){
//		DEBUGLOG0(3,"Error, malloc failed.\n");
//		return NULL;
//	}
//	memcpy(pDevDB_DeviceNode,pData,TempLen);
//	ReadLen=TempLen;
//
//	switch(pDevDB_DeviceNode->DeviceType){
//	case DEVTYPE_ZIGBEE:
//		pDevNode_ZigBee=fCreateDevNode_ZigBee();
//		fBytesToSt_ZigBee(pDevNode_ZigBee,pData+ReadLen,DataLen-ReadLen);
//		if(pDevDB_DeviceNode->pDeviceNode==NULL){
//			DEBUGLOG0(3,"Error, fBinToSt_stDevNode_ZigBee failed.\n");
//			free(pDevDB_DeviceNode);
//			pDevDB_DeviceNode=NULL;
//		}
//		break;
//	case DEVTYPE_WEBCAM:
//		pDevDB_DeviceNode->pDeviceNode=(unDevNode *)fBinToSt_stDevNode_WebCam(pData+ReadLen,DataLen-ReadLen);
//		if(pDevDB_DeviceNode->pDeviceNode==NULL){
//			DEBUGLOG0(3,"Error, fBinToSt_stDevNode_WebCam failed.\n");
//			free(pDevDB_DeviceNode);
//			pDevDB_DeviceNode=NULL;
//		}
//		break;
//	case DEVTYPE_REPTEXT:
//		pDevDB_DeviceNode->pDeviceNode=(unDevNode *)fBinToSt_stDevNode_ReptExt(pData+ReadLen,DataLen-ReadLen);
//		if(pDevDB_DeviceNode->pDeviceNode==NULL){
//			DEBUGLOG0(3,"Error, fBinToSt_stDevNode_ReptExt failed.\n");
//			free(pDevDB_DeviceNode);
//			pDevDB_DeviceNode=NULL;
//		}
//		break;
//	default:
//		DEBUGLOG1(3,"Error, undefined device type %d.\n",pDevDB_DeviceNode->DeviceType);
//		break;
//	}
//
//	return pDevDB_DeviceNode;
	return NULL;
}







