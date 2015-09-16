/*
 * dal_type.h
 *
 *  Created on: 2014-2-28
 *      Author: chengm
 */

#ifndef DAL_TYPE_H_
#define DAL_TYPE_H_

#define	DAL_NODATA          (unsigned char)0x00
#define	DAL_BOOL            (unsigned char)0x10
#define	DAL_UINT8           (unsigned char)0x20
#define	DAL_UINT16          (unsigned char)0x21
#define	DAL_UINT24          (unsigned char)0x22
#define	DAL_UINT32          (unsigned char)0x23
#define	DAL_UINT40          (unsigned char)0x24
#define	DAL_UINT48          (unsigned char)0x25
#define	DAL_UINT56          (unsigned char)0x26
#define	DAL_UINT64          (unsigned char)0x27
#define	DAL_INT8            (unsigned char)0x28
#define	DAL_INT16           (unsigned char)0x29
#define	DAL_INT24           (unsigned char)0x2A
#define	DAL_INT32           (unsigned char)0x2B
#define	DAL_INT40           (unsigned char)0x2C
#define	DAL_INT48           (unsigned char)0x2D
#define	DAL_INT56           (unsigned char)0x2E
#define	DAL_INT64           (unsigned char)0x2F
#define	DAL_ENUM8           (unsigned char)0x30
#define	DAL_SINGLEFLOAT     (unsigned char)0x39
#define	DAL_DOUBLEFLOAT     (unsigned char)0x3A
#define	DAL_STRING          (unsigned char)0x42

#define	DAL_NODATA_LEN			0
#define	DAL_BOOL_LEN			1
#define	DAL_UINT8_LEN			1
#define	DAL_UINT16_LEN			2
#define	DAL_UINT24_LEN			3
#define	DAL_UINT32_LEN			4
#define	DAL_UINT40_LEN			5
#define	DAL_UINT48_LEN			6
#define	DAL_UINT56_LEN			7
#define	DAL_UINT64_LEN			8
#define	DAL_INT8_LEN			1
#define	DAL_INT16_LEN			2
#define	DAL_INT24_LEN			3
#define	DAL_INT32_LEN			4
#define	DAL_INT40_LEN			5
#define	DAL_INT48_LEN			6
#define	DAL_INT56_LEN			7
#define	DAL_INT64_LEN			8
#define	DAL_ENUM8_LEN			1
#define	DAL_SINGLEFLOAT_LEN		4
#define	DAL_DOUBLEFLOAT_LEN		8

#define MAXPARAMDATALEN 256

//DAL模型下，参数的结构体定义
typedef struct{
	unsigned char DataType;//数据类型DAL_xxx
	unsigned char Data[MAXPARAMDATALEN];//存放数据的存储区，实际数据长度根据类型确定
}stParam;

#endif /* DAL_TYPE_H_ */
