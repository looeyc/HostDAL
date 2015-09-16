/*
 * hostdal_test.c
 *
 *  Created on: 2014-2-7
 *      Author: chengm
 */

#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
#include <time.h>
#include "debuglog.h"
#include "dal_event.h"
#include "dal_devtype.h"
//#include "eventproc.h"
#include "dal_errno.h"
#include "hostdal.h"
//#include "devicedb.h"
//#include "module_zigbee.h"

//int GoToTerm=0;

//static void fSigHandler(int unused)
//{
//	if(unused==SIGCHLD){//子进程终止信号不产生任何提示
//		return;
//	}
//
//	DEBUGLOG1(3,"Warning, get a signal %d.\n",unused);
//	if((unused==SIGINT)||(unused==SIGTERM)||(unused==SIGSEGV)){
//		GoToTerm=1;
//	}else{
//		DEBUGLOG0(3,"Ignored.\n");
//	}
//}

void fPrintTime()
{
	time_t now;
	struct tm *timenow=NULL;

	memset(&now,0,sizeof(time_t));

	time(&now);
	timenow = localtime(&now);
	S_DEBUGLOG1(0,"%s", asctime(timenow));
}

#define MAXGEOMAGSENSORNUM 	10
typedef struct{
	char isValid;//0-无效，表示该项为空，1-有效，表示该项非空
	stDALDeviceID DALDeviceID;
	unsigned char isParked;
	unsigned char Voltage;
	unsigned char Temp;
	unsigned char WMODE;
	short mx;
	short my;
	short mz;
	float bx;
	float by;
	float bz;
	unsigned short TS;
	unsigned short TDMA;
	unsigned short TDME;
	unsigned short TBME;
	unsigned short TPVS;
	unsigned short TPVL;
	unsigned short TPZ;
	unsigned short TP3;
	unsigned char RHMN;
	unsigned char PMF;
	unsigned char PMN;
	unsigned char PRPT;
	int ActiveCount;
	int isCorrCmdFlying;
}stGeoMagSensorNode;
stGeoMagSensorNode GeoMagSensorList[MAXGEOMAGSENSORNUM];

#define MAXROUTERNUM 		10
typedef struct{
	char isValid;//0-无效，表示该项为空，1-有效，表示该项非空
	stDALDeviceID DALDeviceID;
	unsigned char Voltage;
	int ActiveCount;
}stRouterNode;
stRouterNode RouterList[MAXROUTERNUM];

