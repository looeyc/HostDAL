/*
 * devicedb_sqlite3.c
 *
 *  Created on: 2014-7-13
 *      Author: chengm
 */

//用sqlite3实现设备数据库

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <pthread.h>
#include "sqlite3.h"
#include "devicedb.h"
#include "devicedb_zigbee.h"
#include "devicedb_webcam.h"
#include "devicedb_transfmt.h"
#include "debuglog.h"

//全局变量
sqlite3 *pGlobal_DevDB_DalDB=NULL;

//------------------------------------------------------------------------------------------------------
//销毁一个WebCam设备节点
void fDestroyDevNode_WebCam(stDevNode_WebCam *pDevNode_WebCam)
{
}

//销毁一个ReptExt节点
void fDestroyDevNode_ReptExt(stDevNode_ReptExt *pDevNode_ReptExt)
{
}

//销毁一个设备节点，释放其所占用的存储资源
//pDeviceNode - 指向设备节点
void fDevDB_DestroyDeviceNode(stDevDB_DeviceNode *pDeviceNode)
{
	if(pDeviceNode==NULL){
		return;
	}

	switch(pDeviceNode->DeviceType){
	case(DEVTYPE_ZIGBEE):
		fDestroyDevNode_ZigBee(&pDeviceNode->pDeviceNode->DevNode_ZigBee);
		break;
	default:
		DEBUGLOG1(3,"Error, unknown devoice type %d.\n",pDeviceNode->DeviceType);
		break;
	}

	free(pDeviceNode);
}

//创建一个设备节点
//DeviceType 需要创建的设备类型
//return 返回创建的设备节点，NULL-失败
//使用完毕后必须调用fDevDB_DestroyDeviceNode销毁
//所创建的设备节点不含任何实际有效的数据
stDevDB_DeviceNode *fDevDB_CreatDeviceNode(enDeviceType DeviceType)
{
	stDevDB_DeviceNode *pRet=NULL;
	unDevNode *pUnDevNode=NULL;

	switch(DeviceType){
	case DEVTYPE_ZIGBEE:
		pUnDevNode=(unDevNode *)fCreateDevNode_ZigBee();
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType 0x%08X.\n",DeviceType);
		break;
	}

	if(pUnDevNode!=NULL){
		pRet=(stDevDB_DeviceNode *)malloc(sizeof(stDevDB_DeviceNode));
		if(pRet==NULL){
			DEBUGLOG0(3,"Error, malloc failed.\n");
			//释放掉之前创建好的
			switch(DeviceType){
			case DEVTYPE_ZIGBEE:
				fDestroyDevNode_ZigBee((stDevNode_ZigBee *)pUnDevNode);
				break;
			default:
				DEBUGLOG1(3,"Error, undefined DeviceType 0x%08X.\n",DeviceType);
				break;
			}
		}else{
			memset(pRet,0,sizeof(stDevDB_DeviceNode));
			//必要的初始化
			pRet->DeviceKeyWords.DeviceType=DeviceType;
			pRet->DeviceType=DeviceType;
			pRet->pDeviceNode=pUnDevNode;
		}
	}
	return pRet;
}

