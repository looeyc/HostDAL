/*
 * hostdal.h
 *
 *  Created on: 2014-2-7
 *      Author: chengm
 */

#ifndef HOSTDAL_H_
#define HOSTDAL_H_

#include "dal_errno.h"
#include "dal_event.h"

//设备名称最大长度（字节）
#define MAXDEVICENAMELEN 32
//设备描述信息最大长度（字节）
#define MAXDEVICEDESCLEN 64

/*******************************************************************************************************************/
//DAL_Init																											|
//功能：DAL模块初始化																									|
//说明：																												|
//	a) 在调用DAL其他接口之前必须先调用该接口；																				|
//	b) 该接口只能调用一次；																								|
//	c) DAL内部存在一个事件队列，当事件处理回调pEventCB被调用时，其输入参数pEvent指向事件队列队首，也就是当前事件。					|
//	   回调函数应当尽快完成对事件的处理，否则后续事件可能会因为队列满而丢失。如果处理时间比较长，或者事件需要传递给其他处理者，			|
//	   回调函数应当通过复制的方式将事件保存下来再处理。																		|
/*******************************************************************************************************************/
//配置参数
typedef struct{
	void (*pEventCB)(stEvent *pEvent);//函数指针，注册事件处理回调，回调如果长时间不返回，后续所有事件（无论优先级高低）将会被阻塞在队列里
	char *pZNPComPath;			//Linux下的ZNP串口设备路径，例如Ubuntu下的“/dev/ttyS0”

	//底层ZNP配置参数，如果不关心的话，可以全部取0--------------------------------------------------------------
	char isValid_IEEEAddr;		//0-未指定、1-指定
	unsigned char IEEEAddr[8];	//对应的isValid_xxx为1时有效，如果为全0xFF，则表示使用NV参数中的IEEEAddr

	//以下网络参数中任何一个被指定时，ZNP启动的时候都会将所有之前的网络状态NV参数清空，“一切从0开始”，所有的子节点记录都会被清掉
	//指定网络参数PANID
	char isValid_PANID;			//0-未指定、1-指定
	unsigned short PANID;		//对应的isValid_xxx为1时有效，如果为0xFFFF，则表示由协调器自己选择
	//指定网络参数Extended PANID
	char isValid_ExtPANID;		//0-未指定、1-指定
	unsigned char ExtPANID[8];	//对应的isValid_xxx为1时有效，如果为全0xFF，则表示
}stDALCfg;

//pDALCfg：初始化DAL模块所需的配置参数。
DALStatus DAL_Init(stDALCfg *pDALCfg);

/*******************************************************************************************************************/
//DAL_GetDALDetails																									|
//功能：获取关于DAL的详细信息，比如DAL本身的软件版本、支持的功能等。															|
/*******************************************************************************************************************/
//ZigBee子模块
typedef struct{
	unsigned short ZigBeeMVersion; //子模块版本号

	//运行时参数
	unsigned char LocalIEEEAddr[8];//znp模块自身的IEEE地址
	unsigned short PANID;
	unsigned char ExtPANID[8];
}stDALSubMDetails_ZigBee;

//WebCam子模块
typedef struct{
	char WebCamMVersion; //子模块版本号
}stDALSubMDetails_WebCam;

//RepExt子模块
typedef struct{
	char RepExtMVersion; //子模块版本号
}stDALSubMDetails_RepExt;

//DAL详细信息
typedef struct{
	//DAL版本
	unsigned short DALVersion;

	//各子模块详细参数
	stDALSubMDetails_ZigBee DALSubMDetails_ZigBee;
	stDALSubMDetails_WebCam DALSubMDetails_WebCam;
	stDALSubMDetails_RepExt DALSubMDetails_RepExt;
}stDALDetails;

//分配一个DALDetails句柄
stDALDetails *AllocateDALDetails();
//释放一个DALDetails句柄
void DelocateDALDetails(stDALDetails *pDALDetails);

//pDALDetails：用于返回获取到的详细信息结构体
DALStatus DAL_GetDALDetails(stDALDetails *pDALDetails);

//用法举例：
//DALStatus rt;
//stDALDetails *pDALDetails=AllocateDALDetails();
//if(pDALDetails!=NULL){
//	rt=DAL_GetDALDetails(pDALDetails);
//	DelocateDALDetails(pDALDetails);
//}

