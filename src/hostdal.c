/*
 * hostdal.c
 *
 *  Created on: 2014-2-7
 *      Author: chengm
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "debuglog.h"
#include "devicedb.h"
#include "dal_event.h"
#include "dal_errno.h"
#include "dal_devtype.h"
#include "eventproc.h"
#include "hostdal.h"
#include "module_zigbee.h"
#include "protocol.h"

//DAL模块的全局参数------------------------------------------------------------------------------------------------------
//状态参数
struct{
	unsigned char isDALInitialized;//0-没有初始化，1-已经初始化
}DALGlobalStatus;

//API的实现-------------------------------------------------------------------------------------------------------------
//pDALCfg：初始化DAL模块所需的配置参数。
DALStatus DAL_Init(stDALCfg *pDALCfg)
{
	int rt;
	DALStatus Status;
	stEventProcCfg EventProcCfg;
	stZigBeeMCfg ZigBeeMCfg;

	if(DALGlobalStatus.isDALInitialized==1){
		return DALS_ILLEGREINITIALIZE;
	}
	memset(&DALGlobalStatus,0,sizeof(DALGlobalStatus));

	if(pDALCfg==NULL){
		return DALS_ILLEGPARAM;
	}
	if((pDALCfg->pEventCB==NULL)||(pDALCfg->pZNPComPath==NULL)){
		return DALS_ILLEGPARAM;
	}

	//初始化设备数据库
	rt=fDevDB_Init();
	if(rt!=0){
		DEBUGLOG1(3,"Error, fDevDB_Init failed %d, can't init device db.\n",rt);
		return DALS_GENERALERR;
	}

	//初始化事件处理模块
	memset(&EventProcCfg,0,sizeof(stEventProcCfg));
	EventProcCfg.pEventCB=pDALCfg->pEventCB;
	Status=DAL_Event_Init(&EventProcCfg);
	if(Status!=DALS_SUCCESS){
		DEBUGLOG1(3,"Error, DAL_Event_Init failed %d, can't init the event proc module of DAL.\n",Status);
		return Status;
	}

	//初始化ZigBee子模块
	memset(&ZigBeeMCfg,0,sizeof(stZigBeeMCfg));
	ZigBeeMCfg.pZNPComPath=pDALCfg->pZNPComPath;
	ZigBeeMCfg.isValid_IEEEAddr=pDALCfg->isValid_IEEEAddr;
	memcpy(ZigBeeMCfg.IEEEAddr,pDALCfg->IEEEAddr,8);
	ZigBeeMCfg.isValid_PANID=pDALCfg->isValid_PANID;
	ZigBeeMCfg.PANID=pDALCfg->PANID;
	ZigBeeMCfg.isValid_ExtPANID=pDALCfg->isValid_ExtPANID;
	memcpy(ZigBeeMCfg.ExtPANID,pDALCfg->ExtPANID,8);
	Status=ZigBeeM_Init(&ZigBeeMCfg);
	if(Status!=DALS_SUCCESS){
		DEBUGLOG1(3,"Error, ZigBeeM_Init failed %d.\n",Status);
		return Status;
	}

	DALGlobalStatus.isDALInitialized=1;

	return DALS_SUCCESS;
}

//分配一个DALDetails句柄
stDALDetails *AllocateDALDetails()
{
	stDALDetails *pRet=NULL;
	pRet=(stDALDetails *)malloc(sizeof(stDALDetails));
	if(pRet!=NULL){
		memset(pRet,0,sizeof(stDALDetails));
	}
	return pRet;
}

//释放一个DALDetails句柄
void DelocateDALDetails(stDALDetails *pDALDetails)
{
	if(pDALDetails==NULL){
		return;
	}
	free(pDALDetails);
}

//pDALDetails：用于返回获取到的详细信息结构体
DALStatus DAL_GetDALDetails(stDALDetails *pDALDetails)
{
	DALStatus rt;

	if(pDALDetails==NULL){
		return DALS_ILLEGPARAM;
	}

	if(DALGlobalStatus.isDALInitialized!=1){
		return DALS_UNINITIALIZED;
	}

	pDALDetails->DALVersion=0x0002;//目前主机DAL的版本号
	//以下为各子模块详细信息
	//ZigBee子模块
	rt=ZigBeeM_GetModuleDetails(&pDALDetails->DALSubMDetails_ZigBee);
	if(rt!=DALS_SUCCESS){
		DEBUGLOG1(3,"Error, ZigBeeM_GetModuleDetails failed %d.\n",rt);
		return rt;
	}
	//WebCam子模块
	//待实现
	//RepExt子模块
	//待实现
	return DALS_SUCCESS;
}

//pDeviceDesc：欲添加设备的描述信息，主机通过该参数知道需要添加的设备类型，并执行具体的添加过程。
DALStatus DAL_AddDevice(stDeviceDesc *pDeviceDesc)
{
	DALStatus Status=DALS_SUCCESS;

	if(pDeviceDesc==NULL){
		return DALS_ILLEGPARAM;
	}

	//因为ZigBee设备的描述参数可能会对应多个EP，发现出多个设备，所以新设备加入消息由ZigBeeM_AddDevice自己推送
	switch(pDeviceDesc->DevType){
	case ZIGBEEE:
		Status=ZigBeeM_AddDevice(&pDeviceDesc->un.DeviceDesc_ZigBee);
		break;
	default:
		DEBUGLOG1(3,"Error, unsupported DevType %d.\n",pDeviceDesc->DevType);
		Status=DALS_UNSUPPORTEDDEVICETYPE;
		break;
	}
	return Status;
}

//pDeviceID：设备ID指针，输入需要移除的设备ID。
DALStatus DAL_RemoveDevice(stDALDeviceID *pDeviceID)
{
	int rt;
	stDevDB_DeviceKeyWords DeviceKeyWords;
	stDevDB_DeviceNode *pDeviceNode=NULL;
	DALStatus Status;
	unsigned int DALDevType;
	stEvent Event;

	if((pDeviceID==NULL)||(pDeviceID==NULL)){
		return DALS_ILLEGPARAM;
	}

	//解析出DAL设备类型
	memcpy(&DALDevType,pDeviceID,4);
	//计算设备关键字
	rt=fDALDeviceIDToDevDBKeyWords(&DeviceKeyWords,pDeviceID);
	if(rt!=0){
		DEBUGLOG1(3,"Error, fDALDeviceIDToDevDBKeyWords failed %d.\n",rt);
		return DALS_GENERALERR;
	}
	//找到设备节点
	pDeviceNode=fDevDB_GetDeviceNode(&DeviceKeyWords);
	if(pDeviceNode==NULL){
		DEBUGLOG0(3,"Error, fDevDB_GetDeviceNode failed, can't find device in data base.\n");
		Status=DALS_DEVICENOTFOUND;
	}else{
		//分别处理
		switch(DeviceKeyWords.DeviceType){
		case DEVTYPE_ZIGBEE:
			Status=ZigBeeM_RemoveDevice(pDeviceNode,&DeviceKeyWords);
			break;
		default:
			DEBUGLOG1(3,"Error, unsupported device type %d.\n",DeviceKeyWords.DeviceType);
			Status=DALS_GENERALERR;
			break;
		}
	}
	//销毁节点
	fDevDB_DestroyDeviceNode(pDeviceNode);
	//删除成功时推送设备删除消息
	if(Status==DALS_SUCCESS){
		//推送设备删除消息
		Event.EventType=DALEVENT_DEVICEREMOVED;
		Event.Priority=PRI_NORMAL;
		Event.un.Event_DeviceRemoved.DeviceID=*pDeviceID;
		Status=DAL_Event_Push(&Event);
		if(Status!=DALS_SUCCESS){
			DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
		}
	}
	return Status;
}

//对设备进行控制
DALStatus DAL_DeviceCtrl(stDALDeviceID *pDeviceID, stDALCtrl *pDALCtrl)
{
	int rt;
	stDevDB_DeviceKeyWords DeviceKeyWords;
	stDevDB_DeviceNode *pDeviceNode=NULL;
	DALStatus Status;
	unsigned int DALDevType;

	if((pDeviceID==NULL)||(pDALCtrl==NULL)){
		return DALS_ILLEGPARAM;
	}

	//解析出DAL设备类型
	memcpy(&DALDevType,pDeviceID,4);
	//计算设备关键字
	rt=fDALDeviceIDToDevDBKeyWords(&DeviceKeyWords,pDeviceID);
	if(rt!=0){
		DEBUGLOG1(3,"Error, fDALDeviceIDToDevDBKeyWords failed %d.\n",rt);
		return DALS_GENERALERR;
	}
	//找到设备节点
	pDeviceNode=fDevDB_GetDeviceNode(&DeviceKeyWords);
	if(pDeviceNode==NULL){
		DEBUGLOG0(3,"Error, fDevDB_GetDeviceNode failed, can't find device in data base.\n");
		Status=DALS_DEVICENOTFOUND;
	}else{
		//分别处理
		switch(DeviceKeyWords.DeviceType){
		case DEVTYPE_ZIGBEE:
			Status=ZigBeeM_DeviceCtrl(pDeviceNode,DALDevType,pDALCtrl);
			break;
		default:
			DEBUGLOG1(3,"Error, unsupported device type %d.\n",DeviceKeyWords.DeviceType);
			Status=DALS_GENERALERR;
			break;
		}
	}

	//销毁节点
	fDevDB_DestroyDeviceNode(pDeviceNode);
	return Status;
}

//分配一个DeviceIDList
stDeviceIDList *AllocateDeviceIDList()
{
	stDeviceIDList *pRet=NULL;
	pRet=(stDeviceIDList *)malloc(sizeof(stDeviceIDList));
	if(pRet!=NULL){
		memset(pRet,0,sizeof(stDeviceIDList));
	}
	return pRet;
}
//释放一个DeviceIDList
void DelocateDeviceIDList(stDeviceIDList *pDeviceIDList)
{
	if(pDeviceIDList!=NULL){
		if(pDeviceIDList->pDALDeviceIDList!=NULL){
			free(pDeviceIDList->pDALDeviceIDList);
		}
		free(pDeviceIDList);
	}
}

//获取设备ID列表
//当前只支持ZigBee类型设备
DALStatus DAL_GetDeviceIDList(stDeviceIDList *pDeviceIDList)
{
	int i=0,TotalDeviceNum=0,rt=0,KeyWordsNum=0,ErrNo=DALS_SUCCESS;
	stDevDB_DeviceKeyWords *pKeyWordsList=NULL;

	if(pDeviceIDList==NULL){
		return DALS_ILLEGPARAM;
	}

	//统计总的设备数量
	TotalDeviceNum=fDevDB_Statistic_DeviceNum(DEVTYPE_ZIGBEE);
	if(TotalDeviceNum<=0){
		pDeviceIDList->DeviceNum=0;
		return DALS_SUCCESS;
	}

	//获取所有设备的DeviceKeyWords列表
	pKeyWordsList=(stDevDB_DeviceKeyWords *)malloc(TotalDeviceNum*sizeof(stDevDB_DeviceKeyWords));
	if(pKeyWordsList!=NULL){
		KeyWordsNum=fDevDB_GetDeviceKeyWordsList(DEVTYPE_ZIGBEE, 0, TotalDeviceNum, pKeyWordsList);
		if(KeyWordsNum<0){
			DEBUGLOG1(3,"Error, fDevDB_GetDeviceKeyWordsList failed %d.\n",KeyWordsNum);
			ErrNo=DALS_GENERALERR;
		}else{
			//转换为DAL的设备ID列表
			//分配空间
			pDeviceIDList->pDALDeviceIDList=(stDALDeviceID *)malloc(KeyWordsNum*sizeof(stDALDeviceID));
			if(pDeviceIDList->pDALDeviceIDList!=NULL){
				//转换
				for(i=0;i<KeyWordsNum;i++){
					rt=fDevDBKeyWordsToDALDeviceID(&pDeviceIDList->pDALDeviceIDList[i],&pKeyWordsList[i]);
					if(rt!=0){
						ErrNo=DALS_GENERALERR;
						break;
					}else{
						pDeviceIDList->DeviceNum++;
					}
				}
			}
		}
	}else{
		ErrNo=DALS_MALLOCFAILED;
	}

	//释放空间
	if(ErrNo!=DALS_SUCCESS){
		if(pDeviceIDList->pDALDeviceIDList!=NULL){
			free(pDeviceIDList->pDALDeviceIDList);
		}
		//出错后返回0个设备
		pDeviceIDList->DeviceNum=0;
	}
	if(pKeyWordsList!=NULL){
		free(pKeyWordsList);
	}

	return ErrNo;
}

//分配一个stDALStatus句柄
stDALStatus *AllocateDALStatus()
{
	stDALStatus *pRet=NULL;
	pRet=(stDALStatus *)malloc(sizeof(stDALStatus));
	if(pRet!=NULL){
		memset(pRet,0,sizeof(stDALStatus));
	}
	return pRet;
}

//释放一个stDALStatus句柄
void DelocateDALStatus(stDALStatus *pDALStatus)
{
	if(pDALStatus!=NULL){
		free(pDALStatus);
	}
}

//pDeviceID：设备ID指针，需要访问的设备ID；
//pDALStatus：获取到的状态参数。
DALStatus DAL_GetDeviceStatus(stDALDeviceID *pDeviceID, stDALStatus *pDALStatus)
{
	int rt;
	stDevDB_DeviceKeyWords DeviceKeyWords;
	stDevDB_DeviceNode *pDeviceNode=NULL;
	DALStatus Status=DALS_SUCCESS;
	unsigned int DALDevType;

	//参数检查
	if((pDeviceID==NULL)||(pDALStatus==NULL)){
		return DALS_ILLEGPARAM;
	}
	//解析出DAL设备类型
	memcpy(&DALDevType,pDeviceID,4);
	//计算设备关键字
	rt=fDALDeviceIDToDevDBKeyWords(&DeviceKeyWords,pDeviceID);
	if(rt!=0){
		DEBUGLOG1(3,"Error, fDALDeviceIDToDevDBKeyWords failed %d.\n",rt);
		return DALS_GENERALERR;
	}
	//找到设备节点
	pDeviceNode=fDevDB_GetDeviceNode(&DeviceKeyWords);
	if(pDeviceNode==NULL){
		DEBUGLOG0(3,"Error, fDevDB_GetDeviceNode failed, can't find device in data base.\n");
		Status=DALS_DEVICENOTFOUND;
	}else{
		//分别处理
		switch(DeviceKeyWords.DeviceType){
		case DEVTYPE_ZIGBEE:
			Status=ZigBeeM_GetDeviceStatus(pDeviceNode, DALDevType, pDALStatus);
			break;
		default:
			DEBUGLOG1(3,"Error, unsupported device type %d.\n",DeviceKeyWords.DeviceType);
			Status=DALS_GENERALERR;
			break;
		}
	}

	//释放系统资源
	if(pDeviceNode!=NULL){
		fDevDB_DestroyDeviceNode(pDeviceNode);
	}
	return Status;
}

//设置设备名称
DALStatus DAL_SetDeviceName(stDALDeviceID *pDeviceID, char *pDeviceName)
{
	int rt;
	stDevDB_DeviceKeyWords DeviceKeyWords;
	stDevDB_DeviceNode *pDeviceNode=NULL;
	DALStatus Status=DALS_SUCCESS;

	//参数检查
	if((pDeviceID==NULL)||(pDeviceName==NULL)){
		return DALS_ILLEGPARAM;
	}

	//计算设备关键字
	rt=fDALDeviceIDToDevDBKeyWords(&DeviceKeyWords,pDeviceID);
	if(rt!=0){
		DEBUGLOG1(3,"Error, fDALDeviceIDToDevDBKeyWords failed %d.\n",rt);
		return DALS_GENERALERR;
	}
	//找到设备节点进行更新
	pDeviceNode=fDevDB_GetDeviceNode(&DeviceKeyWords);
	if(pDeviceNode==NULL){
		DEBUGLOG0(3,"Error, fDevDB_GetDeviceNode failed, can't find device in data base.\n");
		Status=DALS_DEVICENOTFOUND;
	}else{
		strncpy(pDeviceNode->DeviceName,pDeviceName,MAXDEVICENAME);
		rt=fDevDB_AddDevice(pDeviceNode);
		if(rt!=1){
			DEBUGLOG0(3,"Warning, maybe a new device node has been created.\n");
		}
	}

	//释放系统资源
	if(pDeviceNode!=NULL){
		fDevDB_DestroyDeviceNode(pDeviceNode);
	}
	return Status;
}

//设置设备描述信息
DALStatus DAL_SetDeviceDesc(stDALDeviceID *pDeviceID, char *pDeviceDesc)
{
	int rt;
	stDevDB_DeviceKeyWords DeviceKeyWords;
	stDevDB_DeviceNode *pDeviceNode=NULL;
	DALStatus Status=DALS_SUCCESS;

	//参数检查
	if((pDeviceID==NULL)||(pDeviceDesc==NULL)){
		return DALS_ILLEGPARAM;
	}

	//计算设备关键字
	rt=fDALDeviceIDToDevDBKeyWords(&DeviceKeyWords,pDeviceID);
	if(rt!=0){
		DEBUGLOG1(3,"Error, fDALDeviceIDToDevDBKeyWords failed %d.\n",rt);
		return DALS_GENERALERR;
	}
	//找到设备节点进行更新
	pDeviceNode=fDevDB_GetDeviceNode(&DeviceKeyWords);
	if(pDeviceNode==NULL){
		DEBUGLOG0(3,"Error, fDevDB_GetDeviceNode failed, can't find device in data base.\n");
		Status=DALS_DEVICENOTFOUND;
	}else{
		strncpy(pDeviceNode->UserDescript,pDeviceDesc,MAXUSRDESC);
		rt=fDevDB_AddDevice(pDeviceNode);
		if(rt!=1){
			DEBUGLOG0(3,"Warning, maybe a new device node has been created.\n");
		}
	}

	//释放系统资源
	if(pDeviceNode!=NULL){
		fDevDB_DestroyDeviceNode(pDeviceNode);
	}
	return Status;
}

//分配一个stDeviceInfo句柄
stDeviceInfo *AllocateDeviceInfo()
{
	stDeviceInfo *pRet=NULL;
	pRet=(stDeviceInfo *)malloc(sizeof(stDeviceInfo));
	if(pRet!=NULL){
		memset(pRet,0,sizeof(stDeviceInfo));
	}
	return pRet;
}

//释放一个stDeviceInfo句柄
void DelocateDeviceInfo(stDeviceInfo *pDeviceInfo)
{
	if(pDeviceInfo!=NULL){
		free(pDeviceInfo);
	}
}

//pDeviceID：设备ID指针，需要访问的设备ID；
//pDeviceInfo：设备信息
DALStatus DAL_GetDeviceInfo(stDALDeviceID *pDeviceID, stDeviceInfo *pDeviceInfo)
{
	int rt;
	stDevDB_DeviceKeyWords DeviceKeyWords;
	stDevDB_DeviceNode *pDeviceNode=NULL;
	DALStatus Status=DALS_SUCCESS;

	//参数检查
	if((pDeviceID==NULL)||(pDeviceInfo==NULL)){
		return DALS_ILLEGPARAM;
	}

	//计算设备关键字
	rt=fDALDeviceIDToDevDBKeyWords(&DeviceKeyWords,pDeviceID);
	if(rt!=0){
		DEBUGLOG1(3,"Error, fDALDeviceIDToDevDBKeyWords failed %d.\n",rt);
		return DALS_GENERALERR;
	}
	//找到设备节点
	pDeviceNode=fDevDB_GetDeviceNode(&DeviceKeyWords);
	if(pDeviceNode==NULL){
		DEBUGLOG0(3,"Error, fDevDB_GetDeviceNode failed, can't find device in data base.\n");
		Status=DALS_DEVICENOTFOUND;
	}else{
		strncpy(pDeviceInfo->DeviceName,pDeviceNode->DeviceName,MAXDEVICENAMELEN);
		strncpy(pDeviceInfo->DeviceDesc,pDeviceNode->UserDescript,MAXDEVICEDESCLEN);
	}

	//释放系统资源
	if(pDeviceNode!=NULL){
		fDevDB_DestroyDeviceNode(pDeviceNode);
	}
	return Status;
}