//检查指定的设备节点是否已经存在于数据库
//pDeviceKeyWords - 指定的设备关键字
//return 0-不存在，1-存在，else-出错
static int fSql_IsDeviceExistInDB(stDevDB_DeviceKeyWords *pDeviceKeyWords)
{
	int rt,ErrNo=0,isExist=0;
	char pSql[1024];
	sqlite3_stmt *pStmt=NULL;
	stDevKeyInfo_ZigBee DevKeyInfo_ZigBee;

	if(pDeviceKeyWords==NULL){
		return -1;
	}

	switch(pDeviceKeyWords->DeviceType){
	case DEVTYPE_ZIGBEE:
		rt=fParseKeyInfo_ZigBee(&DevKeyInfo_ZigBee, pDeviceKeyWords->DeviceKeyInfo);
		if(rt!=0){
			DEBUGLOG1(3,"Error, fParseKeyInfo_ZigBee failed %d.\n",rt);
			ErrNo=-2;
		}else{
			snprintf(pSql,1024,"SELECT COUNT(*) FROM DeviceTable_ZigBee WHERE IEEEAddr=x'%02X%02X%02X%02X%02X%02X%02X%02X' AND EP=%d;",\
				DevKeyInfo_ZigBee.IEEEAddr[0],\
				DevKeyInfo_ZigBee.IEEEAddr[1],\
				DevKeyInfo_ZigBee.IEEEAddr[2],\
				DevKeyInfo_ZigBee.IEEEAddr[3],\
				DevKeyInfo_ZigBee.IEEEAddr[4],\
				DevKeyInfo_ZigBee.IEEEAddr[5],\
				DevKeyInfo_ZigBee.IEEEAddr[6],\
				DevKeyInfo_ZigBee.IEEEAddr[7],\
				DevKeyInfo_ZigBee.EP
			);
		}
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType %d.\n",pDeviceKeyWords->DeviceType);
		ErrNo=-3;
		break;
	}

	if(ErrNo==0){
		rt=sqlite3_prepare_v2(pGlobal_DevDB_DalDB, pSql, strlen(pSql), &pStmt, NULL);
		if(rt!=SQLITE_OK){
			DEBUGLOG1(3,"Error, sqlite3_prepare_v2 failed %d.\n",rt);
			ErrNo=-4;
		}else{
			rt=sqlite3_step(pStmt);
			if(rt!=SQLITE_ROW){
				DEBUGLOG1(3,"Error, sqlite3_step failed %d.\n",rt);
				ErrNo=-5;
			}else{
				isExist=(sqlite3_column_int(pStmt,0)==0)?0:1;
			}
		}
	}

	if(pStmt!=NULL){
		sqlite3_finalize(pStmt);
	}

	if(ErrNo==0){
		return isExist;
	}else{
		return ErrNo;
	}
}

//如果指定的设备节点存在，就替换，如果不存在就新建
//pDeviceNode - 指定的设备节点
//return 0-成功，else-失败
static int fSql_ReplaceDevice(stDevDB_DeviceNode *pDeviceNode)
{
	int i,rt,ErrNo=0;;
	char pSql[2048];
	char pInClusterList[128]={0};
	char pOutClusterList[128]={0};
	char pRelayList[256]={0};
	char *pWrite;
	sqlite3_stmt *pStmt=NULL;

	if(pDeviceNode==NULL){
		return -1;
	}

	switch(pDeviceNode->DeviceType){
	case DEVTYPE_ZIGBEE:
		//Input Cluster List
		pWrite=pInClusterList;
		for(i=0;i<pDeviceNode->pDeviceNode->DevNode_ZigBee.NumInClusters;i++){
			snprintf(pWrite,128-(pWrite-pInClusterList),"%02X%02X",(pDeviceNode->pDeviceNode->DevNode_ZigBee.InClusterList[i].ClusterID)&0x00FF,((pDeviceNode->pDeviceNode->DevNode_ZigBee.InClusterList[i].ClusterID)>>8)&0x00FF);
			pWrite+=4;
		}
		//Output Cluster List
		pWrite=pOutClusterList;
		for(i=0;i<pDeviceNode->pDeviceNode->DevNode_ZigBee.NumOutClusters;i++){
			snprintf(pWrite,128-(pWrite-pOutClusterList),"%02X%02X",(pDeviceNode->pDeviceNode->DevNode_ZigBee.OutClusterList[i].ClusterID)&0x00FF,(pDeviceNode->pDeviceNode->DevNode_ZigBee.OutClusterList[i].ClusterID>>8)&0x00FF);
			pWrite+=4;
		}
		//Relay List
		pWrite=pRelayList;
		for(i=0;i<pDeviceNode->pDeviceNode->DevNode_ZigBee.Route.RelayCount;i++){
			snprintf(pWrite,256-(pWrite-pRelayList),"%02X%02X",pDeviceNode->pDeviceNode->DevNode_ZigBee.Route.RelayList[i*2],pDeviceNode->pDeviceNode->DevNode_ZigBee.Route.RelayList[i*2+1]);
			pWrite+=4;
		}
		snprintf(pSql,2048,"REPLACE INTO DeviceTable_ZigBee (DeviceName,UserDescript,IEEEAddr,UserDescriptor,NwkAddr,ProfileID,DeviceID,EP,NumInClusters,InClusterList,NumOutClusters,OutClusterList,isRouteAvailable,RelayCount,RelayList) \
			VALUES('%s','%s',x'%02X%02X%02X%02X%02X%02X%02X%02X','%s',%d,%d,%d,%d,%d,x'%s',%d,x'%s',%d,%d,x'%s');",\
			pDeviceNode->DeviceName,
			pDeviceNode->UserDescript,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[0],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[1],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[2],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[3],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[4],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[5],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[6],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr[7],
			pDeviceNode->pDeviceNode->DevNode_ZigBee.UserDescriptor,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.DeviceID,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.EP,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.NumInClusters,
			pInClusterList,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.NumOutClusters,
			pOutClusterList,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable,
			pDeviceNode->pDeviceNode->DevNode_ZigBee.Route.RelayCount,
			pRelayList);
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType %d.\n",pDeviceNode->DeviceType);
		ErrNo=-2;
		break;
	}

	if(ErrNo==0){
		rt=sqlite3_prepare_v2(pGlobal_DevDB_DalDB, pSql, strlen(pSql), &pStmt, NULL);
		if(rt!=SQLITE_OK){
			DEBUGLOG1(3,"Error, sqlite3_prepare_v2 failed %d.\n",rt);
			ErrNo=-3;
		}else{
			rt=sqlite3_step(pStmt);
			if(rt!=SQLITE_DONE){
				DEBUGLOG1(3,"Error, sqlite3_step failed %d.\n",rt);
				ErrNo=-4;
			}
		}
	}

	if(pStmt!=NULL){
		sqlite3_finalize(pStmt);
	}

	return ErrNo;
}