/*******************************************************************************************************************/
//DAL_AddDevice																										|
//功能：手动方式添加设备。																								|
//说明：																												|
//	a) 有些类型的设备暂时不能通过DAL自动发现或者添加，例如由红外转发器控制的设备等，这些设备需要通过这个接口来手动添加；					|
//	b) 当设备添加成功后，DAL会有“设备加入”事件产生。																		|
/*******************************************************************************************************************/
//设备类型
typedef enum{
	ZIGBEEE,//标准ZigBee设备，符合ZCL协议
	WEBCAM,//WebCam设备
	REPTEXT//转发器扩设备
}enDevType;

//ZigBee类型设备描述符
typedef struct{
	unsigned char IEEEAddr[8];//ZigBee设备IEEE地址
}stDeviceDesc_ZigBee;

//WebCam类型设备描述符
typedef struct{
	//目前为空
}stDeviceDesc_WebCam;

//转发器扩展设备的类型定义
typedef enum{
	DEV_UNKNOWN,//未知设备类型
	DEV_USER_DEFINED,//用户自定义设备
	DEV_AIR_CONDITIONER,//空调设备
	DEV_TV_SET//电视机
}enRepExtDevType;

//RepExt类型设备描述符
typedef struct{
	stDALDeviceID RepDeviceID;//转发器的DAL设备ID
	enRepExtDevType RepExtType;//转发器扩展设备类型
}stDeviceDesc_RepExt;

//设备描述符
typedef struct{
	enDevType DevType;//决定un的具体内容
	union{
		stDeviceDesc_ZigBee DeviceDesc_ZigBee;
		stDeviceDesc_WebCam DeviceDesc_WebCam;
		stDeviceDesc_RepExt DeviceDesc_RepExt;
	}un;
}stDeviceDesc;

//pLLDeviceDesc：欲添加设备的描述信息，主机通过该参数知道需要添加的设备类型，并执行具体的添加过程。
DALStatus DAL_AddDevice(stDeviceDesc *pDeviceDesc);

/*******************************************************************************************************************/
//DAL_RemoveDevice																									|
//功能：移除设备。																										|
//说明：设备成功删除后，DAL会有“设备移除”事件产生。																			|
/*******************************************************************************************************************/
//pDeviceID：设备ID指针，输入需要移除的设备ID。
DALStatus DAL_RemoveDevice(stDALDeviceID *pDeviceID);

/*******************************************************************************************************************/
//DAL_DeviceCtrl																									|
//功能：对设备进行控制。																									|
//说明：有的设备在受到控制，发生状态改变后，可能会产生DAL事件。																|
/*******************************************************************************************************************/
//DAL模型下，设备操作的结构体定义
typedef struct{
	unsigned char CtrlID;//调用的方法ID
	unsigned char ParamNum;//参数数量，表示ParamList中的实际参数数量
	stParam ParamList[MAXPARAMNUM];//参数列表，数组下标为参数ID
}stDALCtrl;

//pDeviceID：设备ID指针，输入需要控制的设备ID；
//pDALCtrl：输入DAL控制操作。
DALStatus DAL_DeviceCtrl(stDALDeviceID *pDeviceID, stDALCtrl *pDALCtrl);

/*******************************************************************************************************************/
//DAL_GetDeviceIDList																								|
//功能：获取设备ID列表。																								|
/*******************************************************************************************************************/
//设备ID列表
typedef struct{
	unsigned int DeviceNum;//设备数量
	stDALDeviceID *pDALDeviceIDList;//指向stDALDeviceID类型的数组DALDeviceIDList[N]，其中N表示设备数量，也就是DeviceNum值
}stDeviceIDList;

//分配一个DeviceIDList句柄
stDeviceIDList *AllocateDeviceIDList();
//释放一个DeviceIDList句柄
void DelocateDeviceIDList(stDeviceIDList *pDeviceIDList);

//获取设备ID列表
//pDeviceIDList - 必须是由AllocateDeviceIDList分配的，使用后必须用DelocateDeviceIDList释放，即便返回值不是DALS_SUCCESS
DALStatus DAL_GetDeviceIDList(stDeviceIDList *pDeviceIDList);

