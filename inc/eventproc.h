/*
 * eventproc.h
 *
 *  Created on: 2014-2-8
 *      Author: chengm
 */

#ifndef EVENTPROC_H_
#define EVENTPROC_H_

#include "hostdal.h"
#include "dal_event.h"

typedef struct{
	void (*pEventCB)(stEvent *pEvent);//函数指针，注册事件处理回调
}stEventProcCfg;

//初始化事件处理模块
DALStatus DAL_Event_Init(stEventProcCfg *pEventProcCfg);

//推送事件
//pEvent - 需要推送的事件指针
//说明：事件推送者自己创建事件，并将事件的指针作为参数传递给该API，推送时，事件被复制到模块内部，推送者的事件数据不会发生任何变化
//     如果推送者为事件分配了空间，需要推送者自己释放
DALStatus DAL_Event_Push(stEvent *pEvent);

#endif /* EVENTPROC_H_ */