//将通过SELECT语句搜索到的Sql数据表行转换为stDevDB_DeviceNode结构体
//pStmt - 输入，sqlite3_prepare_v2得到的句柄
//pDevDB_DeviceNode - 输入/输出，调用者提供用于返回转换结果的空间
//return 0-成功，else-失败
int fSql_SqlTableRowToDeviceNode(sqlite3_stmt *pStmt, stDevDB_DeviceNode *pDevDB_DeviceNode)
{
	int i;
	const char *pTableName=NULL;
	char *pTempString=NULL;
	enDeviceType DeviceType;
	const void *pBlob=NULL;

	if((pStmt==NULL)||(pDevDB_DeviceNode==NULL)){
		return -1;
	}

	pTableName=sqlite3_column_table_name(pStmt,0);
	if(!strcmp("DeviceTable_ZigBee",pTableName)){
		DeviceType=DEVTYPE_ZIGBEE;
	}else{
		DEBUGLOG1(3,"Error, can't determin DeviceType with the TableName '%s'.\n",pTableName);
		return -2;
	}

	switch(DeviceType){
	case DEVTYPE_ZIGBEE:
		pDevDB_DeviceNode->DeviceType=DeviceType;
		//0 IEEEAddr
		pBlob=sqlite3_column_blob(pStmt,0);
		if(pBlob!=NULL){
			memcpy(pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.IEEEAddr,pBlob,8);
		}else{
			return -4;
		}
		//1 EP
		pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.EP=sqlite3_column_int(pStmt,1);
		//2 DeviceName
		pTempString=(char *)sqlite3_column_text(pStmt,2);
		if(pTempString!=NULL){
			strncpy(pDevDB_DeviceNode->DeviceName,pTempString,MAXDEVICENAME);
		}
		//3 UserDescript
		pTempString=(char *)sqlite3_column_text(pStmt,3);
		if(pTempString!=NULL){
			strncpy(pDevDB_DeviceNode->UserDescript,pTempString,MAXUSRDESC);
		}
		//4 UserDescriptor
		pTempString=(char *)sqlite3_column_text(pStmt,4);
		if(pTempString!=NULL){
			strncpy(pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.UserDescriptor,pTempString,MAXUSERDESCRIPTORLEN);
		}
		//5 NwkAddr
		pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.NwkAddr=sqlite3_column_int(pStmt,5);
		//6 ProfileID
		pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.ProfileID=sqlite3_column_int(pStmt,6);
		//7 DeviceID
		pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.DeviceID=sqlite3_column_int(pStmt,7);
		//8 NumInClusters
		pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.NumInClusters=sqlite3_column_int(pStmt,8);
		//9 InClusterList
		pBlob=sqlite3_column_blob(pStmt,9);
		if(pBlob!=NULL){
			for(i=0;i<pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.NumInClusters;i++){
				memcpy(&pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.InClusterList[i].ClusterID,((char *)pBlob)+i*2,2);
			}
		}
		//10 NumOutClusters
		pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.NumOutClusters=sqlite3_column_int(pStmt,10);
		//11 OutClusterList
		pBlob=sqlite3_column_blob(pStmt,11);
		if(pBlob!=NULL){
			for(i=0;i<pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.NumOutClusters;i++){
				memcpy(&pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.OutClusterList[i].ClusterID,((char *)pBlob)+i*2,2);
			}
		}
		//12 isRouteAvailable
		pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable=sqlite3_column_int(pStmt,12);
		if(pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.isRouteAvailable){
			//13 RelayCount
			pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.Route.RelayCount=sqlite3_column_int(pStmt,13);
			//14 RelayList
			pBlob=sqlite3_column_blob(pStmt,14);
			if(pBlob!=NULL){
				memcpy(pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.Route.RelayList,pBlob,pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee.Route.RelayCount*2);
			}
		}
		//填写DeviceKeyWords
		pDevDB_DeviceNode->DeviceKeyWords.DeviceType=DEVTYPE_ZIGBEE;
		fGenKeyInfo_ZigBee2(&pDevDB_DeviceNode->pDeviceNode->DevNode_ZigBee, pDevDB_DeviceNode->DeviceKeyWords.DeviceKeyInfo);
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType %d.\n",DeviceType);
		return -3;
		break;
	}
	return 0;
}
//------------------------------------------------------------------------------------------------------

