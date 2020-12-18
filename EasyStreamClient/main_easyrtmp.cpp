#include "StdAfx.h"

#define _CRTDBG_MAP_ALLOC
#include <stdio.h>



#ifdef _WIN32
#include "windows.h"
#else
#include <string.h>
#include <unistd.h>
#endif
#include <stdio.h> 
#include <iostream> 
#include <time.h> 
#include <stdlib.h>
#include "EasyStreamClientAPI.h"
#include "EasyRTMPAPI.h"
#include <list>
//#include "EasyHLS2MP4.h"

//#ifdef _WIN32
//#pragma comment(lib,"libEasyRTMP/libEasyRTMP.lib")
//#endif



#define MAX_RTMP_URL_LEN 256
#ifndef MAX_PATH
#define MAX_PATH        260
#endif
typedef struct __EASY_CLIENT_OBJ_T
{
	Easy_Handle streamClientHandle;
	Easy_Handle pusherHandle;
	EASY_MEDIA_INFO_T mediainfo;


	EASY_RTP_CONNECT_TYPE	connectType;
	char	szURL[MAX_PATH];
	char	szOutputFormat[MAX_PATH];
	char	szOutputAddress[MAX_PATH];
	int		nTimeout;

	FILE	*fOut;
}EASY_CLIENT_OBJ_T;


#ifdef _WIN32
#define EASY_RTMP_KEY "79736C36655969576B5A734132635A666F446B6D7065394659584E35556C524E55463949535573755A58686C4B56634D5671442F7066396C59584E35"
#define STREAM_CLIENT_KEY "4638674F756F69576B5A734132635A666F446B6D7065744659584E35553352795A574674513278705A5735304C6D56345A623558444661672F36582F5A57467A65513D3D"

#else // linux
#define EASY_RTMP_KEY "79736C36655A4F576B596F4132635A666F446B6D70664E6C59584E35636E52746346396F61577370567778576F502B6C2F32566863336B3D"
#define STREAM_CLIENT_KEY "4638674F75704F576B596F4132635A666F446B6D7065396C59584E35633352795A574674593278705A5735306846634D5671442F7066396C59584E35"
#endif



//char* gSrcRtspAddr = "rtsp://222.190.121.133:554/PVG/live/?PVG=172.15.1.26:2100/admin/admin/av/1/6/38";
//char* gSrcRtspAddr = "rtsp://admin:admin@192.168.1.224/11";
//char* gDestRtmpAddr = "rtmp://www.easydss.com:10085/hls/SJNHKPzm?sign=rkxpEHKwGQ";
//char* gDestRtmpAddr = "rtmp://212.64.34.165:10085/hls/kimtest1";

/* EasyRTMP callback */
int __EasyRTMP_Callback(int _frameType, char *pBuf, EASY_RTMP_STATE_T _state, void *_userPtr)
{
	switch (_state)
	{
	case EASY_RTMP_STATE_CONNECTING:
		printf("EasyRTMP Connecting...\n");
		break;
	case EASY_RTMP_STATE_CONNECTED:
		printf("EasyRTMP Connected...\n");
		break;
	case EASY_RTMP_STATE_CONNECT_FAILED:
		printf("EasyRTMP Connect failed...\n");
		break;
	case EASY_RTMP_STATE_CONNECT_ABORT:
		printf("EasyRTMP Connect abort...\n");
		break;
	case EASY_RTMP_STATE_DISCONNECTED:
		printf("EasyRTMP Disconnect...\n");
		break;
	}

	return 0;
}

static int writeFile(const char* _fileName, void* _buf, int _bufLen)
{
	FILE * fp = NULL;
	if (NULL == _buf || _bufLen <= 0) return (-1);

	fp = fopen(_fileName, "ab+"); // 必须确保是以 二进制写入的形式打开

	if (NULL == fp)
	{
		return (-1);
	}

	fwrite(_buf, _bufLen, 1, fp); //二进制写

	fclose(fp);
	fp = NULL;

	return 0;
}

static int readFile(const char* _fileName, void* _buf, int _bufLen)
{
	FILE* fp = NULL;
	if (NULL == _buf || _bufLen <= 0) return (-1);

	fp = fopen(_fileName, "rb"); // 必须确保是以 二进制读取的形式打开 

	if (NULL == fp)
	{
		return (-1);
	}

	fread(_buf, _bufLen, 1, fp); // 二进制读

	fclose(fp);
	return 0;
}


