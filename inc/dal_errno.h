/*
 * dal_errno.h
 *
 *  Created on: 2014-2-7
 *      Author: chengm
 */

#ifndef DAL_ERRNO_H_
#define DAL_ERRNO_H_

//DAL API返回状态，取值定义参见“dal_errno.h“
typedef int DALStatus;

//具体的错误编号
#define DALS_SUCCESS				 0	//成功
#define DALS_GENERALERR 			-1	//通用错误，未指明具体原因
#define DALS_ILLEGPARAM 			-2	//输入参数非法
#define DALS_UNINITIALIZED 			-3	//模块未初始化
#define DALS_ILLEGREINITIALIZE 		-4	//模块已经初始化过了，非法的重初始化操作
#define DALS_DEVICENOTFOUND 		-5	//未找到指定设备
#define DALS_UNSUPPORTEDCTRL 		-6	//不支持的设备操作
#define DALS_MALLOCFAILED			-7	//分配存储空间失败
#define DALS_CREATTHREADFAILED		-8	//创建线程失败
#define DALS_ZIGBEEINITFAILED		-9	//ZigBee初始化失败
#define DALS_UNSUPPORTEDDEVICETYPE	-10
#define DALS_DATABASEACCESSFAILED	-11 //数据库访问失败
#define DALS_ZIGBEEFAILED			-12 //ZigBee访问失败

#endif /* DAL_ERRNO_H_ */