//初始化
//return 0-成功，else-失败
int fDevDB_Init()
{
	int rt;
	char pSql[1024];
	char *pErrMsg=NULL;

	//打开数据库
	rt=sqlite3_open( "./dal.db",  &pGlobal_DevDB_DalDB );
	if(rt!=SQLITE_OK){
		DEBUGLOG2(3,"Error, sqlite3_open failed %d: %s.",rt,sqlite3_errmsg(pGlobal_DevDB_DalDB));
		sqlite3_close(pGlobal_DevDB_DalDB);
		return 1;
	}
	sqlite3_busy_timeout(pGlobal_DevDB_DalDB,1000);

	//创建表
	//ZigBee设备列表
	snprintf(pSql,1024,"CREATE TABLE IF NOT EXISTS DeviceTable_ZigBee (\
		IEEEAddr			BLOB NOT NULL, \
		EP 					INTEGER NOT NULL, \
		DeviceName 			TEXT, \
		UserDescript 		TEXT, \
		UserDescriptor 		TEXT, \
		NwkAddr 			INTEGER, \
		ProfileID 			INTEGER, \
		DeviceID 			INTEGER, \
		NumInClusters 		INTEGER, \
		InClusterList 		BLOB, \
		NumOutClusters 		INTEGER, \
		OutClusterList 		BLOB, \
		isRouteAvailable 	INTEGER, \
		RelayCount 			INTEGER, \
		RelayList 			BLOB, \
		PRIMARY KEY (IEEEAddr,EP));\
		");
	rt=sqlite3_exec(pGlobal_DevDB_DalDB , pSql , 0 , 0 , &pErrMsg);
	if(rt!=SQLITE_OK){
		DEBUGLOG2(3,"Error, CREATE TABLE DeviceTable_ZigBee failed %d: %s.\n",rt,pErrMsg);
		sqlite3_close(pGlobal_DevDB_DalDB);
		return 2;
	}

	return 0;
}

