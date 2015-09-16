/*
 * eventproc.c
 *
 *  Created on: 2014-2-8
 *      Author: chengm
 */

//DAL中的事件处理模块接受各子模块的事件推送，将事件按照优先级排队，并逐一调用注册的事件处理回调进行处理。
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include "debuglog.h"
#include "dal_event.h"
#include "dal_errno.h"
#include "eventproc.h"
#include "hostdal.h"

//事件队列节点，链表元素
typedef struct stEventNode_ stEventNode;
struct stEventNode_{
	stEventNode *pNext;
	stEvent *pEvent;
};

//事件模块全局参数
struct{
	unsigned char isInitialized;//0-没有初始化，1-已经初始化
	void (*pEventCB)(stEvent *pEvent);//事件处理回调
	//不同优先级的事件队列指针，指向队首元素
	stEventNode *pEventQueue_PRI_LOW;
	stEventNode *pEventQueue_PRI_NORMAL;
	stEventNode *pEventQueue_PRI_HIGH;

	//访问锁
	pthread_mutex_t mutexEventQueue;
}DALEventProcStatus;

//克隆一个事件
//克隆过程中会分配新的空间，因此克隆得到的事件在使用之后需要调用DAL_Event_Destory销毁
stEvent *DAL_Event_Clone(stEvent *pEvent)
{
	stEvent *pRet=NULL;

	if(pEvent==NULL){
		return NULL;
	}

	pRet=(stEvent *)malloc(sizeof(stEvent));
	if(pRet==NULL){
		DEBUGLOG0(3,"Error, malloc failed, can't clone event.\n");
		return NULL;
	}
	memset(pRet,0,sizeof(stEvent));

	//级联复制
	memcpy(pRet,pEvent,sizeof(stEvent));

	return pRet;
}

//销毁一个事件
void DAL_Event_Destory(stEvent *pEvent)
{
	if(pEvent!=NULL){
		free(pEvent);
	}
}

//将一个事件加入队列
//pEvent - 需要加入的事件
//ppEventQueue - 指向目标事件队列的指针
static DALStatus DAL_Event_QueuePush(stEvent *pEvent, stEventNode **ppEventQueue)
{
	stEventNode *pScan;
	stEventNode *pNewNode=NULL;

	if((pEvent==NULL)||(ppEventQueue==NULL)){
		return DALS_ILLEGPARAM;
	}

	//创建新节点元素
	pNewNode=malloc(sizeof(stEventNode));
	if(pNewNode==NULL){
		return DALS_MALLOCFAILED;
	}
	memset(pNewNode,0,sizeof(stEventNode));
	pNewNode->pEvent=pEvent;

	//新节点元素加入队列
	pthread_mutex_lock(&DALEventProcStatus.mutexEventQueue);
	pScan=*ppEventQueue;
	if(pScan==NULL){
		*ppEventQueue=pNewNode;
	}else{
		while(pScan->pNext!=NULL){//找到队尾
			pScan=pScan->pNext;
		}
		pScan->pNext=pNewNode;
	}
	pthread_mutex_unlock(&DALEventProcStatus.mutexEventQueue);

	return DALS_SUCCESS;
}

//移除队首事件节点
//ppEventQueue - 指向目标事件队列的指针
static DALStatus DAL_Event_QueuePop(stEventNode **ppEventQueue)
{
	stEventNode *pNewQueHeader=NULL;

	if(ppEventQueue==NULL){
		return DALS_ILLEGPARAM;
	}

	if(*ppEventQueue==NULL){
		return DALS_SUCCESS;
	}

	pthread_mutex_lock(&DALEventProcStatus.mutexEventQueue);
	pNewQueHeader=(*ppEventQueue)->pNext;
	//销毁节点
	DAL_Event_Destory((*ppEventQueue)->pEvent);
	free(*ppEventQueue);
	*ppEventQueue=pNewQueHeader;
	pthread_mutex_unlock(&DALEventProcStatus.mutexEventQueue);

	return DALS_SUCCESS;
}

