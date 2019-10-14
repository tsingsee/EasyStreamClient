
控制台应用程序：EasyStreamClient 项目概述 

EasyStreamClient是一套非常稳定、易用、支持重连的StreamClient工具，以SDK形式提供， 接口调用非常简单，经过多年实战和线上运行打造，支持RTMP推送断线重连、环形缓冲、智能丢帧、 网络事件回调；再也不用像调用live555那样处理整个RTSP的复杂流程，担心内存释放的问题了。 全平台支持（包括Windows/Linux 32&64、ARM各平台、Android、iOS），接口简单且成熟稳定

#运行说明

##Win
运行EasyStreamClient.exe
64位工程程序


	printf("EasyStreamClient.exe -m udp -d rtsp://srcAddr -s file/rtmp/mp4 -f rtmp://dstAddr\n");
	printf("-m: tcp or udp\n");
	printf("-d: rtsp地址、m3u8录像源地址\n");
	//printf("-s: 输出格式，file为H.264(H.265)、rtmp为RTMP推流、mp4为录像合成\n");
	//printf("-f: 输出地址，h.264(H.265)、      rtmp地址、      mp4路径\n");

	printf("-s: 输出格式，file为H.264(H.265)、rtmp为RTMP推流\n");
	printf("-f: 输出地址，h.264(H.265)、      rtmp地址\n");
	printf("-t: 超时时长(秒)\n");

	printf("EasyStreamClient.exe -m tcp -d rtsp://192.168.1.100/ch1 -s file -f 1.h264");



	输入命令示例: -m tcp -d rtsp://admin:12345@112.28.34.31:554/Streaming/Channels/102 -s rtmp -f rtmp://192.168.99.106:10085/hls/test?sign=zlaJjyhZg