//加入设备
//pDeviceNode待加入的设备节点数据
//return 0-成功，加入了新的节点、1-成功，但是其实设备节点已经存在; else-失败
//如果设备节点已经存在，则替换
int fDevDB_AddDevice(stDevDB_DeviceNode *pDeviceNode)
{
	int rt,isAlreadyExist=0;

	if(pDeviceNode==NULL){
		return -1;
	}

	//设备节点是否已经存在
	isAlreadyExist=fSql_IsDeviceExistInDB(&pDeviceNode->DeviceKeyWords);

	//无论是否已经存在，都使用sqlite的替换方法
	rt=fSql_ReplaceDevice(pDeviceNode);
	if(rt!=0){
		DEBUGLOG1(3,"Error, fSql_ReplaceDevice failed %d.\n",rt);
		return -2;
	}

	return isAlreadyExist;
}

//删除设备
//pDevDB_DeviceKeyWords - 设备标识
//return 0-成功，1-指定设备不存在，else-失败
int fDevDB_RemoveDevice(stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords)
{
	int rt,ErrNo=0;
	char IEEEAddr[18];
	char pSql[1024];
	sqlite3_stmt *pStmt=NULL;
	stDevKeyInfo_ZigBee DevKeyInfo_ZigBee;

	if(pDevDB_DeviceKeyWords==NULL){
		return -1;
	}

	switch(pDevDB_DeviceKeyWords->DeviceType){
	case DEVTYPE_ZIGBEE:
		//指定设备是否存在
		rt=fSql_IsDeviceExistInDB(pDevDB_DeviceKeyWords);
		if(rt==0){//不存在
			return 1;
		}else if(rt==1){//存在
			rt=fParseKeyInfo_ZigBee(&DevKeyInfo_ZigBee, pDevDB_DeviceKeyWords->DeviceKeyInfo);
			if(rt!=0){
				DEBUGLOG1(3,"Error, fParseKeyInfo_ZigBee failed %d.\n",rt);
				ErrNo=-2;
			}else{
				snprintf(IEEEAddr,24,"%02X%02X%02X%02X%02X%02X%02X%02X",DevKeyInfo_ZigBee.IEEEAddr[0],DevKeyInfo_ZigBee.IEEEAddr[1],DevKeyInfo_ZigBee.IEEEAddr[2],DevKeyInfo_ZigBee.IEEEAddr[3],DevKeyInfo_ZigBee.IEEEAddr[4],DevKeyInfo_ZigBee.IEEEAddr[5],DevKeyInfo_ZigBee.IEEEAddr[6],DevKeyInfo_ZigBee.IEEEAddr[7]);
				snprintf(pSql,1024,"DELETE FROM DeviceTable_ZigBee WHERE IEEEAddr=x'%s' AND EP=%d;",IEEEAddr,DevKeyInfo_ZigBee.EP);
			}
		}else{//出错
			DEBUGLOG1(3,"Error, fSql_IsDeviceExistInDB failed %d.\n",rt);
			ErrNo=-3;
		}
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType %d.\n",pDevDB_DeviceKeyWords->DeviceType);
		ErrNo=-4;
		break;
	}

	//执行删除操作
	if(ErrNo==0){
		rt=sqlite3_prepare_v2(pGlobal_DevDB_DalDB, pSql, strlen(pSql), &pStmt, NULL);
		if(rt!=SQLITE_OK){
			DEBUGLOG1(3,"Error, sqlite3_prepare_v2 failed %d.\n",rt);
			ErrNo=-5;
		}else{
			rt=sqlite3_step(pStmt);
			if(rt!=SQLITE_DONE){
				DEBUGLOG1(3,"Error, sqlite3_step failed %d.\n",rt);
				ErrNo=-6;
			}
		}
	}

	if(pStmt!=NULL){
		sqlite3_finalize(pStmt);
	}

	return ErrNo;
}