//用法举例：
//DALStatus rt;
//stDeviceIDList *pDeviceIDList=AllocateDeviceIDList();
//if(pDeviceIDList!=NULL){
//	rt=DAL_GetDeviceIDList(pDeviceIDList);
//	DelocateDeviceIDList(pDeviceIDList);
//}

/*******************************************************************************************************************/
//DAL_GetDeviceStatus																								|
//功能：获取设备状态。																									|
/*******************************************************************************************************************/
//DAL模型下，设备的状态参数结构体定义
typedef struct{
	unsigned char StatusNum;//设备的DAL状态参数数量
	stParam StatusList[MAXSTATUSNUM];//设备的DAL状态值数组指针，数组下标为DAL状态ID，例如//pDALStatus[0]表示状态ID为0的状态值
}stDALStatus;

//分配一个stDALStatus句柄
stDALStatus *AllocateDALStatus();
//释放一个stDALStatus句柄
void DelocateDALStatus(stDALStatus *pDALStatus);

//pDeviceID：设备ID指针，需要访问的设备ID；
//pDALStatus：用于返回获取到的状态参数,由调用者提供，通过AllocateDALStatus分配，使用后必须用DelocateDALStatus释放。
DALStatus DAL_GetDeviceStatus(stDALDeviceID *pDeviceID, stDALStatus *pDALStatus);

//用法举例：
//stDALStatus *pDALStatus=AllocateDALStatus();//分配
//if(pDALStatus!=NULL){
//	rt=DAL_GetDeviceStatus(pDeviceID,pDALStatus);//调用
//	DelocateDeviceIDList(pDALStatus);//释放
//}

/*******************************************************************************************************************/
//DAL_SetDeviceName																									|
//功能：设置设备名称。																									|
/*******************************************************************************************************************/
//pDeviceID：设备ID指针，需要访问的设备ID；
//pDeviceName：设备名称，字符串，必须以“\0”结束，长度不能超过MAXDEVICENAMELEN，否则截断。
DALStatus DAL_SetDeviceName(stDALDeviceID *pDeviceID, char *pDeviceName);

//用法举例：
//DALStatus rt;
//unsigned char pDeviceID[24]={...};
//rt=DAL_SetDeviceName(pDeviceID,"TestDevice01");

/*******************************************************************************************************************/
//DAL_SetDeviceDesc																									|
//功能：设置设备描述信息。																								|
/*******************************************************************************************************************/
//pDeviceID：设备ID指针，需要访问的设备ID；
//pDeviceDesc：设备描述信息，字符串，必须以“\0”结束，长度不能超过MAXDEVICEDESCLEN，否则截断。
DALStatus DAL_SetDeviceDesc(stDALDeviceID *pDeviceID, char *pDeviceDesc);

//用法举例：
//DALStatus rt;
//unsigned char pDeviceID[24]={...};
//rt=DAL_SetDeviceDesc(pDeviceID,"This is a test device.");

/*******************************************************************************************************************/
//DAL_GetDeviceInfo																									|
//功能：获取设备信息，其中包括名称与描述等。																				|
/*******************************************************************************************************************/
typedef struct{
	char DeviceName[MAXDEVICENAMELEN];//字符串，设备名称
	char DeviceDesc[MAXDEVICEDESCLEN];//字符串，设备描述信息
}stDeviceInfo;

//分配一个stDeviceInfo句柄
stDeviceInfo *AllocateDeviceInfo();
//释放一个stDeviceInfo句柄
void DelocateDeviceInfo(stDeviceInfo *pDALStatus);

//pDeviceID：设备ID指针，需要访问的设备ID；
//pDeviceInfo：由调用者提供，用于返回设备信息，通过AllocateDeviceInfo分配，使用后必须调用DelocateDeviceInfo进行销毁
DALStatus DAL_GetDeviceInfo(stDALDeviceID *pDeviceID, stDeviceInfo *pDeviceInfo);

//用法举例：
//DALStatus rt;
//unsigned char pDeviceID[24]={...};
//stDeviceInfo *pDeviceInfo=AllocateDeviceInfo();
//if(pDeviceInfo!=NULL){
//	rt=DAL_GetDeviceInfo(pDeviceID,pDeviceInfo);
//	DelocateDeviceInfo(pDeviceInfo);
//}

#endif /* HOSTDAL_H_ */

























