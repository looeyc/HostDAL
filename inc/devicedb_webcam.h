/*
 * devicedb_webcam.h
 *
 *  Created on: 2013-6-18
 *      Author: chengm
 */

#ifndef DEVICEDB_WEBCAM_H_
#define DEVICEDB_WEBCAM_H_

#define MAXSTREAMDESCNUM		2

#define MAXRTMPAPPNAME			32
#define MAXRTMPSTREAMNAME		32

#define MAXHLSPATHLEN			64
#define MAXHLSNAMELEN			64

#define WEBCAMERRSTAT_OK		0//工作正常
#define WEBCAMERRSTAT_TIMEOUT	1//连接超时

//流媒体类型
typedef enum{
	RTMP_STREAM,//RTMP流
	HLS_STREAM//HLS流
}enStreamType;

//适用于描述HLS流
typedef struct{
	int ImageWidth;//图像宽度
	int ImageHeight;//图像高度

	char HLSWebRoot[MAXHLSNAMELEN];//用于HLS的web根目录的绝对路径，例如"/home/chengm/ZBWeb3"
	char HLSWebMediaFolder[MAXHLSPATHLEN];//用于存放HLS媒体文件的总目录相对于HLSWebRoot的路径，例如"media"，则存放HLS媒体文件的目录的绝对路径为"/home/chengm/ZBWeb3/media"，该目录应当mount为tmpfs类型，并且设置为nfs共享
	char HLSStreamName[MAXHLSNAMELEN];//每个HLS流对应一个唯一的HLSStreamName，
									  //HostServer会在HLSWebMediaFolder下创建一个名字为HLSStreamName的文件夹，
									  //WebCam会在注册成功之后nfs挂载HLSStreamName文件夹，并在HLS流启动后向其中写入HLS流文件，其中ts文件序列命名规则为"<HLSStreamName>-xxx.ts"，m3u8索引文件命名为"<HLSStreamName>.m3u8"
}stHLS_STREAM;

//适用于描述RTMP流
typedef struct{
	int ImageWidth;//图像宽度
	int ImageHeight;//图像高度

	//发布的url为"rtmp://ServerName/AppName/StreamName"
	char AppName[MAXRTMPAPPNAME];//该结构体作为注册请求参数时，该字段无效
	char StreamName[MAXRTMPSTREAMNAME];//该结构体作为注册请求参数时，该字段无效
	char ServerName[MAXRTMPSTREAMNAME];//该结构体作为注册请求参数时，该字段无效
}stRTMP_STREAM;

//流描述符
typedef struct{
	enStreamType StreamType;//流媒体协议类型
	union{
		stRTMP_STREAM RTMP_STREAM;
		stHLS_STREAM HLS_STREAM;
	}un;
}stStreamDesc;

//WebCam设备的数据结构
typedef struct{
	//设备信息
	char pMAC[6];//MAC地址
	unsigned short CtrlPort;//控制端口，HostServer或者其他控制端通过该端口控制WebCam
	char pLanIP[16];//局域网IP地址，字符串，例如“192.168.1.111”
	int StreamNum;//支持的流数量,对应StreamDesc数组中的有效数量
	stStreamDesc StreamDesc[MAXSTREAMDESCNUM];//支持的媒体流列表

	//状态
	int TickCnt;//用于检查webcam设备是否发生了连接超时
	int ErroState;//状态 WEBCAMERRSTAT_OK, WEBCAMERRSTAT_GENERRO, WEBCAMERRSTAT_TIMEOUT...
}stDevNode_WebCam;

//销毁一个WebCam节点
void fDestroyDevNode_WebCam(stDevNode_WebCam *pDevNode_WebCam);

#endif /* DEVICEDB_WEBCAM_H_ */