//查找设备，获取设备数据
//pDevDB_DeviceKeyWords - 设备标识
//return NULL-未找到，else-返回设备指针，这个函数将克隆找到的设备数据，使用后一定要调用fDevDB_DestroyDeviceNode销毁返回结果
stDevDB_DeviceNode *fDevDB_GetDeviceNode(stDevDB_DeviceKeyWords *pDevDB_DeviceKeyWords)
{
	int rt;
	char pSql[1024];
	sqlite3_stmt *pStmt=NULL;
	char IEEEAddr[18];
	stDevKeyInfo_ZigBee DevKeyInfo_ZigBee;
	stDevDB_DeviceNode *pRet=NULL;

	if(pDevDB_DeviceKeyWords==NULL){
		return NULL;
	}

	switch(pDevDB_DeviceKeyWords->DeviceType){
	case DEVTYPE_ZIGBEE:
		rt=fParseKeyInfo_ZigBee(&DevKeyInfo_ZigBee, pDevDB_DeviceKeyWords->DeviceKeyInfo);
		if(rt!=0){
			DEBUGLOG1(3,"Error, fParseKeyInfo_ZigBee falied %d.\n",rt);
			return NULL;
		}else{
			snprintf(IEEEAddr,24,"%02X%02X%02X%02X%02X%02X%02X%02X",DevKeyInfo_ZigBee.IEEEAddr[0],DevKeyInfo_ZigBee.IEEEAddr[1],DevKeyInfo_ZigBee.IEEEAddr[2],DevKeyInfo_ZigBee.IEEEAddr[3],DevKeyInfo_ZigBee.IEEEAddr[4],DevKeyInfo_ZigBee.IEEEAddr[5],DevKeyInfo_ZigBee.IEEEAddr[6],DevKeyInfo_ZigBee.IEEEAddr[7]);
			snprintf(pSql,1024,"SELECT * FROM DeviceTable_ZigBee WHERE IEEEAddr=x'%s' AND EP=%d;",IEEEAddr,DevKeyInfo_ZigBee.EP);
			sqlite3_prepare_v2(pGlobal_DevDB_DalDB, pSql, strlen(pSql), &pStmt, NULL);
			rt=sqlite3_step(pStmt);
			if(rt==SQLITE_ROW){
				pRet=fDevDB_CreatDeviceNode(pDevDB_DeviceKeyWords->DeviceType);
				if(pRet==NULL){
					DEBUGLOG0(3,"Error, fDevDB_CreatDeviceNode failed.\n");
				}else{
					rt=fSql_SqlTableRowToDeviceNode(pStmt, pRet);
					if(rt!=0){
						DEBUGLOG1(3,"Error, fSql_SqlTableRowToDeviceNode failed %d.\n",rt);
					}
				}
			}else if(rt==SQLITE_DONE){//没找到
			}else{
				DEBUGLOG1(3,"Error, sqlite3_step failed %d.\n",rt);
			}
		}
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType %d.\n",pDevDB_DeviceKeyWords->DeviceType);
		break;
	}

	if(pStmt!=NULL){
		sqlite3_finalize(pStmt);
	}

	return pRet;
}