/* EasyStreamClient callback */
int Easy_APICALL __EasyStreamClientCallBack(void *channelPtr, int frameType, void *pBuf, EASY_FRAME_INFO* frameInfo)
{
	Easy_Bool bRet = 0;
	int iRet = 0;
	EASY_CLIENT_OBJ_T	*pEasyClient = (EASY_CLIENT_OBJ_T *)channelPtr;

	if (frameType == EASY_SDK_VIDEO_FRAME_FLAG || frameType == EASY_SDK_AUDIO_FRAME_FLAG)
	{
		if (frameInfo && frameInfo->length && frameType == EASY_SDK_AUDIO_FRAME_FLAG)
		{

		}

		if (frameInfo && frameInfo->length && frameType == EASY_SDK_VIDEO_FRAME_FLAG)
		{
#ifdef _DEBUG
			char *pbuf = (char*)pBuf;
			printf("%02X %02X %02X %02X %02X %02X\n", (unsigned char)pbuf[0], (unsigned char)pbuf[1],
													  (unsigned char)pbuf[2], (unsigned char)pbuf[3],
													  (unsigned char)pbuf[4], (unsigned char)pbuf[5]);
#endif

			if (NULL == pEasyClient->fOut && 0 == strncmp(pEasyClient->szOutputFormat, "file", 4))
			{
				if (frameInfo->type == EASY_SDK_VIDEO_FRAME_I)
				{
					pEasyClient->fOut = fopen(pEasyClient->szOutputAddress, "wb");
				}
			}

			if (NULL != pEasyClient->fOut)
			{
				fwrite(pBuf, 1, frameInfo->length, pEasyClient->fOut);
				fflush(pEasyClient->fOut);
			}
		}

		if (0 == strncmp(pEasyClient->szOutputFormat, "rtmp", 4))
		{
			if (frameInfo && frameInfo->length && strlen(pEasyClient->szOutputAddress) > 0)
			{
#if 1
				//printf("video timestamp = %ld : %06ld \n", frameInfo->timestamp_sec, frameInfo->timestamp_usec);
				if (pEasyClient->pusherHandle == NULL)
				{
					pEasyClient->pusherHandle = EasyRTMP_Create();
					EasyRTMP_SetCallback(pEasyClient->pusherHandle, __EasyRTMP_Callback, NULL);
					bRet = EasyRTMP_Connect(pEasyClient->pusherHandle, pEasyClient->szOutputAddress);
					if (!bRet)
					{
						printf("Fail to EasyRTMP_Connect ...\n");
					}

					iRet = EasyRTMP_InitMetadata(pEasyClient->pusherHandle, &pEasyClient->mediainfo, 1024);
					if (iRet < 0)
					{
						printf("Fail to InitMetadata ...\n");
					}
				}

				if (pEasyClient->pusherHandle)
				{
					EASY_AV_Frame avFrame;
					memset(&avFrame, 0, sizeof(EASY_AV_Frame));
					avFrame.u32AVFrameFlag = frameType;
					avFrame.u32AVFrameLen = frameInfo->length;
					avFrame.pBuffer = (unsigned char*)pBuf;
					avFrame.u32VFrameType = frameInfo->type;
					avFrame.u32TimestampSec = frameInfo->timestamp_sec;
					avFrame.u32TimestampUsec = frameInfo->timestamp_usec;

					/*if (frameType == EASY_SDK_VIDEO_FRAME_FLAG)
					{
						static int h264index = 0;
						printf("%s pBuf=%02X %02X %02X %02X %02X %02X %02X \n", __FUNCTION__,
							avFrame.pBuffer[0], avFrame.pBuffer[1], avFrame.pBuffer[2], avFrame.pBuffer[3], avFrame.pBuffer[4], avFrame.pBuffer[5], avFrame.pBuffer[6]);
						char filename[128] = { 0 };
						sprintf(filename, "frame_%d.h264", h264index++);
						writeFile(filename, pBuf, frameInfo->length);
					}*/

					iRet = EasyRTMP_SendPacket(pEasyClient->pusherHandle, &avFrame);
					if (iRet < 0)
					{
						printf("Fail to EasyRTMP_SendH264Packet(I-frame) ...\n");
					}
				}
#endif
			}
		}	
	}
	else if (frameType == EASY_SDK_MEDIA_INFO_FLAG)//回调出媒体信息
	{
		if(pBuf != NULL)
		{
			memcpy(&pEasyClient->mediainfo, pBuf, sizeof(EASY_MEDIA_INFO_T));
			printf("RTSP DESCRIBE Get Media Info: video:%u fps:%u audio:%u channel:%u sampleRate:%u spslen: %d ppslen:%d\n", 
				pEasyClient->mediainfo.u32VideoCodec, pEasyClient->mediainfo.u32VideoFps, 
				pEasyClient->mediainfo.u32AudioCodec, pEasyClient->mediainfo.u32AudioChannel, pEasyClient->mediainfo.u32AudioSamplerate,
				pEasyClient->mediainfo.u32SpsLength, pEasyClient->mediainfo.u32PpsLength);
		}
	}
	else if(frameType == EASY_SDK_EVENT_FRAME_FLAG)
	{
		if(frameInfo->codec == EASY_STREAM_CLIENT_STATE_DISCONNECTED)
		{
			printf("channel source stream disconnected!\n");
		}
		else if (frameInfo->codec == EASY_STREAM_CLIENT_STATE_CONNECTED)
		{
			printf("channel source stream connected!\n");
		}
		else if (frameInfo->codec == EASY_STREAM_CLIENT_STATE_EXIT)
		{
			printf("channel source stream exit!\n");
		}
	}
	else if(frameType == EASY_SDK_SNAP_FRAME_FLAG)
	{
		//char jpgname[128] = { 0 };
		//static int index = 0;
		//sprintf(jpgname, "channel_%d.jpg", index++);
		//FILE* file = fopen(jpgname, "wb+");
		//if (file)
		//{
		//	fwrite(pBuf, 1, frameInfo->length, file);
		//	fclose(file);
		//	file = NULL;
		//}
	}

	return 0;
}