//事件处理回调
void DALEventCB(stEvent *pEvent)
{
	int i,iEntry;
	unsigned int DALDevType;

	switch(pEvent->EventType){
	case DALEVENT_DEVICEEVENT:
		memcpy(&DALDevType,pEvent->un.Event_DALEvent.DeviceID.DeviceID,4);
		switch(DALDevType){
		case DAL_VEHICLEPRESENCESENSORGEOMAG:
			switch(pEvent->un.Event_DALEvent.DALEventID){
			case DAL_VEHICLEPRESENCESENSORGEOMAG_PARKING:
				DEBUGLOG3(0,"Parking state change event %02X%02X -> %d.\t",pEvent->un.Event_DALEvent.DeviceID.DeviceID[9],pEvent->un.Event_DALEvent.DeviceID.DeviceID[8],pEvent->un.Event_DALEvent.ParamList[0].Data[0]);
				break;
			case DAL_VEHICLEPRESENCESENSORGEOMAG_LOWBETTERY:
				DEBUGLOG2(0,"Low bettery warning event %02X%02X.\t",pEvent->un.Event_DALEvent.DeviceID.DeviceID[9],pEvent->un.Event_DALEvent.DeviceID.DeviceID[8]);
				break;
			case DAL_VEHICLEPRESENCESENSORGEOMAG_STATEREPORT:
				//是否有记录？
				for(i=0;i<MAXGEOMAGSENSORNUM;i++){
					if(GeoMagSensorList[i].isValid){
						if( (pEvent->un.Event_DALEvent.DeviceID.DeviceID[0] ==GeoMagSensorList[i].DALDeviceID.DeviceID[0] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[1] ==GeoMagSensorList[i].DALDeviceID.DeviceID[1] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[2] ==GeoMagSensorList[i].DALDeviceID.DeviceID[2] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[3] ==GeoMagSensorList[i].DALDeviceID.DeviceID[3] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[4] ==GeoMagSensorList[i].DALDeviceID.DeviceID[4] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[5] ==GeoMagSensorList[i].DALDeviceID.DeviceID[5] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[6] ==GeoMagSensorList[i].DALDeviceID.DeviceID[6] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[7] ==GeoMagSensorList[i].DALDeviceID.DeviceID[7] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[8] ==GeoMagSensorList[i].DALDeviceID.DeviceID[8] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[9] ==GeoMagSensorList[i].DALDeviceID.DeviceID[9] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[10]==GeoMagSensorList[i].DALDeviceID.DeviceID[10])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[11]==GeoMagSensorList[i].DALDeviceID.DeviceID[11])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[12]==GeoMagSensorList[i].DALDeviceID.DeviceID[12])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[13]==GeoMagSensorList[i].DALDeviceID.DeviceID[13])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[14]==GeoMagSensorList[i].DALDeviceID.DeviceID[14])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[15]==GeoMagSensorList[i].DALDeviceID.DeviceID[15])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[16]==GeoMagSensorList[i].DALDeviceID.DeviceID[16])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[17]==GeoMagSensorList[i].DALDeviceID.DeviceID[17])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[18]==GeoMagSensorList[i].DALDeviceID.DeviceID[18])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[19]==GeoMagSensorList[i].DALDeviceID.DeviceID[19])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[20]==GeoMagSensorList[i].DALDeviceID.DeviceID[20])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[21]==GeoMagSensorList[i].DALDeviceID.DeviceID[21])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[22]==GeoMagSensorList[i].DALDeviceID.DeviceID[22])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[23]==GeoMagSensorList[i].DALDeviceID.DeviceID[23])){
							break;
						}
					}
				}
				iEntry=-1;
				if(i==MAXGEOMAGSENSORNUM){//未找到记录，新建一个
					for(i=0;i<MAXGEOMAGSENSORNUM;i++){
						if(GeoMagSensorList[i].isValid==0){
							iEntry=i;
						}
					}
				}else{//找到记录，更新
					iEntry=i;
					GeoMagSensorList[i].isCorrCmdFlying=0;
				}
				if(iEntry==-1){
					S_DEBUGLOG2(3,"Warning, no entry to hold the new device %02X%02X.\n",pEvent->un.Event_DALEvent.DeviceID.DeviceID[9],pEvent->un.Event_DALEvent.DeviceID.DeviceID[8]);
				}else{
					GeoMagSensorList[iEntry].isValid=1;
					GeoMagSensorList[iEntry].DALDeviceID=pEvent->un.Event_DALEvent.DeviceID;
					GeoMagSensorList[iEntry].isParked=pEvent->un.Event_DALEvent.ParamList[0].Data[0];
					GeoMagSensorList[iEntry].Voltage=pEvent->un.Event_DALEvent.ParamList[1].Data[0];
					GeoMagSensorList[iEntry].Temp=pEvent->un.Event_DALEvent.ParamList[2].Data[0];
					GeoMagSensorList[iEntry].WMODE=pEvent->un.Event_DALEvent.ParamList[3].Data[0];
					if(GeoMagSensorList[iEntry].WMODE==1){
						memcpy(&GeoMagSensorList[iEntry].mx,pEvent->un.Event_DALEvent.ParamList[4].Data,2);
						memcpy(&GeoMagSensorList[iEntry].my,pEvent->un.Event_DALEvent.ParamList[5].Data,2);
						memcpy(&GeoMagSensorList[iEntry].mz,pEvent->un.Event_DALEvent.ParamList[6].Data,2);
						memcpy(&GeoMagSensorList[iEntry].bx,pEvent->un.Event_DALEvent.ParamList[7].Data,4);
						memcpy(&GeoMagSensorList[iEntry].by,pEvent->un.Event_DALEvent.ParamList[8].Data,4);
						memcpy(&GeoMagSensorList[iEntry].bz,pEvent->un.Event_DALEvent.ParamList[9].Data,4);
						memcpy(&GeoMagSensorList[iEntry].TS,pEvent->un.Event_DALEvent.ParamList[10].Data,2);
						memcpy(&GeoMagSensorList[iEntry].TDMA,pEvent->un.Event_DALEvent.ParamList[11].Data,2);
						memcpy(&GeoMagSensorList[iEntry].TDME,pEvent->un.Event_DALEvent.ParamList[12].Data,2);
						memcpy(&GeoMagSensorList[iEntry].TBME,pEvent->un.Event_DALEvent.ParamList[13].Data,2);
						memcpy(&GeoMagSensorList[iEntry].TPZ,pEvent->un.Event_DALEvent.ParamList[14].Data,2);
						memcpy(&GeoMagSensorList[iEntry].TP3,pEvent->un.Event_DALEvent.ParamList[15].Data,2);
						GeoMagSensorList[iEntry].RHMN=pEvent->un.Event_DALEvent.ParamList[16].Data[0];
						GeoMagSensorList[iEntry].PMF=pEvent->un.Event_DALEvent.ParamList[17].Data[0];
						GeoMagSensorList[iEntry].PMN=pEvent->un.Event_DALEvent.ParamList[18].Data[0];
						GeoMagSensorList[iEntry].PRPT=pEvent->un.Event_DALEvent.ParamList[19].Data[0];
						memcpy(&GeoMagSensorList[iEntry].TPVS,pEvent->un.Event_DALEvent.ParamList[20].Data,2);
						memcpy(&GeoMagSensorList[iEntry].TPVL,pEvent->un.Event_DALEvent.ParamList[21].Data,2);
					}
					GeoMagSensorList[iEntry].ActiveCount++;
				}
				break;
			default:
				DEBUGLOG1(0,"Undefined event ID %d for DAL_VEHICLEPRESENCESENSORGEOMAG.\n",pEvent->EventType);
				break;
			}
			break;
		case DAL_ITS_ROUTER_REPEATER:
			switch(pEvent->un.Event_DALEvent.DALEventID){
			case DAL_ITS_ROUTER_REPEATER_STATEREPORT:
				//是否有记录？
				for(i=0;i<MAXROUTERNUM;i++){
					if(RouterList[i].isValid){
						if( (pEvent->un.Event_DALEvent.DeviceID.DeviceID[0] ==RouterList[i].DALDeviceID.DeviceID[0] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[1] ==RouterList[i].DALDeviceID.DeviceID[1] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[2] ==RouterList[i].DALDeviceID.DeviceID[2] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[3] ==RouterList[i].DALDeviceID.DeviceID[3] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[4] ==RouterList[i].DALDeviceID.DeviceID[4] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[5] ==RouterList[i].DALDeviceID.DeviceID[5] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[6] ==RouterList[i].DALDeviceID.DeviceID[6] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[7] ==RouterList[i].DALDeviceID.DeviceID[7] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[8] ==RouterList[i].DALDeviceID.DeviceID[8] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[9] ==RouterList[i].DALDeviceID.DeviceID[9] )&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[10]==RouterList[i].DALDeviceID.DeviceID[10])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[11]==RouterList[i].DALDeviceID.DeviceID[11])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[12]==RouterList[i].DALDeviceID.DeviceID[12])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[13]==RouterList[i].DALDeviceID.DeviceID[13])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[14]==RouterList[i].DALDeviceID.DeviceID[14])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[15]==RouterList[i].DALDeviceID.DeviceID[15])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[16]==RouterList[i].DALDeviceID.DeviceID[16])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[17]==RouterList[i].DALDeviceID.DeviceID[17])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[18]==RouterList[i].DALDeviceID.DeviceID[18])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[19]==RouterList[i].DALDeviceID.DeviceID[19])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[20]==RouterList[i].DALDeviceID.DeviceID[20])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[21]==RouterList[i].DALDeviceID.DeviceID[21])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[22]==RouterList[i].DALDeviceID.DeviceID[22])&&\
							(pEvent->un.Event_DALEvent.DeviceID.DeviceID[23]==RouterList[i].DALDeviceID.DeviceID[23])){
							break;
						}
					}
				}
				iEntry=-1;
				if(i==MAXROUTERNUM){//未找到记录，新建一个
					for(i=0;i<MAXROUTERNUM;i++){
						if(RouterList[i].isValid==0){
							iEntry=i;
						}
					}
				}else{//找到记录，更新
					iEntry=i;
				}
				if(iEntry==-1){
					S_DEBUGLOG2(3,"Warning, no entry to hold the new device %02X%02X.\n",pEvent->un.Event_DALEvent.DeviceID.DeviceID[9],pEvent->un.Event_DALEvent.DeviceID.DeviceID[8]);
				}else{
					RouterList[iEntry].isValid=1;
					RouterList[iEntry].DALDeviceID=pEvent->un.Event_DALEvent.DeviceID;
					RouterList[iEntry].Voltage=pEvent->un.Event_DALEvent.ParamList[0].Data[0];
					RouterList[iEntry].ActiveCount++;
				}
				break;
			default:
				DEBUGLOG1(0,"Undefined event ID %d for DAL_ITS_ROUTER_REPEATER.\n",pEvent->EventType);
				break;
			}
			break;
		default:
			DEBUGLOG0(0," DeviceID=");
			for(i=0;i<24;i++){
				S_DEBUGLOG1(0,"%02X",pEvent->un.Event_DALEvent.DeviceID.DeviceID[i]);
			}
			S_DEBUGLOG1(0," EventID=%d.\n",pEvent->un.Event_DALEvent.DALEventID);
			break;
		}
		break;
	default:
		DEBUGLOG1(0,"DALEventCB for event %d, ignored.\n",pEvent->EventType);
		break;
	}
}