//返回指定类型的设备关键字列表
//DeviceType - 指定的设备类型
//StartIndex - 输入，开始位置，表示从第几个设备开始返回(从0开始)
//MaxNum - 输入，返回的最大数量
//ppKeyWordsList - 调用者提供空间，指向stDevDB_DeviceKeyWords pKeyWordsList[MaxNum]型的数组，用于返回获取到的关键字列表
//return <0-失败，else-返回的数量
int fDevDB_GetDeviceKeyWordsList(enDeviceType DeviceType, int StartIndex, int MaxNum, stDevDB_DeviceKeyWords *pKeyWordsList)
{
	int rt,n,NumFound=0;
	char pSql[1024];
	sqlite3_stmt *pStmt=NULL;
	stDevKeyInfo_ZigBee DevKeyInfo_ZigBee;

	if((StartIndex<0)||(MaxNum<0)||(pKeyWordsList==NULL)){
		return -1;
	}

	switch(DeviceType){
	case DEVTYPE_ZIGBEE:
		sprintf(pSql,"SELECT IEEEAddr,EP,ProfileID,DeviceID FROM DeviceTable_ZigBee;");
		rt=sqlite3_prepare_v2(pGlobal_DevDB_DalDB, pSql, strlen(pSql), &pStmt, NULL);
		if(rt!=SQLITE_OK){
			DEBUGLOG1(3,"Error, sqlite3_prepare_v2 failed %d.\n",rt);
		}else{
			n=0;
			//找到索引起始位置
			while(n<StartIndex){
				rt=sqlite3_step(pStmt);
				if(rt!=SQLITE_ROW){
					break;
				}
			}
			if(n<StartIndex){//还没达到索引位置就没有了
				DEBUGLOG0(2,"Warning, indexing reached the end before the StartIndex.\n");
			}else{
				while(1){
					if(NumFound>=MaxNum){
						break;
					}
					rt=sqlite3_step(pStmt);
					if(rt==SQLITE_ROW){//找到了
						pKeyWordsList[NumFound].DeviceType=DEVTYPE_ZIGBEE;
						//解析数据库中提取到的数据
						memcpy(DevKeyInfo_ZigBee.IEEEAddr,sqlite3_column_blob(pStmt,0),8);
						DevKeyInfo_ZigBee.EP=sqlite3_column_int(pStmt,1);
						DevKeyInfo_ZigBee.ProfileID=sqlite3_column_int(pStmt,2);
						DevKeyInfo_ZigBee.DeviceID=sqlite3_column_int(pStmt,3);
						fGenKeyInfo_ZigBee(pKeyWordsList[NumFound].DeviceKeyInfo,&DevKeyInfo_ZigBee);
						NumFound++;
					}else if(rt==SQLITE_DONE){//没有了
						break;
					}else{
						DEBUGLOG1(3,"Error, sqlite3_step failed %d.\n",rt);
						break;
					}
				}
			}
		}
		break;
	default:
		DEBUGLOG1(3,"Error, unknown DeviceType %d.\n",DeviceType);
		break;
	}

	//释放资源
	if(pStmt!=NULL){
		sqlite3_finalize(pStmt);
	}

	return NumFound;
}


