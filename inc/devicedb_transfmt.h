/*
 * devicedb_transfmt.h
 *
 *  Created on: 2013-6-26
 *      Author: chengm
 */

#ifndef TRANSFMT_H_
#define TRANSFMT_H_

//将设备节点数据结构转换为连续的字节流形式，便于存储和传输
//DevDB_DeviceNode - 待转换的结构体数据
//pBuffer - 调用者提供空间，存储转换结果
//BufferLen - 提供的存储空间长度（字节）
//return  <=0-出错，else-实际转换出来的数据长度（字节）
int fStToBin_stDevDB_DeviceNode(stDevDB_DeviceNode *pDevDB_DeviceNode, char *pBuffer, int BufferLen);

//将设备节点数据的连续字节流式恢复为数据结构形式
//pBuffer - 待转换的二进制数据存储区
//DataLen - 数据长度（字节）
//return - NULL-失败，else-恢复出来的设备数据结构，
//注意：该函数会分配存储空间，使用后一定要调用fDevDB_DestroyDeviceNode销毁,fDevDB_DestroyDeviceNode位于devicedb.h！
stDevDB_DeviceNode *fBinToSt_stDevDB_DeviceNode(char *pData, int DataLen);

#endif /* TRANSFMT_H_ */






