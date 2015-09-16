/*
 * dal_event.h
 *
 *  Created on: 2014-2-7
 *      Author: chengm
 */

//在定义一个新事件的时候，要注意DAL_Event_Clone、DAL_Event_Destory两个函数中是否需要级联复制或者级联释放

#ifndef DAL_EVENT_H_
#define DAL_EVENT_H_

#include "dal_type.h"

#define MAXPARAMNUM 			32
#define MAXSTATUSNUM 			16

//DAL事件编号
#define DALEVENT_DEVICEJIONED		0x0000	//设备加入事件
#define DALEVENT_DEVICEREMOVED		0x0001	//设备移除事件
#define DALEVENT_DEVICEEVENT		0x0002	//DAL设备事件
#define DALEVENT_DEVICEEXCEPTION	0x0003	//设备异常报告

//设备ID长度
#define DEVICEIDLEN	24

typedef struct{
	unsigned char DeviceID[DEVICEIDLEN];
}stDALDeviceID;

//事件优先级
typedef enum{
	PRI_LOW,//低优先级，不重要的事件
	PRI_NORMAL,//一般
	PRI_HIGH//紧急，必须立即处理
}enEventPriority;

//设备加入事件
typedef struct{
	stDALDeviceID DeviceID;//加入的设备ID
}stEvent_DeviceJoined;

//设备移除事件
typedef struct{
	stDALDeviceID DeviceID;//被移除的设备ID
}stEvent_DeviceRemoved;

//DAL设备事件
typedef struct{
	stDALDeviceID DeviceID;//上报事件的设备ID
	unsigned char DALEventID;//上报的DAL事件ID
	unsigned char ParamNum;//事件参数数量
	stParam ParamList[MAXPARAMNUM];//参数数组，数组下标为参数ID
}stEvent_DeviceEvent;

//设备异常报告
typedef struct{
	stDALDeviceID DeviceID;//设备ID
	unsigned char ExceptionID;//异常代码
	unsigned char ParamNum;//事件参数数量
	stParam ParamList[MAXPARAMNUM];//参数数组，数组下标为参数ID
}stEvent_DeviceException;

//事件的结构体定义
typedef struct{
	unsigned short EventType;//事件类型
	enEventPriority Priority;//事件优先级，反应事件紧急程度
	union{
		//成员为各类具体的事件，具体是哪一个，由EventType字段决定
		stEvent_DeviceJoined Event_DeviceJoined;
		stEvent_DeviceRemoved Event_DeviceRemoved;
		stEvent_DeviceEvent Event_DALEvent;
		stEvent_DeviceException Event_DeviceException;
	}un;
}stEvent;

//克隆一个事件
//克隆过程中会分配新的空间，因此克隆得到的事件在使用之后需要调用DAL_Event_Destory销毁
stEvent *DAL_Event_Clone(stEvent *pEvent);

//销毁一个事件
//pEvent - 需要销毁的时间，目前为止，只有通过DAL_Event_Clone得到的事件需要调用该函数进行销毁
void DAL_Event_Destory(stEvent *pEvent);

#endif /* DAL_EVENT_H_ */