int Easy_APICALL __EasyDownloadCallBack(void *userptr, const char* path)
{
	if (path)
	{
		printf("%s : %s\n", __FUNCTION__, path);
	}
	else
	{
		printf("%s : download failed!\n", __FUNCTION__);
	}
	return 0;
}

int PrintPrompt()
{
	printf("EasyStreamClient.exe -m udp -d rtsp://srcAddr -s file/rtmp/mp4 -f rtmp://dstAddr\n");
	printf("-m: tcp or udp\n");
	printf("-d: rtsp地址、m3u8录像源地址\n");
	//printf("-s: 输出格式，file为H.264(H.265)、rtmp为RTMP推流、mp4为录像合成\n");
	//printf("-f: 输出地址，h.264(H.265)、      rtmp地址、      mp4路径\n");

	printf("-s: 输出格式，file为H.264(H.265)、rtmp为RTMP推流\n");
	printf("-f: 输出地址，h.264(H.265)、      rtmp地址\n");
	printf("-t: 超时时长(秒)\n");

	printf("EasyStreamClient.exe -m tcp -d rtsp://192.168.1.100/ch1 -s file -f 1.h264");

	return 0;
}

int main(int argc, char * argv[])
{
	int size = 338;
	char buff[1024] = { 0 };
	int i = 0;

#ifdef _WIN32
	extern char* optarg;
#endif

	int iret = 0;
	/* iret = EasyStreamClient_Activate(STREAM_CLIENT_KEY);
	if (iret != 0)
	{
		printf("EasyStreamClient Activate error. ret=%d!!!\n", iret);
		return -2;
	}*/


	if (argc < 2)
	{
		PrintPrompt();
		return 0;
	}


	EASY_CLIENT_OBJ_T		easyClientObj;
	memset(&easyClientObj, 0x00, sizeof(EASY_CLIENT_OBJ_T));

	easyClientObj.connectType = EASY_RTP_OVER_TCP;
	easyClientObj.nTimeout = 5;

	for (int i = 0; i < argc; i++)
	{
		if (0 == strncmp(argv[i], "-m", 2))
		{
			if (argc >= i + 1)
			{
				if ((0 == strncmp(argv[i + 1], "udp", 3)) ||
					(0 == strncmp(argv[i + 1], "UDP", 3)))
				{
					easyClientObj.connectType = EASY_RTP_OVER_UDP;
				}
			}
		}
		else if (0 == strncmp(argv[i], "-d", 2))
		{
			if (argc >= i + 1)
				_snprintf(easyClientObj.szURL, sizeof(easyClientObj.szURL), "%s", argv[i + 1]);
		}
		else if (0 == strncmp(argv[i], "-s", 2))
		{
			if (argc >= i + 1)
				_snprintf(easyClientObj.szOutputFormat, sizeof(easyClientObj.szOutputFormat), "%s", argv[i + 1]);
		}
		else if (0 == strncmp(argv[i], "-f", 2))
		{
			if (argc >= i + 1)
				_snprintf(easyClientObj.szOutputAddress, sizeof(easyClientObj.szOutputAddress), "%s", argv[i + 1]);
		}
		else if (0 == strncmp(argv[i], "-t", 2))
		{
			if (argc >= i+1)
				easyClientObj.nTimeout = atoi(argv[i + 1]);
		}
	}

	//memset(easyClientObj.szURL, 0x00, sizeof(easyClientObj.szURL));
	//strcpy(easyClientObj.szURL, "rtsp://124.42.239.202:1554/pag://11.10.150.183:7302:1f9af7693fca48caa13289d5a0501108:1:MAIN:TCP?streamform=rtp");
	//rtsp://172.16.63.35:554/05937331564756400101?DstCode=01&ServiceType=1&ClientType=1&StreamID=1&SrcTP=2&DstTP=1&SrcPP=1&DstPP=1&MediaTransMode=0&BroadcastType=0&SV=0&Token=VQw4ZLs8+9cK0MTgVCTwbDmmNx8MDWeO&DomainCode=630e33ece3344a8bad93faf91bc75805&UserId=2&

	printf("Connect Type: %s\n", easyClientObj.connectType == EASY_RTP_OVER_TCP ? "TCP" : "UDP");
	printf("Connect Address: %s\n", easyClientObj.szURL);
	printf("Output Format: %s\n", easyClientObj.szOutputFormat);
	printf("Output Address: %s\n", easyClientObj.szOutputAddress);
	printf("Timeout: %d\n", easyClientObj.nTimeout);
	
	if ((int)strlen(easyClientObj.szURL) < 7)
	{
		PrintPrompt();
		return 0;
	}
	

	if (0 == strncmp(easyClientObj.szOutputFormat, "rtmp", 4))
	{
		iret = EasyRTMP_Activate(EASY_RTMP_KEY);
		if (iret < 0)
		{
			printf("EasyRTMP_Activate fail: %d\n", iret);
			return 0;
		}
	}
	
	 iret = EasyStreamClient_Activate(STREAM_CLIENT_KEY);
	if (iret < 0)
	{
		printf("EasyStreamClient Activate error. ret=%d!!!\n", iret);
		return 0;
	}

	int userptr = 1;
	EasyStreamClient_Init(&easyClientObj.streamClientHandle, 1);
		
	if (!easyClientObj.streamClientHandle)
	{
		printf("Initial fail.\n");
		return 0;
	}
	
	EasyStreamClient_SetAudioEnable(easyClientObj.streamClientHandle, 1);

	EasyStreamClient_SetCallback(easyClientObj.streamClientHandle, __EasyStreamClientCallBack);


	EasyStreamClient_OpenStream(easyClientObj.streamClientHandle, easyClientObj.szURL, easyClientObj.connectType, (void*)&easyClientObj, 1000, easyClientObj.nTimeout, 1);

	printf("按回车键退出...\n");
	getchar();

	if (easyClientObj.streamClientHandle)
	{
		EasyStreamClient_Deinit(easyClientObj.streamClientHandle);
		easyClientObj.streamClientHandle = NULL;
	}

	if (easyClientObj.pusherHandle)
	{
		EasyRTMP_Release(easyClientObj.pusherHandle);
		easyClientObj.pusherHandle = NULL;
	}

    return 0;
}