//事件上报线程，调用事件回调对事件进行处理
static void *Thread_Event_Submit(void *pArg)
{
	//轮询三个事件队列
	while(1){
		if(DALEventProcStatus.pEventQueue_PRI_HIGH!=NULL){//如果EventQueue_PRI_HIGH非空
			//处理事件
			DALEventProcStatus.pEventCB(DALEventProcStatus.pEventQueue_PRI_HIGH->pEvent);
			//移除队首事件
			DAL_Event_QueuePop(&DALEventProcStatus.pEventQueue_PRI_HIGH);
			continue;
		}
		if(DALEventProcStatus.pEventQueue_PRI_NORMAL!=NULL){//如果EventQueue_PRI_NORMAL非空
			//处理事件
			DALEventProcStatus.pEventCB(DALEventProcStatus.pEventQueue_PRI_NORMAL->pEvent);
			//移除队首事件
			DAL_Event_QueuePop(&DALEventProcStatus.pEventQueue_PRI_NORMAL);
			continue;
		}
		if(DALEventProcStatus.pEventQueue_PRI_LOW!=NULL){//如果EventQueue_PRI_LOW非空
			//处理事件
			DALEventProcStatus.pEventCB(DALEventProcStatus.pEventQueue_PRI_LOW->pEvent);
			//移除队首事件
			DAL_Event_QueuePop(&DALEventProcStatus.pEventQueue_PRI_LOW);
			continue;
		}

		//一次轮询完毕
		usleep(50000);//等待50ms
	}

	return NULL;
}

//初始化事件处理模块
DALStatus DAL_Event_Init(stEventProcCfg *pEventProcCfg)
{
	int rt;
	pthread_t ntid;

	if(pEventProcCfg==NULL){
		return DALS_ILLEGPARAM;
	}
	if(pEventProcCfg->pEventCB==NULL){
		return DALS_ILLEGPARAM;
	}

	memset(&DALEventProcStatus,0,sizeof(DALEventProcStatus));
	DALEventProcStatus.isInitialized=1;
	DALEventProcStatus.pEventCB=pEventProcCfg->pEventCB;
	pthread_mutex_init(&DALEventProcStatus.mutexEventQueue,NULL);

	//创建事件上报线程
	rt=pthread_create(&ntid,NULL,Thread_Event_Submit,NULL);
	if(rt!=0){
		DEBUGLOG1(3,"Error, pthread_create failed %d, can't creat thread Thread_Event_Submit.\n",rt);
		return DALS_CREATTHREADFAILED;
	}

	return DALS_SUCCESS;
}

//推送事件
DALStatus DAL_Event_Push(stEvent *pEvent)
{
	DALStatus rt;
	stEvent *pNewEvent=NULL;

	//检测事件处理模块是否已经初始化
	if(DALEventProcStatus.isInitialized!=1){
		return DALS_UNINITIALIZED;
	}
	//参数检查
	if(pEvent==NULL){
		return DALS_ILLEGPARAM;
	}

	//克隆事件
	pNewEvent=DAL_Event_Clone(pEvent);
	if(pNewEvent==NULL){
		DEBUGLOG0(3,"Error, DAL_Event_Clone failed, can't push event.\n");
		return DALS_GENERALERR;
	}

	//将事件加入对应优先级的队列
	switch(pEvent->Priority){
	case PRI_LOW:
		rt=DAL_Event_QueuePush(pNewEvent,&DALEventProcStatus.pEventQueue_PRI_LOW);
		break;
	case PRI_NORMAL:
		rt=DAL_Event_QueuePush(pNewEvent,&DALEventProcStatus.pEventQueue_PRI_NORMAL);
		break;
	case PRI_HIGH:
		rt=DAL_Event_QueuePush(pNewEvent,&DALEventProcStatus.pEventQueue_PRI_HIGH);
		break;
	default:
		DEBUGLOG1(3,"Error, undefined event priority %d.\n",pEvent->Priority);
		DAL_Event_Destory(pNewEvent);
		break;
	}

	return rt;
}


