//搜索符合指定特征的设备
//pSearchKey - 查找的设备特征
//Num - ppDevDB_DeviceNodeList能容纳的指针数量
//ppDevDB_DeviceNodeList - 调用者提供空间，用于返回找到的设备节点指针列表，指向(stDevDB_DeviceNode *)ppDevDB_DeviceNodeList[N]型指针数组
//return - 找到的设备节点数量，表示ppDevDB_DeviceNodeList数组中实际填入的指针数量，<0表示错误
//注意，
//1、ppDevDB_DeviceNodeList在使用后要调用fDevDB_DestroyDeviceNode逐一释放每一个元素指针，如下:
//	for(i=0;i<Num;i++) {fDevDB_DestroyDeviceNode(ppDevDB_DeviceNodeList[i]);}
//2、如果返回值等于Num，即提供的数组被填满，则有可能有更多的符合条件的设备
//3、如果查找的中间发生了错误，则只返回成功的那一部分
int fDevDB_FindDeviceNode(stDevDB_SearchKey *pSearchKey, int Num, stDevDB_DeviceNode *ppDevDB_DeviceNodeList[])
{
	int rt,n=0;
	char pSql[1024];
	char *pWrite=NULL;
	sqlite3_stmt *pStmt=NULL;
	stDevDB_DeviceNode *pNewDeviceNode=NULL;

	if((pSearchKey==NULL)||(Num<=0)||(ppDevDB_DeviceNodeList==NULL)){
		return -1;
	}

	switch(pSearchKey->DeviceType){
	case DEVTYPE_ZIGBEE:
		pWrite=pSql;
		pWrite+=sprintf(pWrite,"SELECT * FROM DeviceTable_ZigBee WHERE ");
		if(pSearchKey->un.SearchKey_ZigBee.AddrType==0){//0-NwkAddr、1-IEEEAddr
			pWrite+=sprintf(pWrite,"NwkAddr=%d ",pSearchKey->un.SearchKey_ZigBee.unAddr.NwkAddr);
		}else{
			pWrite+=sprintf(pWrite,"IEEEAddr=x'%02X%02X%02X%02X%02X%02X%02X%02X' ",pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[0],pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[1],pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[2],pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[3],pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[4],pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[5],pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[6],pSearchKey->un.SearchKey_ZigBee.unAddr.IEEEAddr[7]);
		}
		if(pSearchKey->un.SearchKey_ZigBee.doMatchEP){
			pWrite+=sprintf(pWrite,"AND EP=%d;",pSearchKey->un.SearchKey_ZigBee.EP);
		}else{
			pWrite+=sprintf(pWrite,";");
		}
		rt=sqlite3_prepare_v2(pGlobal_DevDB_DalDB, pSql, strlen(pSql), &pStmt, NULL);
		if(rt!=SQLITE_OK){
			DEBUGLOG1(3,"Error, sqlite3_prepare_v2 failed %d.\n",rt);
		}else{
			n=0;
			while(1){
				rt=sqlite3_step(pStmt);
				if(rt==SQLITE_ROW){//找到了
					pNewDeviceNode=fDevDB_CreatDeviceNode(DEVTYPE_ZIGBEE);
					if(pNewDeviceNode==NULL){
						DEBUGLOG0(3,"Error, fDevDB_CreatDeviceNode failed.\n");
						break;
					}else{
						rt=fSql_SqlTableRowToDeviceNode(pStmt,pNewDeviceNode);
						if(rt!=0){
							DEBUGLOG1(3,"Error, fSql_SqlTableRowToDeviceNode failed %d.\n",rt);
							fDevDB_DestroyDeviceNode(pNewDeviceNode);
							break;
						}else{
							ppDevDB_DeviceNodeList[n]=pNewDeviceNode;
						}
					}
				}else if(rt==SQLITE_DONE){//没有了
					break;
				}else{
					DEBUGLOG1(3,"Error, sqlite3_step failed %d.\n",rt);
					break;
				}

				if(++n>=Num){
					break;
				}
			}
		}
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType %d.\n",pSearchKey->DeviceType);
		break;
	}

	if(pStmt!=NULL){
		sqlite3_finalize(pStmt);
	}

	return n;
}

//统计当前的设备总数量
//DeviceType - 指定统计的设备类型
//return 统计结果，<0-表示失败
//当前仅支持ZigBee设备类型 DEVTYPE_ZIGBEE
int fDevDB_Statistic_DeviceNum(enDeviceType DeviceType)
{
	int rt,ErrNo=0,DeviceNum=0;
	char pSql[1024];
	sqlite3_stmt *pStmt=NULL;

	switch(DeviceType){
	case DEVTYPE_ZIGBEE:
		sprintf(pSql,"SELECT COUNT(*) FROM DeviceTable_ZigBee;");
		break;
	default:
		DEBUGLOG1(3,"Error, undefined DeviceType %d.\n",DeviceType);
		ErrNo=-1;
		break;
	}

	if(ErrNo==0){
		rt=sqlite3_prepare_v2(pGlobal_DevDB_DalDB, pSql, strlen(pSql), &pStmt, NULL);
		if(rt!=SQLITE_OK){
			DEBUGLOG1(3,"Error, sqlite3_prepare_v2 failed %d.\n",rt);
			ErrNo=-2;
		}else{
			rt=sqlite3_step(pStmt);
			if(rt==SQLITE_ROW){
				DeviceNum=sqlite3_column_int(pStmt,0);
			}else{
				DEBUGLOG1(3,"Error, sqlite3_step failed %d.\n",rt);
				ErrNo=-3;
			}
		}
	}

	//释放资源
	if(pStmt!=NULL){
		sqlite3_finalize(pStmt);
	}

	if(ErrNo!=0){
		return ErrNo;
	}else{
		return DeviceNum;
	}
}