//ZNP COM设备路径
//#define ZNPCOMDEVPATH "/dev/ttySAC1"	//for arm linux
//#define ZNPCOMDEVPATH "/dev/ttyAMA0" //for Rpi
#ifdef OK335
#define ZNPCOMDEVPATH "/dev/ttyO2" //for ok335
#else
#define ZNPCOMDEVPATH "/dev/ttyS1"	//for ubuntu
#endif

int main(int argc,char *argv[])
{
	int i,j,TestCnt=0;
	DALStatus rt;
	stDALCfg DALCfg;
	int DeviceNum;
	int doTest1=0,doTest2=0,doTest3=0;
	unsigned char TempU8;
	unsigned short TempU16;

	memset(&GeoMagSensorList,0,sizeof(stGeoMagSensorNode)*MAXGEOMAGSENSORNUM);
	memset(&RouterList,0,sizeof(stRouterNode)*MAXROUTERNUM);

	memset(&DALCfg,0,sizeof(stDALCfg));
	DALCfg.pZNPComPath=ZNPCOMDEVPATH;
	DALCfg.pEventCB=DALEventCB;

	if(argc==2){//00 12 4B 00 01 68 86 5E
		S_DEBUGLOG0(0,"Using preset zigbee params.\n");
		DALCfg.isValid_IEEEAddr=1;
		DALCfg.IEEEAddr[0]=0x00;
		DALCfg.IEEEAddr[1]=0x86;
		DALCfg.IEEEAddr[2]=0x68;
		DALCfg.IEEEAddr[3]=0x01;
		DALCfg.IEEEAddr[4]=0x00;
		DALCfg.IEEEAddr[5]=0x4B;
		DALCfg.IEEEAddr[6]=0x12;
		DALCfg.IEEEAddr[7]=0x00;

		DALCfg.isValid_PANID=1;
		DALCfg.PANID=0x0F00;

		DALCfg.isValid_ExtPANID=1;
		DALCfg.ExtPANID[0]=0x00;
		DALCfg.ExtPANID[1]=0x86;
		DALCfg.ExtPANID[2]=0x68;
		DALCfg.ExtPANID[3]=0x01;
		DALCfg.ExtPANID[4]=0x00;
		DALCfg.ExtPANID[5]=0x4B;
		DALCfg.ExtPANID[6]=0x12;
		DALCfg.ExtPANID[7]=0x00;
	}else if(argc==3){//所有传感器执行校正
		doTest1=1;
	}else if(argc==4){//设置传感器参数
		doTest3=1;
	}

	rt=DAL_Init(&DALCfg);
	if(rt!=DALS_SUCCESS){
		S_DEBUGLOG1(0,"DAL_Init failed %d.\n",rt);
	}else{
		S_DEBUGLOG0(0,"DAL_Init ok.\n");
	}

//	//手动添加设备
//	{
//		stDeviceDesc DeviceDesc;
//		DALStatus Status;
//		int i,Num=0;
//		unsigned char IEEEAddr[5][8]={	{0xc0,0xf4,0x41,0x03,0x00,0x4b,0x12,0x00},
//										{0xb0,0xf4,0x41,0x03,0x00,0x4b,0x12,0x00}
//									 };
//		for(i=0;i<Num;i++){
//			DeviceDesc.DevType=ZIGBEEE;
//			memcpy(DeviceDesc.un.DeviceDesc_ZigBee.IEEEAddr,IEEEAddr[i],8);
//			Status=DAL_AddDevice(&DeviceDesc);
//			if(Status!=DALS_SUCCESS){
//				printf("DAL_AddDevice failed %d.\n",Status);
//			}
//		}
//	}

	//DAL_GetDALDetails获取DAL详细参数测试
	{
		DALStatus rt;
		stDALDetails *pDALDetails=NULL;

		pDALDetails=AllocateDALDetails();
		if(pDALDetails==NULL){
			S_DEBUGLOG0(3,"AllocateDALDetails failed.\n");
		}else{
			rt=DAL_GetDALDetails(pDALDetails);
			if(rt!=DALS_SUCCESS){
				S_DEBUGLOG1(3,"DAL_GetDALDetails failed %d.\n",rt);
			}else{
				S_DEBUGLOG1(0,"DALVersion: %d\n",pDALDetails->DALVersion);
				S_DEBUGLOG1(0,"ZigBeeMVersion: %d\n",pDALDetails->DALSubMDetails_ZigBee.ZigBeeMVersion);
				S_DEBUGLOG1(0,"PANID: %04X\n",pDALDetails->DALSubMDetails_ZigBee.PANID);
				S_DEBUGLOG0(0,"ExtPANID: ");
				for(i=0;i<8;i++){
					S_DEBUGLOG1(0,"%02X",pDALDetails->DALSubMDetails_ZigBee.ExtPANID[i]);
				}
				S_DEBUGLOG0(0,"\n");
				S_DEBUGLOG0(0,"LocalIEEEAddr: ");
				for(i=0;i<8;i++){
					S_DEBUGLOG1(0,"%02X",pDALDetails->DALSubMDetails_ZigBee.LocalIEEEAddr[i]);
				}
				S_DEBUGLOG0(0,"\n");
			}
		}
		DelocateDALDetails(pDALDetails);
	}

	sleep(5);

	while(1){
		sleep(1);

		//打印所有设备记录
		//GEOMAG Sensor list
		S_DEBUGLOG0(0,"------------GEOMAG Sensor List-------------\n");
		for(i=0;i<MAXGEOMAGSENSORNUM;i++){
			if(GeoMagSensorList[i].isValid){
				S_DEBUGLOG2(0,"%02X%02X:\t",GeoMagSensorList[i].DALDeviceID.DeviceID[9],GeoMagSensorList[i].DALDeviceID.DeviceID[8]);
				//检测结果
				S_DEBUGLOG1(0,"%d\t| ",GeoMagSensorList[i].isParked);
				//电池电压
				S_DEBUGLOG2(0,"%d.%dV\t",GeoMagSensorList[i].Voltage/10,GeoMagSensorList[i].Voltage%10);
				//温度
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].Temp);
				//WMODE
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].WMODE);
				//当前测量值
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].mx);
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].my);
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].mz);
				//背景值
				S_DEBUGLOG1(0,"%f\t",GeoMagSensorList[i].bx);
				S_DEBUGLOG1(0,"%f\t",GeoMagSensorList[i].by);
				S_DEBUGLOG1(0,"%f\t",GeoMagSensorList[i].bz);
				//TS
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TS);
				//TDMA
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TDMA);
				//TDME
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TDME);
				//TBME
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TBME);
				//TPZ
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TPZ);
				//TP3
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TP3);
				//RHMN
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].RHMN);
				//PMF
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].PMF);
				//PMN
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].PMN);
				//PRPT
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].PRPT);
				//TPVS
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TPVS);
				//TPVL
				S_DEBUGLOG1(0,"%d\t",GeoMagSensorList[i].TPVL);
				//活跃计数
				S_DEBUGLOG1(0,"%d\n",GeoMagSensorList[i].ActiveCount);
			}
		}
		//Router List
		S_DEBUGLOG0(0,"----------------Router List----------------\n");
		for(i=0;i<MAXROUTERNUM;i++){
			if(RouterList[i].isValid){
				S_DEBUGLOG2(0,"%02X%02X:\t",RouterList[i].DALDeviceID.DeviceID[9],RouterList[i].DALDeviceID.DeviceID[8]);
				//电池电压
				S_DEBUGLOG2(0,"%d.%dV\t",RouterList[i].Voltage/10,RouterList[i].Voltage%10);
				//活跃计数
				S_DEBUGLOG1(0,"%d\n",RouterList[i].ActiveCount);
			}
		}
		fPrintTime();

		//传感器校正测试
		//测试DAL_VEHICLEPRESENCESENSORGEOMAG:DAL_VEHICLEPRESENCESENSORGEOMAG_ADJUST
		if(doTest1){
			DALStatus Status;
			stDALCtrl DALCtrl;

			for(i=0;i<MAXGEOMAGSENSORNUM;i++){
				if(GeoMagSensorList[i].isValid){//存在
					if((GeoMagSensorList[i].bx==0)&&(GeoMagSensorList[i].by==0)&&(GeoMagSensorList[i].bz==0)){//还未校正
						if(GeoMagSensorList[i].isCorrCmdFlying==0){
							printf("Found a uncorred sensor: \n");
							for(j=0;j<DEVICEIDLEN;j++){
								printf("%02X",GeoMagSensorList[i].DALDeviceID.DeviceID[j]);
							}
							printf("\n");
							memset(&DALCtrl,0,sizeof(stDALCtrl));
							DALCtrl.CtrlID=0x00;
							DALCtrl.ParamNum=0;
							printf("Invoking DAL_DeviceCtrl...\n");
							Status=DAL_DeviceCtrl(&GeoMagSensorList[i].DALDeviceID, &DALCtrl);
							if(Status!=DALS_SUCCESS){
								printf("Error, DAL_DeviceCtrl failed %d.\n",Status);
							}else{
								printf("Inited ok.\n");
							}
							GeoMagSensorList[i].isCorrCmdFlying=1;
						}
					}
				}
			}
		}

		//测试删除设备
		if(doTest2){
			DALStatus Status;
			stDeviceIDList *pDeviceIDList=NULL;

			doTest2=0;

			pDeviceIDList=AllocateDeviceIDList();
			if(pDeviceIDList!=NULL){
				//获取设备ID列表
				Status=DAL_GetDeviceIDList(pDeviceIDList);
				if(Status!=DALS_SUCCESS){
					printf("Error, DAL_GetDeviceIDList failed %d.\n",Status);
				}else{
					for(i=0;i<pDeviceIDList->DeviceNum;i++){
						Status=DAL_RemoveDevice(&pDeviceIDList->pDALDeviceIDList[i]);
						if(Status!=DALS_SUCCESS){
							printf("Error, DAL_RemoveDevice failed %d.\n",Status);
						}
					}
				}
				DelocateDeviceIDList(pDeviceIDList);
			}
		}

		//传感器参数设置
		if(doTest3){
			DALStatus Status;
			stDeviceIDList *pDeviceIDList=NULL;
			stDALCtrl DALCtrl;
			unsigned int DevType;
			int WhiteIndexList[10]={1,1,0,0,0,0,0,0,0,0};

			doTest3=0;
			pDeviceIDList=AllocateDeviceIDList();
			if(pDeviceIDList!=NULL){
				//获取设备ID列表
				Status=DAL_GetDeviceIDList(pDeviceIDList);
				if(Status!=DALS_SUCCESS){
					printf("Error, DAL_GetDeviceIDList failed %d.\n",Status);
				}else{
					for(i=0;i<pDeviceIDList->DeviceNum;i++){
						printf("(%d)\t",i);
						for(j=0;j<DEVICEIDLEN;j++){
							printf("%02X",pDeviceIDList->pDALDeviceIDList[i].DeviceID[j]);
						}
						printf("\n");
					}

					for(i=0;i<pDeviceIDList->DeviceNum;i++){
						if(WhiteIndexList[i]==0){
							continue;
						}
						memcpy(&DevType,pDeviceIDList->pDALDeviceIDList[i].DeviceID,4);
						if(DevType==DAL_VEHICLEPRESENCESENSORGEOMAG){
							memset(&DALCtrl,0,sizeof(stDALCtrl));
							DALCtrl.CtrlID=0x01;
							DALCtrl.ParamNum=13;
							//WMODE 参数
							TempU8=1;
							DALCtrl.ParamList[0].DataType=DAL_UINT8;
							memcpy(DALCtrl.ParamList[0].Data,&TempU8,1);
							//TS参数
							TempU16=50*50;
							DALCtrl.ParamList[1].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[1].Data,&TempU16,2);
							//TDMA参数
							TempU16=50*50;
							DALCtrl.ParamList[2].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[2].Data,&TempU16,2);
							//TDME参数
							TempU16=50*50;
							DALCtrl.ParamList[3].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[3].Data,&TempU16,2);
							//TBME参数
							TempU16=50*50;
							DALCtrl.ParamList[4].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[4].Data,&TempU16,2);
							//TPZ参数
							TempU16=40*40;
							DALCtrl.ParamList[5].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[5].Data,&TempU16,2);
							//TP3参数
							TempU16=100*100;
							DALCtrl.ParamList[6].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[6].Data,&TempU16,2);
							//RHMN参数
							TempU8=5;
							DALCtrl.ParamList[7].DataType=DAL_UINT8;
							memcpy(DALCtrl.ParamList[7].Data,&TempU8,1);
							//PMF参数
							TempU8=1;
							DALCtrl.ParamList[8].DataType=DAL_UINT8;
							memcpy(DALCtrl.ParamList[8].Data,&TempU8,1);
							//PMN参数
							TempU8=1;
							DALCtrl.ParamList[9].DataType=DAL_UINT8;
							memcpy(DALCtrl.ParamList[9].Data,&TempU8,1);
							//PRPT参数
							TempU8=1;
							DALCtrl.ParamList[10].DataType=DAL_UINT8;
							memcpy(DALCtrl.ParamList[10].Data,&TempU8,1);
							//TPVS参数
							TempU16=1;
							DALCtrl.ParamList[11].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[11].Data,&TempU8,1);
							//TPVL参数
							TempU16=1;
							DALCtrl.ParamList[12].DataType=DAL_UINT16;
							memcpy(DALCtrl.ParamList[12].Data,&TempU8,1);

							printf("Invoking DAL_DeviceCtrl...\n");
							Status=DAL_DeviceCtrl(pDeviceIDList->pDALDeviceIDList+i, &DALCtrl);
							if(Status!=DALS_SUCCESS){
								printf("Error, DAL_DeviceCtrl failed %d.\n",Status);
							}
						}
					}
				}
				DelocateDeviceIDList(pDeviceIDList);
			}
		}

	}
}













