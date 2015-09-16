/*
 * module_zigbee.c
 *
 *  Created on: 2014-2-10
 *      Author: chengm
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "debuglog.h"
#include "profiles.h"
#include "znplib.h"
#include "znpmsg.h"
#include "clusters.h"
#include "zclmsg.h"
#include "znpzcl.h"
#include "module_zigbee.h"
#include "devicedb_zigbee.h"
#include "devicedb.h"
#include "protocol.h"
#include "eventproc.h"
#include "dal_devtype.h"
#include "alige.h"

#define ENDPOINT_HA		33	//HA协议使用的EP
#define ENDPOINT_ITS	34	//ITS协议使用的EP

//全局参数
stZigBeeMStatus GlobalZigBeeMStatus;

//枚举指定的ZigBee设备
//DevAddr - 16位短地址
//EndPoint - 需要枚举的EndPoint
//return 枚举得到的ZigBee设备节点数据结构,NULL-枚举失败
//如果枚举成功，则返回的节点结构在使用之后要调用fDestroyDevNode_ZigBee进行销毁
stDevNode_ZigBee *fZigBeeDeviceEnumerate_Endpoint(unsigned short DevAddr, unsigned char EndPoint)
{
	int i,rt;
	stDevNode_ZigBee *pRet=NULL;
	stZDO_SIMPLE_DESC_RSP mZDO_SIMPLE_DESC_RSP;
	stZDO_IEEE_ADDR_RSP mZDO_IEEE_ADDR_RSP;
	stZDO_USER_DESC_RSP mZDO_USER_DESC_RSP;

	rt=Znp_ZDO_SIMPLE_DESC_REQ(DevAddr,DevAddr,EndPoint,&mZDO_SIMPLE_DESC_RSP);
	if(rt!=0){
		DEBUGLOG2(3,"Error, Znp_ZDO_SIMPLE_DESC_REQ for 0x%04X failed %d.\n",DevAddr,rt);
		return NULL;
	}

	rt=Znp_ZDO_IEEE_ADDR_REQ(DevAddr,1,0,&mZDO_IEEE_ADDR_RSP);
	if(rt!=0){
		DEBUGLOG2(3,"Error, Znp_ZDO_IEEE_ADDR_REQ for 0x%04X failed %d.\n",DevAddr,rt);
		return NULL;
	}

	rt=Znp_ZDO_USER_DESC_REQ(DevAddr, DevAddr, &mZDO_USER_DESC_RSP);
	if(rt!=0){
		DEBUGLOG2(3,"Error, Znp_ZDO_USER_DESC_REQ for 0x%04X failed %d.\n",DevAddr,rt);
		return NULL;
	}

	pRet=fCreateDevNode_ZigBee();
	if(pRet==NULL){
		DEBUGLOG0(3,"Error, fCreateDevNode_ZigBee failed.\n");
		return NULL;
	}

	memcpy(pRet->IEEEAddr,mZDO_IEEE_ADDR_RSP.IEEEAddr,8);
	memcpy(pRet->UserDescriptor,mZDO_USER_DESC_RSP.UserDescriptor,MAXUSERDESCRIPTORLEN);
	pRet->UserDescriptor[MAXUSERDESCRIPTORLEN-1]='\0';
	pRet->NwkAddr=DevAddr;
	pRet->ProfileID=mZDO_SIMPLE_DESC_RSP.ProfileId;
	pRet->DeviceID=mZDO_SIMPLE_DESC_RSP.DeviceID;
	pRet->EP=EndPoint;
	pRet->NumInClusters=mZDO_SIMPLE_DESC_RSP.NumInClusters;
	pRet->NumOutClusters=mZDO_SIMPLE_DESC_RSP.NumOutClusters;
	for(i=0;i<pRet->NumInClusters;i++){
		pRet->InClusterList[i].ClusterID=mZDO_SIMPLE_DESC_RSP.InClusterList[i];
		//当前不做属性发现，以后视情况添加
		//pRet->InClusterList[i].AttriNum=0;
		//memset(pRet->InClusterList[i].AttriIDList,0,MAXATTRIIDNUM*2);
	}
	for(i=0;i<pRet->NumOutClusters;i++){
		pRet->OutClusterList[i].ClusterID=mZDO_SIMPLE_DESC_RSP.OutClusterList[i];
		//当前不做属性发现，以后视情况添加
		//pRet->OutClusterList[i].AttriNum=0;
		//memset(pRet->OutClusterList[i].AttriIDList,0,MAXATTRIIDNUM*2);
	}

	return pRet;
}

//配置一个ZigBee设备
//根据应用需要，对不同的ZigBee设备进行必要的配置，例如绑定、配置属性报告等
void fZigBeeDeviceCfg(stDevNode_ZigBee *pDevNode)
{
	int rt;
	stZCL_CFGREPORT mZCL_CFGREPORT;
	stZCL_CFGREPORT_R mZCL_CFGREPORT_R;

	if(pDevNode==NULL){
		return ;
	}

	DEBUGLOG1(3,"Try to config Zigbee device 0x%04X.\n",pDevNode->NwkAddr);

	switch(pDevNode->ProfileID){
	case PROFILE_HA:
		switch(pDevNode->DeviceID){
		case DEV_HA_ON_OFF_LIGHT://对于OnOff型照明设备，绑定ONOFF Cluster，以实现状态变化时的属性报告
			rt=Znp_ZDO_BIND_REQ(pDevNode->NwkAddr,pDevNode->IEEEAddr,pDevNode->EP,CLUSTER_ONOFF,ADDRESS_64_BIT,GlobalZigBeeMStatus.LocalIEEEAddr,GlobalZigBeeMStatus.pZclInst_HA->EP);
			if(rt!=0){
				DEBUGLOG1(3,"Error, Znp_ZDO_BIND_REQ failed for device 0x%04X.\n",pDevNode->NwkAddr);
			}else{
				//配置属性报告
				memset(&mZCL_CFGREPORT,0,sizeof(stZCL_CFGREPORT));
				mZCL_CFGREPORT.RecordNum=1;
				mZCL_CFGREPORT.AttriReportCfgRecord[0].AttriID=ATTRIID_ONOFF_ONOFF;
				mZCL_CFGREPORT.AttriReportCfgRecord[0].Direction=0;
				mZCL_CFGREPORT.AttriReportCfgRecord[0].DataType=ATTRIDATATYPE_ONOFF_ONOFF;
				mZCL_CFGREPORT.AttriReportCfgRecord[0].MaxInterval=0x0001;
				mZCL_CFGREPORT.AttriReportCfgRecord[0].MinInterval=0x0000;
				rt=ZclCfgAttriReport(GlobalZigBeeMStatus.pZclInst_HA,NULL,pDevNode->NwkAddr,pDevNode->EP,CLUSTER_ONOFF,&mZCL_CFGREPORT,&mZCL_CFGREPORT_R);
				if(rt!=0){
					DEBUGLOG2(3,"Error, ZclCfgAttriReport for device 0x%04X failed %d.\n",pDevNode->NwkAddr,rt);
				}
			}
			break;
		case DEV_HA_IAS_ZONE://对于红外入侵检测设备，通过绑定操作，将其HA_IAS_ZONE Cluster绑定到主机的HA端点
			//绑定HA_IAS_ZONE，以接收入侵警报
			rt=Znp_ZDO_BIND_REQ(pDevNode->NwkAddr,pDevNode->IEEEAddr,pDevNode->EP,CLUSTER_IASZONE,ADDRESS_64_BIT,GlobalZigBeeMStatus.LocalIEEEAddr,GlobalZigBeeMStatus.pZclInst_HA->EP);
			if(rt!=0){
				DEBUGLOG1(3,"Error, Znp_ZDO_BIND_REQ HA_IAS_ZONE failed for device 0x%04X.\n",pDevNode->NwkAddr);
			}
			//绑定HA_GEN_ALARM，以接收低电量警报
			rt=Znp_ZDO_BIND_REQ(pDevNode->NwkAddr,pDevNode->IEEEAddr,pDevNode->EP,CLUSTER_ALARMS,ADDRESS_64_BIT,GlobalZigBeeMStatus.LocalIEEEAddr,GlobalZigBeeMStatus.pZclInst_HA->EP);
			if(rt!=0){
				DEBUGLOG1(3,"Error, Znp_ZDO_BIND_REQ failed HA_GEN_ALARM for device 0x%04X.\n",pDevNode->NwkAddr);
			}
			break;
		default:
			DEBUGLOG0(3,"Error, unkonwn device with HA ProfileID.\n");
			break;
		}
		break;
	case PROFILE_ITS:
		//目前ITS proflie中没有设备需要配置
		break;
	default:
		DEBUGLOG1(3,"Error, unkonwn profile 0x%04X.\n",pDevNode->ProfileID);
		break;
	}
}

//枚举指定的ZigBee设备,并且对设备进行初始化配置
//DevAddr - 16位短地址
//设备如果枚举成功，将被加入数据库，并且推送DAL消息
//return 0-成功，else-出错（如果设备有多个EP，只要有一个不成功，都返回失败）
int fZigBeeDeviceEnumerate(unsigned short DevAddr)
{
	int i,rt,err=0;
	stDevNode_ZigBee *pDevNode_ZigBee=NULL;
	stZDO_ACTIVE_EP_RSP mZDO_ACTIVE_EP_RSP;
	stDevDB_DeviceNode DevDB_DeviceNode;
	DALStatus Status;
	stEvent Event;

	//找出新加入设备的EndPoint
	rt=Znp_ZDO_ACTIVE_EP_REQ(DevAddr,DevAddr,&mZDO_ACTIVE_EP_RSP);
	if(rt!=0){
		DEBUGLOG2(3,"Error, Znp_ZDO_ACTIVE_EP_REQ for new joined device 0x%04X failed %d.\n",DevAddr,rt);
		err=1;
	}else{
		for(i=0;i<mZDO_ACTIVE_EP_RSP.ActiveEPCount;i++){//每个EP枚举为一个设备
			pDevNode_ZigBee=fZigBeeDeviceEnumerate_Endpoint(DevAddr, mZDO_ACTIVE_EP_RSP.ActiveEPList[i]);
			if(pDevNode_ZigBee==NULL){
				DEBUGLOG1(2,"Warning, fZigBeeDeviceEnumerate for EP %d failed.\n",mZDO_ACTIVE_EP_RSP.ActiveEPList[i]);
				err=2;
			}else{
				//枚举成功，加入数据库
				DevDB_DeviceNode.DeviceKeyWords.DeviceType=DEVTYPE_ZIGBEE;
				rt=fGenKeyInfo_ZigBee2(pDevNode_ZigBee,DevDB_DeviceNode.DeviceKeyWords.DeviceKeyInfo);
				if(rt!=0){
					DEBUGLOG1(3,"Error, fGenKeyInfo_ZigBee failed %d.\n",rt);
					err=3;
				}else{
					strncpy(DevDB_DeviceNode.DeviceName,"NewZigBeeDevice",MAXDEVICENAME);
					DevDB_DeviceNode.DeviceType=DEVTYPE_ZIGBEE;
					strncpy(DevDB_DeviceNode.UserDescript,"This is a new zigbee device.",MAXUSRDESC);
					DevDB_DeviceNode.pDeviceNode=(unDevNode *)pDevNode_ZigBee;
					rt=fDevDB_AddDevice(&DevDB_DeviceNode);
					if((rt!=0)&&(rt!=1)){
						DEBUGLOG1(2,"Warning, fDevDB_AddDevice failed %d.\n",rt);
						err=4;
					}else{//设备已经枚举成功并添加到数据库
						fZigBeeDeviceCfg(pDevNode_ZigBee);//配置该设备
					}
					if(rt==0){//加入成功，且是一个新的节点才推送消息
						//推送新设备加入消息
						Event.EventType=DALEVENT_DEVICEJIONED;
						Event.Priority=PRI_NORMAL;
						rt=fDevDBKeyWordsToDALDeviceID(&Event.un.Event_DeviceJoined.DeviceID,&DevDB_DeviceNode.DeviceKeyWords);
						if(rt!=0){
							DEBUGLOG1(3,"Error, fDevDBKeyWordsToDALDeviceID failed %d.\n",rt);
						}else{
							Status=DAL_Event_Push(&Event);
							if(Status!=DALS_SUCCESS){
								DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
							}
						}
					}
				}
				//销毁发现的节点
				fDestroyDevNode_ZigBee(pDevNode_ZigBee);
			}
		}
	}
	return err;
}

//ZDO_END_DEVICE_ANNCE_IND消息处理
//枚举新设备，推送新设备加入消息
void ZnpMsgHandler_ZDO_END_DEVICE_ANNCE_IND(stZnpMsg *pZnpMsg)
{
	int rt;
	stDevDB_SearchKey SearchKey;
	stDevDB_DeviceNode *ppDeviceNodeList[1]={NULL};

	if(pZnpMsg==NULL){
		return;
	}

	//检查设备数据库中是否已经有相同的IEEE地址的设备
	memset(&SearchKey,0,sizeof(stDevDB_SearchKey));
	SearchKey.DeviceType=DEVTYPE_ZIGBEE;
	SearchKey.un.SearchKey_ZigBee.AddrType=1;//Match IEEE address
	SearchKey.un.SearchKey_ZigBee.doMatchEP=0;//No need to match EP
	memcpy(SearchKey.un.SearchKey_ZigBee.unAddr.IEEEAddr,pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr,8);
	rt=fDevDB_FindDeviceNode(&SearchKey, 1, ppDeviceNodeList);
	if(rt==0){//没找到，枚举
		//fZigBeeDeviceEnumerate(pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.SrcAddr);
		DEBUGLOG0(1,"A new device joined with IEEE address ");
		S_DEBUGLOG4(1,"%02X-%02X-%02X-%02X-",pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[7],pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[6],pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[5],pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[4]);
		S_DEBUGLOG4(1,"%02X-%02X-%02X-%02X\n",pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[3],pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[2],pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[1],pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[0]);
		S_DEBUGLOG0(1,"A device self_introduction is expected from the new joined device to create a database record.\n");
	}else if(rt>0){//找到了，忽略
		DEBUGLOG2(1,"Received a ZDO_END_DEVICE_ANNCE_IND msg from an existing device %02X%02X, ignored.\n",pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[1],pZnpMsg->un.mZDO_END_DEVICE_ANNCE_IND.IEEEAddr[0]);
	}else{//出错
		DEBUGLOG1(3,"Error, fDevDB_FindDeviceNode failed %d.\n",rt);
	}
	//释放资源
	if(ppDeviceNodeList[0]!=NULL){
		fDevDB_DestroyDeviceNode(ppDeviceNodeList[0]);
	}
}

//ZDO_LEAVE_IND消息处理
//看数据库中是否有该设备，如果有就删除，并且推送DAL消息
void ZnpMsgHandler_ZDO_LEAVE_IND(stZnpMsg *pZnpMsg)
{
	int i,rt,nFound=0;
	DALStatus Status;
	stDevDB_SearchKey SearchKey;
	stDevDB_DeviceNode *ppDevDB_DeviceNodeList[8];
	stEvent Event;

	memset(ppDevDB_DeviceNodeList,0,sizeof(stDevDB_DeviceNode *)*8);

	//数据库中是否有该设备
	SearchKey.DeviceType=DEVTYPE_ZIGBEE;
	SearchKey.un.SearchKey_ZigBee.AddrType=1;//unAddr类型：0-NwkAddr、1-IEEEAddr
	memcpy(SearchKey.un.SearchKey_ZigBee.unAddr.IEEEAddr,pZnpMsg->un.mZDO_LEAVE_IND.ExtAddr,8);
	SearchKey.un.SearchKey_ZigBee.doMatchEP=0;//不需要匹配EP，所有该设备下的EP都会被删除
	nFound=fDevDB_FindDeviceNode(&SearchKey, 8, ppDevDB_DeviceNodeList);
	if(nFound<0){
		DEBUGLOG1(3,"Error, fDevDB_FindDeviceNode failed %d.\n",nFound);
	}else if(nFound>0){
		if(nFound>=8){
			DEBUGLOG0(3,"Warning, there may be more device in db to be removed.\n");
		}
		//逐一删除，并推送消息
		for(i=0;i<nFound;i++){
			rt=fDevDB_RemoveDevice(&ppDevDB_DeviceNodeList[i]->DeviceKeyWords);
			if(rt==0){//删除成功
				//推送消息
				Event.EventType=DALEVENT_DEVICEREMOVED;
				Event.Priority=PRI_NORMAL;
				rt=fDevDBKeyWordsToDALDeviceID(&Event.un.Event_DeviceJoined.DeviceID,&ppDevDB_DeviceNodeList[i]->DeviceKeyWords);
				if(rt!=0){
					DEBUGLOG1(3,"Error, fDevDBKeyWordsToDALDeviceID failed %d.\n",rt);
				}else{
					Status=DAL_Event_Push(&Event);
					if(Status!=DALS_SUCCESS){
						DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
					}
				}
			}else if(rt==1){//指定设备不存在
				DEBUGLOG0(2,"Warning, fDevDB_RemoveDevice a none existing device.\n");
			}else{//出错
				DEBUGLOG1(2,"Warning, fDevDB_RemoveDevice failed %d.\n",rt);
			}
			fDevDB_DestroyDeviceNode(ppDevDB_DeviceNodeList[i]);
		}
	}else{//数据库中没有找到相关的设备记录
		DEBUGLOG0(1,"Received a ZDO_LEAVE_IND msg from a device not exists in Device db.\n");
	}
}

//ZNP默认消息处理回调
void DefaultZnpMsgCB(stZnpMsg *pZnpMsg)
{
	if(pZnpMsg==NULL){
		DEBUGLOG0(3,"Error, got a NULL pZnpMsg!\n");//This should not happen.
		return;
	}

	switch(pZnpMsg->MsgType){
	case CMD_ZDO_END_DEVICE_ANNCE_IND://新设备加入
		ZnpMsgHandler_ZDO_END_DEVICE_ANNCE_IND(pZnpMsg);
		break;
	case CMD_ZDO_LEAVE_IND://设备退出ZigBee网络时上报的消息
		ZnpMsgHandler_ZDO_LEAVE_IND(pZnpMsg);
		break;
	case CMD_AF_DATA_CONFIRM:
		DEBUGLOG1(2,"Warning, unexpected AF_DATA_CONFIRM with status %d, ignored!\n",pZnpMsg->un.mAF_DATA_CONFIRM.Status);
		break;
	default:
		DEBUGLOG1(2,"Warning, unexpected ZnpMsg 0x%04X, ignored!\n",pZnpMsg->MsgType);
		break;
	}
}

void ZclMsgHandler_ZCL_GENERAL_ATTRIREPORT_HA(stZclMsg *pZclMsg)
{
	int i,rt,isErr=0;
	stDevDB_SearchKey SearchKey;
	stDevDB_DeviceNode *ppDeviceNode[1]={NULL};
	stEvent Event;
	DALStatus Status;

	if(pZclMsg==NULL){
		return;
	}
	memset(&Event,0,sizeof(stEvent));

	//找到发送报告的设备在数据库中的记录
	SearchKey.DeviceType=DEVTYPE_ZIGBEE;
	SearchKey.un.SearchKey_ZigBee.AddrType=0;//0-NwkAddr、1-IEEEAddr
	SearchKey.un.SearchKey_ZigBee.unAddr.NwkAddr=pZclMsg->ZclMsgAccessory.SrcAddr;
	SearchKey.un.SearchKey_ZigBee.doMatchEP=1;
	SearchKey.un.SearchKey_ZigBee.EP=pZclMsg->ZclMsgAccessory.SrcEP;
	rt=fDevDB_FindDeviceNode(&SearchKey,1,ppDeviceNode);
	if(rt==0){
		DEBUGLOG1(2,"Warning, can't find HA device node record (NwkAddr=0x%04X) for incoming ZCL_GENERAL_ATTRIREPORT msg, ignored.\n",pZclMsg->ZclMsgAccessory.SrcAddr);
		isErr=1;
		//rt=fZigBeeDeviceEnumerate(pZclMsg->ZclMsgAccessory.SrcAddr);
		//if(rt!=0){
		//	DEBUGLOG1(2,"Device enumerating failed %d.\n",rt);
		//	isErr=1;
		//}
	}else{//推送事件
		//共同部分
		Event.EventType=DALEVENT_DEVICEEVENT;
		rt=fDevDBKeyWordsToDALDeviceID(&Event.un.Event_DALEvent.DeviceID,&ppDeviceNode[0]->DeviceKeyWords);
		if(rt!=0){
			DEBUGLOG0(2,"Error, fDevDBKeyWordsToDALDeviceID failed.\n");
			isErr=1;
		}else{
			//差别部分
			//ProfileID肯定是HA，因为这个函数是由DefaultZclMsgCB_HA调用的，DefaultZclMsgCB_HA本身就是HA端点的ZclMsg处理回调
			switch(ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.DeviceID){
			case DEV_HA_ON_OFF_LIGHT:
				switch(pZclMsg->ZclMsgAccessory.ClusterID){
				case CLUSTER_ONOFF:
					Event.Priority=PRI_NORMAL;
					Event.un.Event_DALEvent.DALEventID=DAL_ONOFFLIGHT_EVENT_ONOFF;
					for(i=0;i<pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriNum;i++){//找到OnOff属性
						if(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[i].AttriID==ATTRIID_ONOFF_ONOFF){
							Event.un.Event_DALEvent.ParamNum=1;
							Event.un.Event_DALEvent.ParamList[0].DataType=ATTRIDATATYPE_ONOFF_ONOFF;
							Event.un.Event_DALEvent.ParamList[0].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[i].AttriData.Data[0];
							break;
						}
					}
					if(i>pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriNum){
						DEBUGLOG0(3,"Error, can't find expected AttriID in the incoming ZCL_GENERAL_ATTRIREPORT ZclMsg.\n");
						isErr=1;
					}
					break;
				default:
					DEBUGLOG3(3,"Error, unexpected event for Cluster 0x%04x of Device 0x%04x in profile 0x%04x.\n",pZclMsg->ZclMsgAccessory.ClusterID,ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.DeviceID,ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.ProfileID);
					isErr=1;
					break;
				}
				break;
			default:
				DEBUGLOG1(3,"Error, unknown DeviceID 0x%04x for HA profile.\n",ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.DeviceID);
				isErr=1;
				break;
			}
		}
	}

	//推送事件
	if(!isErr){
		Status=DAL_Event_Push(&Event);
		if(Status!=DALS_SUCCESS){
			DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
		}
	}

	//释放系统资源
	if(ppDeviceNode[0]!=NULL){
		fDevDB_DestroyDeviceNode(ppDeviceNode[0]);
	}
}

void ZclMsgHandler_ZCLMsgForCLUSTER_HA(stZclMsg *pZclMsg)
{
	int i,rt,EventNum=0;
	stDevDB_SearchKey SearchKey;
	stDevDB_DeviceNode *ppDeviceNode[1]={NULL};
	stEvent pEventList[2];
	DALStatus Status;
	unsigned short pAttriList[4];
	stZCL_ATTRIREAD_R mZCL_ATTRIREAD_R;
	unsigned char BatteryVoltage=0;

	if(pZclMsg==NULL){
		return;
	}
	memset(pEventList,0,sizeof(stEvent)*2);

	//找到发送消息的设备在数据库中的记录
	SearchKey.DeviceType=DEVTYPE_ZIGBEE;
	SearchKey.un.SearchKey_ZigBee.AddrType=0;//0-NwkAddr、1-IEEEAddr
	SearchKey.un.SearchKey_ZigBee.unAddr.NwkAddr=pZclMsg->ZclMsgAccessory.SrcAddr;
	SearchKey.un.SearchKey_ZigBee.doMatchEP=1;
	SearchKey.un.SearchKey_ZigBee.EP=pZclMsg->ZclMsgAccessory.SrcEP;
	rt=fDevDB_FindDeviceNode(&SearchKey,1,ppDeviceNode);
	if(rt==0){
		DEBUGLOG0(2,"Warning, can't find device node for incoming ZclMsg, ignored.\n");
	}else{
		//ProfileID肯定是HA，因为这个函数是由DefaultZclMsgCB_HA调用的，DefaultZclMsgCB_HA本身就是HA端点的ZclMsg处理回调
		switch(pZclMsg->ZclMsgAccessory.ClusterID){
		case CLUSTER_IASZONE:
			switch(pZclMsg->ZclMsgHdr.CmdID){
			case CMD_IASZONE_ZONESTATUSCHANGENOTIF:
				if(pZclMsg->un.ClusterCmd.unClusterCmd_IASZone.mCMD_IASZONE_ZONESTATUSCHANGENOTIF.ZoneStatus & 0x0001){//入侵报警
					//创建事件
					pEventList[EventNum].EventType=DALEVENT_DEVICEEVENT;
					rt=fDevDBKeyWordsToDALDeviceID(&pEventList[EventNum].un.Event_DALEvent.DeviceID,&ppDeviceNode[0]->DeviceKeyWords);
					if(rt!=0){
						DEBUGLOG0(2,"Error, fDevDBKeyWordsToDALDeviceID failed.\n");
					}else{
						pEventList[EventNum].Priority=PRI_HIGH;
						pEventList[EventNum].un.Event_DALEvent.DALEventID=DAL_IRINTRUSIONALARM_INTRUSION;
						pEventList[EventNum].un.Event_DALEvent.ParamNum=0;
						EventNum++;
					}
				}
				break;
			default:
				DEBUGLOG2(3,"Error, unsupported cmd %d for cluster 0x%04x.\n",pZclMsg->ZclMsgHdr.CmdID,pZclMsg->ZclMsgAccessory.ClusterID);
				break;
			}
			break;
		case CLUSTER_ALARMS:
			switch(pZclMsg->ZclMsgHdr.CmdID){
			case CMD_ALARM_ALARM:
				switch(pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.ClusterID){
				case CLUSTER_POWERCFG://电压报警
					if(pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.AlarmCode==POWERCFG_MAINS_ALARM_VOLT_TOO_LOW){//电压过低
						//读取当前电压
						pAttriList[0]=ATTRIID_POWERCFG_BATTERYVOLTAGE;
						rt=ZclAttriRead(GlobalZigBeeMStatus.pZclInst_HA, (ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.isRouteAvailable)?(&ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.Route):NULL, ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.NwkAddr, ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.EP, CLUSTER_POWERCFG, 1, pAttriList, &mZCL_ATTRIREAD_R);
						if(rt!=0){
							DEBUGLOG1(3,"Error, ZclAttriRead failed %d.\n",rt);
						}else{
							if(mZCL_ATTRIREAD_R.RecordNum>0){
								if(mZCL_ATTRIREAD_R.Records[0].Status==0){
									BatteryVoltage=mZCL_ATTRIREAD_R.Records[0].AttriData.Data[0];
								}
							}
						}
						//创建事件
						pEventList[EventNum].EventType=DALEVENT_DEVICEEVENT;
						rt=fDevDBKeyWordsToDALDeviceID(&pEventList[EventNum].un.Event_DALEvent.DeviceID,&ppDeviceNode[0]->DeviceKeyWords);
						if(rt!=0){
							DEBUGLOG0(2,"Error, fDevDBKeyWordsToDALDeviceID failed.\n");
						}else{
							pEventList[EventNum].Priority=PRI_NORMAL;
							pEventList[EventNum].un.Event_DALEvent.DALEventID=DAL_IRINTRUSIONALARM_LOWBATTERY;
							pEventList[EventNum].un.Event_DALEvent.ParamNum=1;
							pEventList[EventNum].un.Event_DALEvent.ParamList[0].DataType=DAL_UINT8;
							pEventList[EventNum].un.Event_DALEvent.ParamList[0].Data[0]=BatteryVoltage;
							EventNum++;
						}
					}else{
						DEBUGLOG1(3,"Warning, unsupported AlarmCode %d for cluster HA_GEN_POWERCFG, ignored.\n",pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.AlarmCode);
					}
					break;
				default:
					DEBUGLOG1(3,"Warning, received an alarm from cluster 0x%04X, which is not supported yet.\n",pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.ClusterID);
					break;
				}
				break;
			default:
				DEBUGLOG1(3,"Warning, unsupported cmd %d for cluster HA_GEN_ALARM.\n",pZclMsg->ZclMsgHdr.CmdID);
				break;
			}
			break;
		default:
			DEBUGLOG1(3,"Error, unsupported cluster 0x%04x.\n",pZclMsg->ZclMsgAccessory.ClusterID);
			break;
		}
	}

	//推送事件
	for(i=0;i<EventNum;i++){
		Status=DAL_Event_Push(pEventList+i);
		if(Status!=DALS_SUCCESS){
			DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
		}
	}

	//释放系统资源
	if(ppDeviceNode[0]!=NULL){
		fDevDB_DestroyDeviceNode(ppDeviceNode[0]);
	}
}

//HA的ZCL默认消息处理回调
void DefaultZclMsgCB_HA(stZclMsg *pZclMsg)
{
	switch(pZclMsg->ZclMsgHdr.FrameCtrl.FrameType){
	case ZCL_FRAMETYPE_PROFILE:
		switch(pZclMsg->ZclMsgHdr.CmdID){
		case ZCL_GENERAL_ATTRIREPORT:
			ZclMsgHandler_ZCL_GENERAL_ATTRIREPORT_HA(pZclMsg);
			break;
		default:
			DEBUGLOG2(2,"Warning, unknown cmd %d (TranSeqNum %d) of ZCL_FRAMETYPE_PROFILE.\n",pZclMsg->ZclMsgHdr.CmdID,pZclMsg->ZclMsgHdr.TranSeqNum);
			break;
		}
		break;
	case ZCL_FRAMETYPE_CLUSTER://来自于具体Cluster的Zcl消息
		ZclMsgHandler_ZCLMsgForCLUSTER_HA(pZclMsg);
		break;
	default:
		DEBUGLOG1(2,"Warning, unknown FrameType %d.\n",pZclMsg->ZclMsgHdr.FrameCtrl.FrameType);
		break;
	}
}

void ZclMsgHandler_ZCL_GENERAL_ATTRIREPORT_ITS(stZclMsg *pZclMsg)
{
	int rt,isErr=0;;
	stDevDB_SearchKey SearchKey;
	stDevDB_DeviceNode *ppDeviceNode[1]={NULL};
	stEvent Event;
	DALStatus Status;
	//unClusterCmd ClusterCmd;

	memset(&Event,0,sizeof(stEvent));

	//找到发送消息的设备在数据库中的记录
	SearchKey.DeviceType=DEVTYPE_ZIGBEE;
	SearchKey.un.SearchKey_ZigBee.AddrType=0;//0-NwkAddr、1-IEEEAddr
	SearchKey.un.SearchKey_ZigBee.unAddr.NwkAddr=pZclMsg->ZclMsgAccessory.SrcAddr;
	SearchKey.un.SearchKey_ZigBee.doMatchEP=1;
	SearchKey.un.SearchKey_ZigBee.EP=pZclMsg->ZclMsgAccessory.SrcEP;
	rt=fDevDB_FindDeviceNode(&SearchKey,1,ppDeviceNode);
	if(rt==0){
		DEBUGLOG1(2,"Warning, can't find ITS device node record (NwkAddr=0x%04X) for incoming ZCL_GENERAL_ATTRIREPORT msg, ignored.\n",pZclMsg->ZclMsgAccessory.SrcAddr);
		//rt=fZigBeeDeviceEnumerate(pZclMsg->ZclMsgAccessory.SrcAddr);
		//if(rt!=0){
		//	//之前的枚举动作没有成功，则尝试向设备发出自我介绍请求
		//	DEBUGLOG1(2,"Device enumerating failed %d, try to reuqest the device to make a self introduction.\n",rt);
		//	memset(&ClusterCmd,0,sizeof(unClusterCmd));
		//	rt=ZclClusterCtrl(GlobalZigBeeMStatus.pZclInst_ITS, NULL, pZclMsg->ZclMsgAccessory.SrcAddr, pZclMsg->ZclMsgAccessory.SrcEP, CLUSTER_SELFINTRODUCTION, CMD_SELFINTRODUCTION_INTRODUCE_REQ, &ClusterCmd, 0);
		//	if(rt!=0){
		//		DEBUGLOG0(2,"Warning, failed to send out the CMD_SELFINTRODUCTION_INTRODUCE_REQ.\n");
		//	}
		//}
		isErr=1;
	}else{
		switch(ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.DeviceID){
		case DEV_ITS_VEHICLE_PRESENCE_SENSOR_GEOMAG:
			switch(pZclMsg->ZclMsgAccessory.ClusterID){
			case CLUSTER_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG:
				//配合CC2530部分的设计，如果报告参数只有一个，则说明是车位停放状态发生了变化
				if(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriNum==1){
					//创建DAL事件----------------
					Event.EventType=DALEVENT_DEVICEEVENT;
					rt=fDevDBKeyWordsToDALDeviceID(&Event.un.Event_DALEvent.DeviceID,&ppDeviceNode[0]->DeviceKeyWords);
					if(rt!=0){
						DEBUGLOG0(2,"Error, fDevDBKeyWordsToDALDeviceID failed.\n");
						isErr=1;
					}else{
						Event.Priority=PRI_LOW;
						Event.un.Event_DALEvent.DALEventID=DAL_VEHICLEPRESENCESENSORGEOMAG_PARKING;
						Event.un.Event_DALEvent.ParamNum=1;
						//检测结果
						Event.un.Event_DALEvent.ParamList[0].DataType=DAL_BOOL;
						Event.un.Event_DALEvent.ParamList[0].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[0].AttriData.Data[0];
					}
				}else{//否则是状态报告
					//创建DAL事件----------------
					Event.EventType=DALEVENT_DEVICEEVENT;
					rt=fDevDBKeyWordsToDALDeviceID(&Event.un.Event_DALEvent.DeviceID,&ppDeviceNode[0]->DeviceKeyWords);
					if(rt!=0){
						DEBUGLOG0(2,"Error, fDevDBKeyWordsToDALDeviceID failed.\n");
						isErr=1;
					}else{
						Event.Priority=PRI_LOW;
						Event.un.Event_DALEvent.DALEventID=DAL_VEHICLEPRESENCESENSORGEOMAG_STATEREPORT;
						//检测结果
						Event.un.Event_DALEvent.ParamList[0].DataType=DAL_BOOL;
						Event.un.Event_DALEvent.ParamList[0].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[0].AttriData.Data[0];
						//电池电压
						Event.un.Event_DALEvent.ParamList[1].DataType=DAL_UINT8;
						Event.un.Event_DALEvent.ParamList[1].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[1].AttriData.Data[0];
						//温度
						Event.un.Event_DALEvent.ParamList[2].DataType=DAL_INT8;
						Event.un.Event_DALEvent.ParamList[2].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[2].AttriData.Data[0];
						//WMODE
						Event.un.Event_DALEvent.ParamList[3].DataType=DAL_UINT8;
						Event.un.Event_DALEvent.ParamList[3].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[3].AttriData.Data[0];
						if(Event.un.Event_DALEvent.ParamList[3].Data[0]==0){//WMODE==0
							Event.un.Event_DALEvent.ParamNum=4;
						}else{
							Event.un.Event_DALEvent.ParamNum=22;
							//当前测量值
							Event.un.Event_DALEvent.ParamList[4].DataType=DAL_INT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[4].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[4].AttriData.Data));
							Event.un.Event_DALEvent.ParamList[5].DataType=DAL_INT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[5].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[5].AttriData.Data));
							Event.un.Event_DALEvent.ParamList[6].DataType=DAL_INT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[6].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[6].AttriData.Data));
							//背景值
							Event.un.Event_DALEvent.ParamList[7].DataType=DAL_SINGLEFLOAT;
							memcpy(Event.un.Event_DALEvent.ParamList[7].Data,pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[7].AttriData.Data,4);
							Event.un.Event_DALEvent.ParamList[8].DataType=DAL_SINGLEFLOAT;
							memcpy(Event.un.Event_DALEvent.ParamList[8].Data,pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[8].AttriData.Data,4);
							Event.un.Event_DALEvent.ParamList[9].DataType=DAL_SINGLEFLOAT;
							memcpy(Event.un.Event_DALEvent.ParamList[9].Data,pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[9].AttriData.Data,4);
							//TS
							Event.un.Event_DALEvent.ParamList[10].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[10].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[10].AttriData.Data));
							//TDMA
							Event.un.Event_DALEvent.ParamList[11].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[11].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[11].AttriData.Data));
							//TDME
							Event.un.Event_DALEvent.ParamList[12].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[12].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[12].AttriData.Data));
							//TBME
							Event.un.Event_DALEvent.ParamList[13].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[13].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[13].AttriData.Data));
							//TPZ
							Event.un.Event_DALEvent.ParamList[14].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[14].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[14].AttriData.Data));
							//TP3
							Event.un.Event_DALEvent.ParamList[15].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[15].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[15].AttriData.Data));
							//RHMN
							Event.un.Event_DALEvent.ParamList[16].DataType=DAL_UINT8;
							Event.un.Event_DALEvent.ParamList[16].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[16].AttriData.Data[0];
							//PMF
							Event.un.Event_DALEvent.ParamList[17].DataType=DAL_UINT8;
							Event.un.Event_DALEvent.ParamList[17].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[17].AttriData.Data[0];
							//PMN
							Event.un.Event_DALEvent.ParamList[18].DataType=DAL_UINT8;
							Event.un.Event_DALEvent.ParamList[18].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[18].AttriData.Data[0];
							//PRPT
							Event.un.Event_DALEvent.ParamList[19].DataType=DAL_UINT8;
							Event.un.Event_DALEvent.ParamList[19].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[19].AttriData.Data[0];
							//TPVS
							Event.un.Event_DALEvent.ParamList[20].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[20].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[20].AttriData.Data));
							//TPVL
							Event.un.Event_DALEvent.ParamList[21].DataType=DAL_UINT16;
							UNALIGEWRITE_16BIT(Event.un.Event_DALEvent.ParamList[21].Data,UNALIGEREAD_16BIT(pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[21].AttriData.Data));
						}
					}
				}
				break;
			default:
				DEBUGLOG3(3,"Error, unexpected event for Cluster 0x%04x of Device 0x%04x in profile 0x%04x.\n",pZclMsg->ZclMsgAccessory.ClusterID,ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.DeviceID,ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.ProfileID);
				isErr=1;
				break;
			}
			break;
		case DEV_ITS_ROUTER_REPEATER:
			//创建DAL事件----------------
			Event.EventType=DALEVENT_DEVICEEVENT;
			rt=fDevDBKeyWordsToDALDeviceID(&Event.un.Event_DALEvent.DeviceID,&ppDeviceNode[0]->DeviceKeyWords);
			if(rt!=0){
				DEBUGLOG0(2,"Error, fDevDBKeyWordsToDALDeviceID failed.\n");
				isErr=1;
			}else{
				Event.Priority=PRI_LOW;
				Event.un.Event_DALEvent.DALEventID=DAL_ITS_ROUTER_REPEATER_STATEREPORT;
				Event.un.Event_DALEvent.ParamNum=1;
				Event.un.Event_DALEvent.ParamList[0].DataType=DAL_UINT8;
				Event.un.Event_DALEvent.ParamList[0].Data[0]=pZclMsg->un.GeneralCmd.mZCL_ATTRIREPORT.AttriReportRecord[0].AttriData.Data[0];
			}
			break;
		default:
			DEBUGLOG1(3,"Error, unknown DeviceID 0x%04X for ITS profile.\n",ppDeviceNode[0]->pDeviceNode->DevNode_ZigBee.DeviceID);
			isErr=1;
			break;
		}
	}

	//推送DAL事件
	if(!isErr){
		Status=DAL_Event_Push(&Event);
		if(Status!=DALS_SUCCESS){
			DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
		}
	}

	//释放系统资源
	if(ppDeviceNode[0]!=NULL){
		fDevDB_DestroyDeviceNode(ppDeviceNode[0]);
	}
}

void ZclMsgHandler_ZCLMsgForCLUSTER_ITS(stZclMsg *pZclMsg)
{
	int i,rt,EventNum=0;
	stDevDB_SearchKey SearchKey;
	stDevDB_DeviceNode *ppDeviceNode[1]={NULL};
	stDevDB_DeviceNode *pNewDeviceNode=NULL;
	stEvent pEventList[2];
	DALStatus Status;
	stEvent Event;

	if(pZclMsg==NULL){
		return;
	}
	memset(pEventList,0,sizeof(stEvent)*2);

	//找到发送消息的设备在数据库中的记录
	SearchKey.DeviceType=DEVTYPE_ZIGBEE;
	SearchKey.un.SearchKey_ZigBee.AddrType=0;//0-NwkAddr、1-IEEEAddr
	SearchKey.un.SearchKey_ZigBee.unAddr.NwkAddr=pZclMsg->ZclMsgAccessory.SrcAddr;
	SearchKey.un.SearchKey_ZigBee.doMatchEP=1;
	SearchKey.un.SearchKey_ZigBee.EP=pZclMsg->ZclMsgAccessory.SrcEP;
	rt=fDevDB_FindDeviceNode(&SearchKey,1,ppDeviceNode);
	if(rt==0){
		//首先检查是不是自我介绍，如果是自我介绍，则创建设备节点
		if(pZclMsg->ZclMsgAccessory.ClusterID==CLUSTER_SELFINTRODUCTION){
			if(pZclMsg->ZclMsgHdr.CmdID==CMD_SELFINTRODUCTION_INTRODUCE_RSP){
				DEBUGLOG1(2,"Received a self introduction from 0x%04X, try to creat a device node.\n",pZclMsg->ZclMsgAccessory.SrcAddr);
				//建立节点
				pNewDeviceNode=fDevDB_CreatDeviceNode(DEVTYPE_ZIGBEE);
				if(pNewDeviceNode!=NULL){
					pNewDeviceNode->pDeviceNode->DevNode_ZigBee.DeviceID=pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.DeviceID;
					pNewDeviceNode->pDeviceNode->DevNode_ZigBee.EP=pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.EP;
					memcpy(pNewDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr,pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.IEEEAddr,8);
					pNewDeviceNode->pDeviceNode->DevNode_ZigBee.NumInClusters=pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.NumInClusters;
					memcpy(pNewDeviceNode->pDeviceNode->DevNode_ZigBee.InClusterList,pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.InClusterIDList,pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.NumInClusters*2);
					pNewDeviceNode->pDeviceNode->DevNode_ZigBee.NumOutClusters=pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.NumOutClusters;
					memcpy(pNewDeviceNode->pDeviceNode->DevNode_ZigBee.OutClusterList,pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.OutClusterIDList,pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.NumOutClusters*2);
					pNewDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr=pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.NwkAddr;
					pNewDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID=pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.ProfileID;
					memcpy(pNewDeviceNode->pDeviceNode->DevNode_ZigBee.UserDescriptor,pZclMsg->un.ClusterCmd.unClusterCmd_SelfIntroduction.mCMD_SELFINTRODUCTION_INTRODUCE_RSP.UserDescriptor,16);
					pNewDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable=0;

					fGenKeyInfo_ZigBee2(&pNewDeviceNode->pDeviceNode->DevNode_ZigBee, pNewDeviceNode->DeviceKeyWords.DeviceKeyInfo);
					pNewDeviceNode->DeviceKeyWords.DeviceType=DEVTYPE_ZIGBEE;
					pNewDeviceNode->DeviceType=DEVTYPE_ZIGBEE;
					snprintf(pNewDeviceNode->DeviceName,MAXDEVICENAME,"NewDevice");
					snprintf(pNewDeviceNode->UserDescript,MAXUSRDESC,"This is a new zigbee device");

					//加入数据库
					rt=fDevDB_AddDevice(pNewDeviceNode);
					if(rt==0){//新节点创建成功，推送新设备加入消息
						Event.EventType=DALEVENT_DEVICEJIONED;
						Event.Priority=PRI_NORMAL;
						rt=fDevDBKeyWordsToDALDeviceID(&Event.un.Event_DeviceJoined.DeviceID,&pNewDeviceNode->DeviceKeyWords);
						if(rt!=0){
							DEBUGLOG1(3,"Error, fDevDBKeyWordsToDALDeviceID failed %d.\n",rt);
						}else{
							Status=DAL_Event_Push(&Event);
							if(Status!=DALS_SUCCESS){
								DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
							}
						}
					}else if(rt==1){//替换了一个已经存在的节点
						DEBUGLOG2(1,"Updated an existing device %02X%02X.\n",pNewDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[1],pNewDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[0]);
					}else{//出错
						DEBUGLOG1(3,"Error, adding device node to db failed %d.\n",rt);
					}
					//销毁临时的新节点
					fDevDB_DestroyDeviceNode(pNewDeviceNode);
				}
			}
		}else{//不是自我介绍
			DEBUGLOG1(2,"Warning, can't find device node record (NwkAddr=0x%04X) for incoming ZCL_CLUSTER msg, ignored.\n",pZclMsg->ZclMsgAccessory.SrcAddr);
			//rt=fZigBeeDeviceEnumerate(pZclMsg->ZclMsgAccessory.SrcAddr);
			//if(rt!=0){
			//	DEBUGLOG1(2,"Device enumerating failed %d.\n",rt);
			//}
		}
	}else{
		switch(pZclMsg->ZclMsgAccessory.ClusterID){
		case CLUSTER_ALARMS:
			switch(pZclMsg->ZclMsgHdr.CmdID){
			case CMD_ALARM_ALARM:
				switch(pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.ClusterID){
				case CLUSTER_POWERCFG://电压报警
					if(pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.AlarmCode==POWERCFG_MAINS_ALARM_VOLT_TOO_LOW){//电压过低
						//创建事件
						pEventList[EventNum].EventType=DALEVENT_DEVICEEVENT;
						rt=fDevDBKeyWordsToDALDeviceID(&pEventList[EventNum].un.Event_DALEvent.DeviceID,&ppDeviceNode[0]->DeviceKeyWords);
						if(rt!=0){
							DEBUGLOG0(2,"Error, fDevDBKeyWordsToDALDeviceID failed.\n");
						}else{
							pEventList[EventNum].Priority=PRI_NORMAL;
							pEventList[EventNum].un.Event_DALEvent.DALEventID=DAL_VEHICLEPRESENCESENSORGEOMAG_LOWBETTERY;
							pEventList[EventNum].un.Event_DALEvent.ParamNum=1;
							pEventList[EventNum].un.Event_DALEvent.ParamList[0].DataType=DAL_UINT8;
							pEventList[EventNum].un.Event_DALEvent.ParamList[0].Data[0]=0;//电池电压填0，因为传感器采用低功耗主动通信方式，无法读取电压值
							EventNum++;
						}
					}else{
						DEBUGLOG1(3,"Warning, unsupported AlarmCode %d for cluster CLUSTER_POWERCFG, ignored.\n",pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.AlarmCode);
					}
					break;
				default:
					DEBUGLOG1(3,"Warning, received an alarm from cluster 0x%04X, which is not supported yet.\n",pZclMsg->un.ClusterCmd.unClusterCmd_Alarm.mCMD_ALARM_ALARM.ClusterID);
					break;
				}
				break;
			default:
				DEBUGLOG1(3,"Warning, unsupported cmd %d for cluster HA_GEN_ALARM.\n",pZclMsg->ZclMsgHdr.CmdID);
				break;
			}
			break;
		case CLUSTER_SELFINTRODUCTION://已有设备在做自我介绍，忽略
			DEBUGLOG1(3,"Received a self introduction from a known device 0x%04X, ignored.\n",pZclMsg->ZclMsgAccessory.SrcAddr);
			break;
		default:
			DEBUGLOG1(3,"Error, unsupported cluster 0x%04x.\n",pZclMsg->ZclMsgAccessory.ClusterID);
			break;
		}
	}

	//推送事件
	for(i=0;i<EventNum;i++){
		Status=DAL_Event_Push(pEventList+i);
		if(Status!=DALS_SUCCESS){
			DEBUGLOG1(3,"Error, DAL_Event_Push failed %d.\n",Status);
		}
	}

	//释放系统资源
	if(ppDeviceNode[0]!=NULL){
		fDevDB_DestroyDeviceNode(ppDeviceNode[0]);
	}
}

//HA的ZCL默认消息处理回调
void DefaultZclMsgCB_ITS(stZclMsg *pZclMsg)
{
	switch(pZclMsg->ZclMsgHdr.FrameCtrl.FrameType){
	case ZCL_FRAMETYPE_PROFILE:
		switch(pZclMsg->ZclMsgHdr.CmdID){
		case ZCL_GENERAL_ATTRIREPORT:
			ZclMsgHandler_ZCL_GENERAL_ATTRIREPORT_ITS(pZclMsg);
			break;
		default:
			DEBUGLOG2(2,"Warning, unknown cmd %d (TranSeqNum %d) of ZCL_FRAMETYPE_PROFILE.\n",pZclMsg->ZclMsgHdr.CmdID,pZclMsg->ZclMsgHdr.TranSeqNum);
			break;
		}
		break;
	case ZCL_FRAMETYPE_CLUSTER://来自于具体Cluster的Zcl消息
		ZclMsgHandler_ZCLMsgForCLUSTER_ITS(pZclMsg);
		break;
	default:
		DEBUGLOG1(2,"Warning, unknown FrameType %d.\n",pZclMsg->ZclMsgHdr.FrameCtrl.FrameType);
		break;
	}
}

DALStatus ZigBeeM_Init(stZigBeeMCfg *pZigBeeMCfg)
{
	int rt,len;
	stZnpCfg ZnpCfg;
	stZclCfg ZclCfg;
	stZclInstRegistParam ZclInstRegistParam;

	//初始化全局参数
	memset(&GlobalZigBeeMStatus,0,sizeof(stZigBeeMStatus));

	//初始化ZNP
	memset(&ZnpCfg,0,sizeof(stZnpCfg));
	ZnpCfg.pZNPComPath=pZigBeeMCfg->pZNPComPath;
	ZnpCfg.pDefaultZnpMsgCB=DefaultZnpMsgCB;
	ZnpCfg.isValid_IEEEAddr=pZigBeeMCfg->isValid_IEEEAddr;
	memcpy(ZnpCfg.IEEEAddr,pZigBeeMCfg->IEEEAddr,8);
	ZnpCfg.isValid_PANID=pZigBeeMCfg->isValid_PANID;
	ZnpCfg.PANID=pZigBeeMCfg->PANID;
	ZnpCfg.isValid_ExtPANID=pZigBeeMCfg->isValid_ExtPANID;
	memcpy(ZnpCfg.ExtPANID,pZigBeeMCfg->ExtPANID,8);
	rt=ZnpInit(&ZnpCfg);
	if(rt!=0){
		DEBUGLOG1(3,"Error, ZnpInit failed %d.\n",rt);
		return DALS_ZIGBEEINITFAILED;
	}

	//获取本地的IEEE地址
	len=8;
	rt=Znp_ZB_GET_DEVICE_INFO(1, &len, GlobalZigBeeMStatus.LocalIEEEAddr);
	if(rt!=0){
		DEBUGLOG1(3,"Error, Znp_ZB_GET_DEVICE_INFO failed %d.\n",rt);
		return DALS_ZIGBEEINITFAILED;
	}

	//初始化ZCL
	memset(&ZclCfg,0,sizeof(stZclCfg));
	rt=ZclInit(&ZclCfg);
	if(rt!=0){
		DEBUGLOG1(3,"Error, ZclInit failed %d.\n",rt);
		return DALS_ZIGBEEINITFAILED;
	}

	//注册HA协议接口的Zcl实例
	ZclInstRegistParam.EndPoint=ENDPOINT_HA;
	ZclInstRegistParam.Profile=PROFILE_HA;
	ZclInstRegistParam.pCBFxn=DefaultZclMsgCB_HA;
	GlobalZigBeeMStatus.pZclInst_HA=ZclRegistInst(&ZclInstRegistParam);
	if(GlobalZigBeeMStatus.pZclInst_HA==NULL){
		DEBUGLOG0(3,"Error, ZclRegistInst for HA failed.\n");
		return DALS_ZIGBEEINITFAILED;
	}

	//注册ITS协议接口的Zcl实例
	ZclInstRegistParam.EndPoint=ENDPOINT_ITS;
	ZclInstRegistParam.Profile=PROFILE_ITS;
	ZclInstRegistParam.pCBFxn=DefaultZclMsgCB_ITS;
	GlobalZigBeeMStatus.pZclInst_ITS=ZclRegistInst(&ZclInstRegistParam);
	if(GlobalZigBeeMStatus.pZclInst_ITS==NULL){
		DEBUGLOG0(3,"Error, ZclRegistInst for ITS failed.\n");
		return DALS_ZIGBEEINITFAILED;
	}

	return DALS_SUCCESS;
}

//pZigBeeMDetails - 用于返回获取结果，
DALStatus ZigBeeM_GetModuleDetails(stDALSubMDetails_ZigBee *pZigBeeMDetails)
{
	int rt,len;
	unsigned char pBuff[8];

	if(pZigBeeMDetails==NULL){
		return DALS_ILLEGPARAM;
	}

	pZigBeeMDetails->ZigBeeMVersion=0x0002;

	//LocalIEEEAddr
	len=8;
	rt=Znp_ZB_GET_DEVICE_INFO(1, &len, pZigBeeMDetails->LocalIEEEAddr);
	if(rt!=0){
		DEBUGLOG1(3,"Error, Znp_ZB_GET_DEVICE_INFO failed %d to get LocalIEEEAddr.\n",rt);
		return DALS_ZIGBEEFAILED;
	}
	//PANID
	len=8;
	rt=Znp_ZB_GET_DEVICE_INFO(6, &len, pBuff);
	if(rt!=0){
		DEBUGLOG1(3,"Error, Znp_ZB_GET_DEVICE_INFO failed %d to get PANID.\n",rt);
		return DALS_ZIGBEEFAILED;
	}
	pZigBeeMDetails->PANID=UNALIGEREAD_16BIT(pBuff);

	//ExtPANID
	len=8;
	rt=Znp_ZB_GET_DEVICE_INFO(7, &len,(unsigned char *)&pZigBeeMDetails->ExtPANID);
	if(rt!=0){
		DEBUGLOG1(3,"Error, Znp_ZB_GET_DEVICE_INFO failed %d to get ExtPANID.\n",rt);
		return DALS_ZIGBEEFAILED;
	}

	return DALS_SUCCESS;
}

DALStatus ZigBeeM_DeviceCtrl(stDevDB_DeviceNode *pDeviceNode, unsigned int DALDevType, stDALCtrl *pDALCtrl)
{
	int rt;
	DALStatus Status=DALS_SUCCESS;
	unClusterCmd ClusterCmd;

	if((pDeviceNode==NULL)||(pDALCtrl==NULL)){
		return DALS_ILLEGPARAM;
	}

	switch(DALDevType){
	case DAL_ONOFFLIGHT:
		switch(pDALCtrl->CtrlID){
		case DAL_ONOFFLIGHT_ONOFFCTRL:
			switch(pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID){
			case PROFILE_HA:
				rt=ZclClusterCtrl(GlobalZigBeeMStatus.pZclInst_HA, (pDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable)?&pDeviceNode->pDeviceNode->DevNode_ZigBee.Route:NULL, pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr, pDeviceNode->pDeviceNode->DevNode_ZigBee.EP, CLUSTER_ONOFF, (pDALCtrl->ParamList[0].Data[0]==0x00)?CMD_ONOFF_OFF:CMD_ONOFF_ON, NULL, 1);
				if(rt!=0){
					DEBUGLOG0(2,"Warning, ZclClusterCtrl failed.\n");
					Status=DALS_GENERALERR;
				}
				break;
			default:
				DEBUGLOG1(3,"Error, unsupported ZigBee profile 0x%04X.\n",pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID);
				Status=DALS_UNSUPPORTEDDEVICETYPE;
				break;
			}
			break;
		default:
			DEBUGLOG2(3,"Error, unknown CtrlID 0x%02X for DALDevType 0x%08X.\n",pDALCtrl->CtrlID,DALDevType);
			Status=DALS_GENERALERR;
			break;
		}
		break;
	case DAL_IRINTRUSIONALARM:
		switch(pDALCtrl->CtrlID){
		case DAL_IRINTRUSIONALARM_TEST:
			switch(pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID){
			case PROFILE_HA:
				ClusterCmd.unClusterCmd_Identify.mCMD_IDENTIFY_IDENTIFY.IdentifyTime=10;//10s
				rt=ZclClusterCtrl(GlobalZigBeeMStatus.pZclInst_HA, (pDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable)?&pDeviceNode->pDeviceNode->DevNode_ZigBee.Route:NULL, pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr, pDeviceNode->pDeviceNode->DevNode_ZigBee.EP, CLUSTER_IDENTIFY, CMD_IDENTIFY_IDENTIFY, &ClusterCmd, 1);
				if(rt!=0){
					DEBUGLOG0(2,"Warning, ZclClusterCtrl failed.\n");
					Status=DALS_GENERALERR;
				}
				break;
			default:
				DEBUGLOG1(3,"Error, unsupported ZigBee profile 0x%04X.\n",pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID);
				Status=DALS_UNSUPPORTEDDEVICETYPE;
				break;
			}
			break;
		default:
			DEBUGLOG2(3,"Error, unknown CtrlID 0x%02X for DALDevType 0x%08X.\n",pDALCtrl->CtrlID,DALDevType);
			Status=DALS_GENERALERR;
			break;
		}
		break;
	case DAL_VEHICLEPRESENCESENSORGEOMAG:
		switch(pDALCtrl->CtrlID){
		case DAL_VEHICLEPRESENCESENSORGEOMAG_ADJUST:
			DEBUGLOG2(0,"Calling DAL_VEHICLEPRESENCESENSORGEOMAG_ADJUST for 0x%04X:%d.\n",pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr,pDeviceNode->pDeviceNode->DevNode_ZigBee.EP);
			//ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_ADJUST
			rt=ZclClusterCtrl(GlobalZigBeeMStatus.pZclInst_ITS, (pDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable)?&pDeviceNode->pDeviceNode->DevNode_ZigBee.Route:NULL, pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr, pDeviceNode->pDeviceNode->DevNode_ZigBee.EP, CLUSTER_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG, DAL_VEHICLEPRESENCESENSORGEOMAG_ADJUST, NULL, 0);
			if(rt!=0){
				DEBUGLOG0(2,"Warning, ZclClusterCtrl failed.\n");
				Status=DALS_GENERALERR;
			}
			break;
		case DAL_VEHICLEPRESENCESENSORGEOMAG_CONFIG:
			if(pDALCtrl->ParamNum!=13){
				DEBUGLOG0(3,"Error, need more config parameters for DAL_VEHICLEPRESENCESENSORGEOMAG_CONFIG.\n");
				Status=DALS_ILLEGPARAM;
			}else{
				DEBUGLOG2(0,"Calling DAL_VEHICLEPRESENCESENSORGEOMAG_CONFIG for 0x%04X:%d.\n",pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr,pDeviceNode->pDeviceNode->DevNode_ZigBee.EP);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.WMODE,	pDALCtrl->ParamList[0].Data,1);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TS,		pDALCtrl->ParamList[1].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TDMA,	pDALCtrl->ParamList[2].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TDME,	pDALCtrl->ParamList[3].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TBME,	pDALCtrl->ParamList[4].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TPZ,	pDALCtrl->ParamList[5].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TP3,	pDALCtrl->ParamList[6].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.RHMN,	pDALCtrl->ParamList[7].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.PMF,	pDALCtrl->ParamList[8].Data,1);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.PMN,	pDALCtrl->ParamList[9].Data,1);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.PRPT,	pDALCtrl->ParamList[10].Data,1);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TPVS,	pDALCtrl->ParamList[11].Data,2);
				memcpy(&ClusterCmd.unClusterCmd_ITS_Vehicle_Presence_Sensing_Geomag.mCMD_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG_CONFIG.TPVL,	pDALCtrl->ParamList[12].Data,2);
				rt=ZclClusterCtrl(GlobalZigBeeMStatus.pZclInst_ITS, (pDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable)?&pDeviceNode->pDeviceNode->DevNode_ZigBee.Route:NULL, pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr, pDeviceNode->pDeviceNode->DevNode_ZigBee.EP, CLUSTER_ITS_VEHICLE_PRESENCE_SENSING_GEOMAG, DAL_VEHICLEPRESENCESENSORGEOMAG_CONFIG, &ClusterCmd, 0);
				if(rt!=0){
					DEBUGLOG0(2,"Warning, ZclClusterCtrl failed.\n");
					Status=DALS_GENERALERR;
				}
			}
			break;
		default:
			DEBUGLOG2(3,"Error, unknown CtrlID 0x%02X for DALDevType 0x%08X.\n",pDALCtrl->CtrlID,DALDevType);
			Status=DALS_GENERALERR;
			break;
		}
		break;
	default:
		DEBUGLOG1(3,"Error, unsupported DALDevType 0x%08X.\n",DALDevType);
		Status=DALS_GENERALERR;
		break;
	}
	return Status;
}

DALStatus ZigBeeM_GetDeviceStatus(stDevDB_DeviceNode *pDeviceNode, unsigned int DALDevType, stDALStatus *pDALStatus)
{
	int rt;
	unsigned char AttriNum;
	unsigned short AttriIDList[MAXSTATUSNUM];
	stZCL_ATTRIREAD_R Zcl_AttriRead_R;
	DALStatus Status=DALS_SUCCESS;

	if((pDeviceNode==NULL)||(pDALStatus==NULL)){
		return DALS_ILLEGPARAM;
	}

	switch(DALDevType){
	case DAL_ONOFFLIGHT:
		switch(pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID){
		case PROFILE_HA:
			if(pDeviceNode->pDeviceNode->DevNode_ZigBee.DeviceID==DEV_HA_ON_OFF_LIGHT){
				AttriNum=1;
				AttriIDList[0]=ATTRIID_ONOFF_ONOFF;
				rt=ZclAttriRead(GlobalZigBeeMStatus.pZclInst_HA, (pDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable)?&pDeviceNode->pDeviceNode->DevNode_ZigBee.Route:NULL, pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr, pDeviceNode->pDeviceNode->DevNode_ZigBee.EP, CLUSTER_ONOFF, AttriNum, AttriIDList, &Zcl_AttriRead_R);
				if(rt!=0){
					DEBUGLOG1(3,"Error, ZclAttriRead failed %d.\n",rt);
					Status=DALS_GENERALERR;
				}else{
					if(Zcl_AttriRead_R.Records[0].Status!=0){
						DEBUGLOG0(3,"Error, ZclAttriRead failed.\n");
						Status=DALS_GENERALERR;
					}else{
						pDALStatus->StatusNum=1;
						pDALStatus->StatusList[0].DataType=DAL_BOOL;
						pDALStatus->StatusList[0].Data[0]=Zcl_AttriRead_R.Records[0].AttriData.Data[0];
					}
				}
			}else{
				DEBUGLOG0(3,"Error, target ZigBee device is not a DEV_HA_ON_OFF_LIGHT device while DALDevType is DAL_ONOFFLIGHT.\n");
				Status=DALS_UNSUPPORTEDDEVICETYPE;
			}
			break;
		default:
			DEBUGLOG1(3,"Error, unsupported ZigBee profile 0x%04X.\n",pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID);
			Status=DALS_UNSUPPORTEDDEVICETYPE;
			break;
		}
		break;
	case DAL_IRINTRUSIONALARM:
		switch(pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID){
		case PROFILE_HA:
			if(pDeviceNode->pDeviceNode->DevNode_ZigBee.DeviceID==DEV_HA_IAS_ZONE){
				AttriNum=1;
				AttriIDList[0]=ATTRIID_POWERCFG_BATTERYVOLTAGE;
				rt=ZclAttriRead(GlobalZigBeeMStatus.pZclInst_HA, (pDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable)?&pDeviceNode->pDeviceNode->DevNode_ZigBee.Route:NULL, pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr, pDeviceNode->pDeviceNode->DevNode_ZigBee.EP, CLUSTER_POWERCFG, AttriNum, AttriIDList, &Zcl_AttriRead_R);
				if(rt!=0){
					DEBUGLOG1(3,"Error, ZclAttriRead failed %d.\n",rt);
					Status=DALS_GENERALERR;
				}else{
					if(Zcl_AttriRead_R.Records[0].Status!=0){
						DEBUGLOG0(3,"Error, ZclAttriRead failed.\n");
						Status=DALS_GENERALERR;
					}else{
						pDALStatus->StatusNum=1;
						pDALStatus->StatusList[0].DataType=DAL_UINT8;
						pDALStatus->StatusList[0].Data[0]=Zcl_AttriRead_R.Records[0].AttriData.Data[0];
					}
				}
			}else{
				DEBUGLOG0(3,"Error, target ZigBee device is not a DEV_HA_IAS_ZONE device while DALDevType is DAL_IRINTRUSIONALARM.\n");
				Status=DALS_UNSUPPORTEDDEVICETYPE;
			}
			break;
		default:
			DEBUGLOG1(3,"Error, unsupported ZigBee profile 0x%04X.\n",pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID);
			Status=DALS_UNSUPPORTEDDEVICETYPE;
			break;
		}
		break;
	default:
		DEBUGLOG1(3,"Error, unsupported DALDevType 0x%08X.\n",DALDevType);
		Status=DALS_UNSUPPORTEDDEVICETYPE;
		break;
	}
	return Status;
}

//pDeviceDesc_ZigBee - 需要添加的ZigBee设备描述符
DALStatus ZigBeeM_AddDevice(stDeviceDesc_ZigBee *pDeviceDesc_ZigBee)
{
	int rt;
	unsigned short NwkAddr;
	DALStatus Status=DALS_SUCCESS;
	stZB_FIND_DEVICE_CONFIRM mZB_FIND_DEVICE_CONFIRM;

	//参数检查
	if(pDeviceDesc_ZigBee==NULL){
		return DALS_ILLEGPARAM;
	}

	rt=Znp_ZB_FIND_DEVICE_REQUEST(pDeviceDesc_ZigBee->IEEEAddr, &mZB_FIND_DEVICE_CONFIRM);
	if(rt!=0){
		DEBUGLOG1(3,"Error, Znp_ZB_FIND_DEVICE_REQUEST failed %d.\n",rt);
		return DALS_DEVICENOTFOUND;
	}else{
		memcpy(&NwkAddr,mZB_FIND_DEVICE_CONFIRM.SearchKey,2);
	}

	rt=fZigBeeDeviceEnumerate(NwkAddr);
	if(rt!=0){
		Status=DALS_GENERALERR;
	}

	return Status;
}

DALStatus ZigBeeM_RemoveDevice(stDevDB_DeviceNode *pDeviceNode, stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords)
{
	int rt;
	DALStatus Status=DALS_SUCCESS;
	unsigned char DeviceAddress[8]={0,0,0,0,0,0,0,0};

	//参数检查
	if((pDeviceNode==NULL)||(pDevDB_DeviceKeyWords==NULL)){
		return DALS_ILLEGPARAM;
	}
	//从ZigBee网络中驱离设备
	rt=Znp_ZDO_MGMT_LEAVE_REQ(pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr,DeviceAddress,0,0);
	if(rt!=0){
		DEBUGLOG1(3,"Error, Znp_ZDO_MGMT_LEAVE_REQ failed %d.\n",rt);
		return DALS_ZIGBEEFAILED;
	}
	//从数据库中删除
	rt=fDevDB_RemoveDevice(pDevDB_DeviceKeyWords);
	if(rt==1){
		Status=DALS_DEVICENOTFOUND;
	}else if(rt!=0){
		DEBUGLOG1(3,"Error, fDevDB_RemoveDevice failed %d.\n",rt);
		Status=DALS_DATABASEACCESSFAILED;
	}
	return Status;
}




