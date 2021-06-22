//apptester的功能文件
#include <iostream>
#include <conio.h>
#include <stdlib.h>
#include<string.h>
#include <graphics.h>              // 引用图形库头文件
#include <windows.h>
#include "winsock.h"
#include "stdio.h"
#include "CfgFileParms.h"
#include "function.h"

//#pragma comment (lib,"EasyXa.lib")

#define TOTAL_RECV_WIN_SIZE 1000  // 接收窗口大小
#define MAX_SEND_PACK_SIZE 100    // 一个包的最大数据载荷,不能大于107，大概吧，分分钟死机
using namespace std;

U8* autoSendBuf;        //用来组织发送数据的缓存，大小为MAX_BUFFER_SIZE,可以在这个基础上扩充设计，形成适合的结构，例程中没有使用，只是提醒一下
int printCount = 0; //打印控制
int spin = 1;  //打印动态信息控制

//------华丽的分割线，一些统计用的全局变量------------
int iSndTotal = 0;  //发送数据总量
int iSndTotalCount = 0; //发送数据总次数
int iSndErrorCount = 0;  //发送错误次数
int iRcvTotal = 0;     //接收数据总量
int iRcvTotalCount = 0; //转发数据总次数
int iRcvUnknownCount = 0;  //收到不明来源数据总次数

//**************SPIRIT******************
int OK_to_commu = 0;  //记录当前连接情况
int handshake_case = 0;  //记录握手情况
int handshake_load;  //记录上一步自己产生的个位数
U8* handshake_Recvbuf = (U8*)malloc(2);  //一个字节的存储空间，存储握手时候的数据,第一个数组存的是控制字段，第二个是数据
int handshake_break = 0;  // 记录没有多长时间没有收到消息了，满200（20秒）就会让连接断开;在OK_to_commnu在为1的时候，每次收到相应回应就会重新置零
int handshake_load_updataed = 0;
int commu_mod = 7;  // 7是初始化，表示mod什么都不是，0表示发送模式，1表示接收模式
int dst_IP;         // 记录对方是谁
int DATA_type = 7;  // 记录当前通信的数据类型，7表示什么都不是，0是图，1是文本

// 可视化部分
int r[5][4] = { {30,20,130,60},{170,20,220,60},{260,20,310,60},{30,80,130,120},{170,80,220,120} };//三个按钮的二维数组
MOUSEMSG m;




//接收模式
struct
{
	int ack_Timeout = 0;       //用于延时ack
	int front;                  //绝对前
	int tail;                   //绝对后
	long int front_seq;         //前对应的seq
	long int tail_seq;          //后对应的seq
	int w_p;               //写定位指针
	int err_tail;          //离front最近的失帧的尾端
	int total_win_size;    //总窗大小，常量
	int write_time;        //写窗口次数，用于两帧一个ack
	int win_size;          //剩余窗口
	int file_open;         //文件是否打开
	int start_time;        //记录数据传输开始的时候printCount是多少
	int finish_time;       //记录数据传输结束时候的printfCount是多少
	int dst_IP;            //记录谁在和我通信
	int Info_init = 7;
	int OK_to_fin;       //允许断开

}RecvInfo;                 //接收方的信息集
U8 RecvWin[TOTAL_RECV_WIN_SIZE];
FILE* fp;

//发送模式
struct
{
	int Info_init = 7;
	long int abs_front_seq;     //窗口前端对应的seq
	long int abs_tail_seq;      //窗口尾端对应的seq
	long int ready_seq;         //新数据的seq
	int linear;                 //指数到线性增长的界限，拥塞控制
	int max_pack_size;          //一个数据包数据载荷部分最大字节数
	int sendTimeout = 0;        //使包与包之间的发送有时间间距，sendCounter其实也是这个发送端的自己的计数器


	int winCounter;             //当前窗口超时重发次数
	int winTimeout;             // 记录窗口什么时候达到超时
	long int recv_ack;          //记录上一个ack
	U8 recv_len;                //记录上一个ack包中len
	int recv_win_size;          //记录上一个ack包中的剩余窗口大小
	int resend_switch;          //用于判定是否需要执行重发某包操作
	int file_open;              //用于记录文件有没有被打开
	long int file_size;         //用于记录文件大小
	long int scale;             //用于记录给谁加减了
	int OK_to_fin = 0;            //收到对面的FIN，就是控制字段4，就会开启
	int dst_IP;                 //记录谁在和我通信


}SendInfo;                      //发送方的信息集
U8 DATA_send[MAX_SEND_PACK_SIZE];              //用于缓存发送数据
U8 DATA_resend[MAX_SEND_PACK_SIZE];            //用于缓存重发数据
FILE* fr;
//**************GABRIEL*****************
// https://blog.csdn.net/weixin_43488958/article/details/103892744?utm_medium=distribute.pc_relevant.none-task-blog-baidujs_title-0&spm=1001.2101.3001.4242
// 关于base64 和 图片互转部分（白嫖网站上的）（美其名曰参考）**************************************************
unsigned int image_size_max = 100000; // 感觉有点大，这个数
const char* base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char* base64_encode(const unsigned char* bindata, char* base64, int binlength);
int base64_decode(const char* base64, unsigned char* bindata); // 怎么还在报错？没有定义？？为什么不去康康最后一段代码？？
void code_image(void);
void decode_image(void);




void print_statistics();
void menu();

int SA_DATA(U8* sabuf, U8* DATA, U8 dst, U8 control, long int* seq, long int* ack, U8 len);
void connection_break(void);
void recv_connection_break(void);

//***************重要函数提醒******************************
//名称：InitFunction
//功能：初始化功能面，由main函数在读完配置文件，正式进入驱动机制前调用
//输入：
//输出：
void InitFunction(CCfgFileParms& cfgParms)
{
	int i;
	int retval;

	retval = cfgParms.getValueInt(autoSendTime, (char*)"autoSendTime");
	if (retval == -1 || autoSendTime == 0) {
		autoSendTime = DEFAULT_AUTO_SEND_TIME;
	}
	retval = cfgParms.getValueInt(autoSendSize, (char*)"autoSendSize");
	if (retval == -1 || autoSendSize == 0) {
		autoSendSize = DEFAULT_AUTO_SEND_SIZE;
	}

	autoSendBuf = (char*)malloc(MAX_BUFFER_SIZE);
	if (autoSendBuf == NULL) {
		cout << "内存不够" << endl;
		//这个，计算机也太，退出吧
		exit(0);
	}
	for (i = 0; i < MAX_BUFFER_SIZE; i++) {
		autoSendBuf[i] = 'a'; //初始化数据全为字符'a',只是为了测试
	}
	return;
}
//***************重要函数提醒******************************
//名称：EndFunction
//功能：结束功能面，由main函数在收到exit命令，整个程序退出前调用
//输入：
//输出：
void EndFunction()
{
	if (autoSendBuf != NULL)
		free(autoSendBuf);
	return;
}
//
//
//
//
//
//
//
//
//
//
//***************重要函数提醒******************************
//名称：TimeOut
//功能：本函数被调用时，意味着sBasicTimer中设置的超时时间到了，
//      函数内容可以全部替换为设计者自己的想法
//      例程中实现了几个同时进行功能，供参考
//      1)根据iWorkMode工作模式，判断是否将键盘输入的数据发送，还是自动发送——这个功能其实是应用层的
//        因为scanf会阻塞，导致计时器在等待键盘的时候完全失效，所以使用_kbhit()无阻塞、不间断地在计时的控制下判断键盘状态，这个点Get到没？
//      2)不断刷新打印各种统计值，通过打印控制符的控制，可以始终保持在同一行打印，Get？
//      3)如果工iWorkMode设置为自动发送，就每经过autoSendTime * DEFAULT_TIMER_INTERVAL ms，向接口0发送一次
//输入：时间到了就触发，只能通过全局变量供给输入
//输出：这就是个不断努力干活的老实孩子
void TimeOut() // 100ms触发一次
{
	int iSndRetval;
	int len;
	U8 sendBuf[3];
	int i;

	printCount++;
	handshake_break++;
	if (commu_mod == 1)
		RecvInfo.ack_Timeout++;
	//printf("hb：%d",handshake_break);
	if (commu_mod == 0)
		SendInfo.sendTimeout++;


	if ((OK_to_commu == 1) || (handshake_case == 1) || (handshake_case == 2) || (handshake_case == 3))
	{
		if (SendInfo.OK_to_fin == 1) // 一次数据传输结束
		{
			SendInfo.Info_init = 7;  // 尚未初始化
			DATA_type = 7;
			SendInfo.OK_to_fin = 0;
			commu_mod = 6;
			if (SendInfo.file_open == 1)
			{
				fclose(fr);
				printf("关闭txt文件\n");
				SendInfo.file_open = 0;
			}
		}
		if (RecvInfo.OK_to_fin == 1)
		{
			RecvInfo.OK_to_fin = 0;
			commu_mod = 6;
			RecvInfo.Info_init = 7;
			if (RecvInfo.file_open == 1)
			{
				fclose(fp);
				printf("关闭txt文件\n");
				RecvInfo.finish_time = printCount;
				if (DATA_type == 0) // 数据是图
				{
					decode_image();
					printf("已将数据译码为jpg\n");
					char pic_name[80] = "D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\output.jpg";
					char cmd[100];
					sprintf(cmd, "mspaint %s", pic_name);
					system(cmd); // 显示图片
					/*initgraph(450, 450);
					IMAGE img;
					loadimage(&img, "D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\output.jpg",450,450);
					putimage(0, 0, &img);*/
				}
				if (DATA_type == 1)
				{
					fp = fopen("D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\recv.txt", "r");
					fseek(fp, 0, SEEK_END);
					int f_len = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					U8* s = (U8*)malloc(f_len + 4);
					/*U8* s_copy = (U8*)malloc(f_len + 20);
					sprintf(s_copy+10, " %s", s);
					s_copy[0] = (char)dst_IP;*/
					s[0] = dst_IP + 48;
					s[1] = ':';
					s[2] = ' ';
					if (s == NULL)
						return;
					fread(s + 3, f_len, 1, fp);
					s[f_len + 3] = '\0';
					MessageBox(NULL, s, "消息", MB_OK | MB_ICONINFORMATION);
					printf("\n*********************************************");
					printf("\n%d曰：%s\n", dst_IP, s + 3);
					printf("*********************************************\n");
					free(s);
					fclose(fp);
				}

				//printf("本次通信平均数据传输速率约为%.1f Kbs\n", RecvInfo.front_seq * 8 * 10 / (float)(RecvInfo.finish_time - RecvInfo.start_time - 50) / 1000.0);


			}
			RecvInfo.file_open = 0;
			DATA_type = 7;

		}
		if (SendInfo.winCounter > 5 || handshake_break % 2000 == 0)
			//窗口重发那么多次，还不断？   超时不说话，还不断？
		{
			OK_to_commu = 0;
			handshake_case = 0;  //连接超时断开
			iWorkMode = 0;
			commu_mod = 7;
			SendInfo.Info_init = 7;  // 尚未初始化
			handshake_load_updataed = 0;


			RecvInfo.Info_init = 7;


			if (SendInfo.file_open == 1)
			{
				fclose(fr);
				printf("关闭txt文件\n");
			}
			if (RecvInfo.file_open == 1)
			{
				fclose(fp);
				printf("关闭txt文件\n");
				RecvInfo.finish_time = printCount;
				if (DATA_type == 0) // 数据是图
				{
					decode_image();
					printf("已将数据译码为jpg\n");
				}

				//printf("本次通信平均数据传输速率约为%.1f Kbs\n",RecvInfo.front_seq*8*10/(float)(RecvInfo.finish_time-RecvInfo.start_time-50)/1000.0);


			}
			SendInfo.file_open = 0;
			RecvInfo.file_open = 0;
			DATA_type = 7;
			printf("断了,handshake_case:%d, OK_to_commu:%d\n", handshake_case, OK_to_commu);
			connection_break();
		}
	}

	if (_kbhit()) {
		//键盘有动作，进入菜单模式
		//menu();
		win_case_judge();
		while (_kbhit())
		{
			_getch();
		}

	}



	//发送方的************************************************************************
	//发送方，没有狂发请求包，被允许初始化信息集——进行信息集的初始化，开辟txt，存储将要来发送的数据
	if (commu_mod == 0 && iWorkMode == 0 && SendInfo.Info_init == 1)  //尚未进入过接收模式，且选择了发送照片
	{
		//创建txt接收信息
		SendInfo.Info_init = 0;
		if (DATA_type == 0)
		{
			code_image();  // 译码图片
		}
		fr = fopen("D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\send.txt", "r");  // 打开译码后的txt
		if (fr == NULL)
		{
			printf("send文件读取失败\n");
			iWorkMode = 0;
			SendInfo.Info_init = 1;   // 允许初始化
			return;
		}
		//commu_mod = 0;
		printf("发送方方信息集初始化\n");
		fseek(fr, 0, SEEK_END);
		SendInfo.file_size = ftell(fr);// 获知文件中的数据长度
		printf("数据长度:%ld", SendInfo.file_size);
		fseek(fr, 0, SEEK_SET);
		SendInfo.abs_front_seq = 1;    // 指向DATA的第一个字节（居然从一开始排序！）
		SendInfo.abs_tail_seq = 2;     // 指向当前窗口尾部，这个字节在这个窗口外面，不允许发送！
		SendInfo.ready_seq = 1;        // 指向新数据的头
		SendInfo.file_open = 1;        // 文件打开了
		SendInfo.linear = 400;         // 初始化指-线界限为200字节
		SendInfo.max_pack_size = MAX_SEND_PACK_SIZE;  // 最大数据载荷为100字节
		SendInfo.recv_ack = -1;        // 初始化一个奇奇怪怪的东西
		SendInfo.recv_len = 0;
		SendInfo.recv_win_size = 450;  // 我们希望对面的剩余窗口足够的大
		SendInfo.resend_switch = 0;    // 还没到重发地步
		SendInfo.sendTimeout = 0;      // 到5的倍数就会发一个新数据
		SendInfo.winCounter = 0;       // sendCounter就要重发窗口数据，不可连续三次要求窗口重发，不然就会断开连接
		SendInfo.winTimeout = 0;       // 记录窗口卡住的时长
		SendInfo.scale = 0;            // 用于记录上一个产生加减倍的ack
		SendInfo.OK_to_fin = 0;      //一开始肯定不让断开啊
		handshake_break = 0;           //先别自动断嘛
		//初始化缓冲区
		memset(DATA_send, (char)-1, MAX_SEND_PACK_SIZE);
		memset(DATA_resend, (char)-1, MAX_SEND_PACK_SIZE);


	}
	//狂发申请发送包
	if (iWorkMode == 7 && handshake_break % 50 == 0 && OK_to_commu == 1)
	{
		sendBuf[1] = 32;  // 0010,0000
		sendBuf[2] = 0;
		sendBuf[0] = dst_IP;
		SendtoLower(sendBuf, 3, 0);
		printf("发送图片请求中...\n");
	}
	if (iWorkMode == 3 && handshake_break % 50 == 0 && OK_to_commu == 1)
	{
		sendBuf[1] = 64;  // 0100,0000
		sendBuf[2] = 0;
		sendBuf[0] = dst_IP;
		SendtoLower(sendBuf, 3, 0);
		printf("发送文字请求中...\n");
	}


	//超时ack
	if (OK_to_commu == 1 && RecvInfo.ack_Timeout % 8 == 0 && commu_mod == 1)
	{
		// 超时前再写一次文件
		for (i = 0; i < RecvInfo.total_win_size - 1; i++)
		{
			//下一个为'?'就不能前进了
			if (RecvWin[(RecvInfo.front + 1) % RecvInfo.total_win_size] == (char)-1)
			{
				//printf("front前面一格为'?'\n");
				break;
			}
			//下一个为tail也不能前进
			if ((RecvInfo.front + 1) % RecvInfo.total_win_size == RecvInfo.tail)
			{
				//printf("front前面一格为tail\n");
				break;
			}
			// 前进
			//printf("front前进一格\n");
			RecvInfo.front = (RecvInfo.front + 1) % RecvInfo.total_win_size;
			RecvInfo.front_seq += 1;
			//printf("现在front在%d,写入文件：%c ",RecvInfo.front, *(RecvWin + RecvInfo.front));
			fwrite(RecvWin + RecvInfo.front, sizeof(U8), 1, fp); //写入一个字节
			RecvWin[RecvInfo.front] = (char)-1;  // 窗口初始化
		}

		printf("ackTimeout\n");
		long int seq = RecvInfo.front - RecvInfo.tail - 1;
		if (seq < 0)
			seq = RecvInfo.total_win_size + seq; // 剩余窗口大小
		long int ack = RecvInfo.front_seq + 1;
		int len = RecvInfo.err_tail - RecvInfo.front;
		if (len < 0)
			len = len + RecvInfo.total_win_size;
		if (len > 100)
			len = 100;
		U8* sabuf = (U8*)malloc(19);
		if (sabuf == NULL)
			return;
		SA_DATA(sabuf, NULL, (char)dst_IP, 1, &seq, &ack, (U8)len);
		printf("超时ack包的ack为%ld\n", ack);
		free(sabuf);
	}




	//接受方****************************************************************************
	//接收方信息初始化
	if (OK_to_commu == 1 && commu_mod == 1 && RecvInfo.Info_init == 1) // 进入发送状态后
	{
		RecvInfo.Info_init = 0;        // 防止多次信息集的初始化
		//创建打开文件，接收信息
		fp = fopen("D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\recv.txt", "wb+");
		if (fp == NULL)
		{
			printf("创建文件失败\n");
			commu_mod = 7;            //回去待机
			RecvInfo.Info_init = 1;   //允许重新初始化信息集
			return;
		}
		//初始化信息集
		RecvInfo.file_open = 1;
		//进入接收模式，之后不会进入信息集初始化
		commu_mod = 1;
		handshake_break = 0;
		printf("进入接受模式\n");
		//初始化信息集
		printf("初始化接受信息集中\n");
		RecvInfo.ack_Timeout = 0;
		RecvInfo.err_tail = 1;
		RecvInfo.front = 0;
		RecvInfo.front_seq = 0;
		RecvInfo.tail = 1;
		RecvInfo.tail_seq = 1;
		RecvInfo.total_win_size = TOTAL_RECV_WIN_SIZE;  // 接收窗口大小
		RecvInfo.write_time = 0;
		RecvInfo.w_p = 0;
		RecvInfo.win_size = TOTAL_RECV_WIN_SIZE - 1;
		RecvInfo.start_time = 0;
		RecvInfo.finish_time = 0;
		RecvInfo.OK_to_fin = 0;   // 不让断开
		RecvInfo.start_time = printCount;   //记录开始时刻
		//初始化窗口
		memset(RecvWin, (char)-1, RecvInfo.total_win_size);
		//test
		/*U8* sabuf = (U8*)malloc(118);
		U8  _DATA[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
		long int seq = 1;
		long int ack = 0;
		U8 len = 100;
		if (sabuf == NULL)
			return;
		SA_DATA(sabuf, _DATA, 2, &seq, &ack, len);
		free(sabuf);*/
	}
	//发送方的Timeout
	if (OK_to_commu == 1 && commu_mod == 0 && SendInfo.Info_init == 0) // 握手成功与成为被索取方，信息集已经被初始化的前提下
	{
		int i;
		//窗口超时重发应该写在这里吧
		if (SendInfo.ready_seq >= SendInfo.abs_tail_seq && SendInfo.ready_seq < SendInfo.file_size)  // 这是窗口卡死的情况,只要front能动，winCounter和winTimeout都会清零
		{
			SendInfo.winTimeout++;
			if (SendInfo.winTimeout % 200 == 0)  //窗口超时
			{
				SendInfo.ready_seq = SendInfo.abs_front_seq; // 指针回拨
				printf("指针回拨\n");
				SendInfo.winCounter++;
				SendInfo.winTimeout = 0;
			}
		}

		//是否需要发送“新”数据
		if (SendInfo.ready_seq < SendInfo.abs_tail_seq && SendInfo.sendTimeout % 5 == 0)  // 有新数据发送，而且到了发新包的时刻了
		{
			/*int send_time = 2;
			if (SendInfo.recv_win_size < 4)
				send_time = 4;*/

			if (SendInfo.sendTimeout % 1 == 0)
			{
				//统计发包信息，写缓存
				U8 control = 2; // 这是一个使seq字段生效的包
				long int seq = SendInfo.ready_seq;
				long int ack = 0;
				U8 base = 0;
				U8 len;
				if (SendInfo.abs_tail_seq - SendInfo.ready_seq >= SendInfo.max_pack_size)
					len = SendInfo.max_pack_size;
				else
					len = (U8)(SendInfo.abs_tail_seq - SendInfo.ready_seq);
				if (SendInfo.file_size - SendInfo.ready_seq < 100 && SendInfo.abs_tail_seq >= SendInfo.file_size)
					len = SendInfo.file_size - SendInfo.ready_seq + 1;
				U8* sabuf = (U8*)malloc(len + 19); // 准备好用于发送的sabuf
				if (sabuf == NULL)
					return;
				if (SendInfo.recv_ack > SendInfo.file_size)// || SendInfo.ready_seq > SendInfo.file_size)
				{
					printf("传输完毕，收到的ack超过文件大小，ready在%d\n", SendInfo.ready_seq);
					//为了防止对面苦等，给对面一个提示尽快结束包
					U8* sabuf = (U8*)malloc(19);//能装下一个头部就行
					if (sabuf == NULL) return;
					U8 control = 4;             // 0000,0100
					long int seq = 0;
					long int ack = 0;
					SA_DATA(sabuf, NULL, (U8)dst_IP, control, &seq, &ack, 0);
					free(sabuf);
					return;                              // 后面的工作都是无意义的啊
				}
				//将相应信息写入缓存DATA中
				//先使只读文件指针偏移到ready_seq处
				fseek(fr, SendInfo.ready_seq - 1, SEEK_SET);  // 注意从头开始偏移，而且要减去1，因为seq从1开始
				//然后写入缓存
				fread(DATA_send, (int)len, 1, fr); // 将fr中以一个字节为单位，len那么长的数据写入DATA_send中
				//printf("发包：seq为%ld，数据载荷长度len为:%d,窗口的头在%ld,窗口的尾在%ld,ready在%ld\n", seq, len,SendInfo.abs_front_seq,SendInfo.abs_tail_seq,SendInfo.ready_seq);
				SA_DATA(sabuf, DATA_send, (U8)dst_IP, control, &seq, &ack, len);   //填充包头，可以发了
				free(sabuf);
				SendInfo.ready_seq += (int)len;  // ready指针可以前进了
				handshake_break = 0;
			}


		}

		//然后处理ack包
		//窗口可以移动了
		if (SendInfo.recv_ack > SendInfo.abs_front_seq)
		{
			long int tmp = SendInfo.abs_front_seq;
			SendInfo.abs_front_seq = SendInfo.recv_ack;
			SendInfo.abs_tail_seq += (SendInfo.recv_ack - tmp); // 保证窗口大小不变地移动
			if (SendInfo.abs_tail_seq > SendInfo.file_size)            // 尾部怎么可能超过文件尾部呢
				SendInfo.abs_tail_seq = SendInfo.file_size + 1;
			if (SendInfo.abs_front_seq >= SendInfo.abs_tail_seq)
			{
				printf("传输完毕，头在%ld,尾在%ld,头大于等于尾\n", SendInfo.abs_front_seq, SendInfo.abs_tail_seq);
				//为了防止对面苦等，给对面一个提示尽快结束包
				U8* sabuf = (U8*)malloc(19);//能装下一个头部就行
				if (sabuf == NULL) return;
				U8 control = 4;             // 0000,0100
				long int seq = 0;        // 皮短腿
				long int ack = 0;
				SA_DATA(sabuf, NULL, (U8)dst_IP, control, &seq, &ack, 0);
				free(sabuf);
				SendInfo.OK_to_fin = 1;
				//return;                          // 后面的工作都是无意义的啊
			}
			SendInfo.winTimeout = 0;
			SendInfo.winCounter = 0;
		}
		//处理需要重传的
		if (SendInfo.resend_switch >= 3) //这个ack发了好多次了
		{
			//printf("正在处理重传......\n");
			long int resend_wp = SendInfo.recv_ack;
			U8* sabuf = (U8*)malloc(19 + (SendInfo.recv_len));
			U8 control = 2;
			long int seq = resend_wp;
			long int ack = 0;
			U8 len = SendInfo.recv_len;
			if (len > SendInfo.max_pack_size)
				len = SendInfo.max_pack_size;
			if (resend_wp + len >= SendInfo.ready_seq)
				len = SendInfo.ready_seq - resend_wp; // resend_wp不能超过ready
			if (sabuf == NULL)
				return;
			//将相应信息写入缓存DATA中
			//先使只读文件指针偏移到ready_seq处
			fseek(fr, resend_wp - 1, SEEK_SET);  // 注意从头开始偏移，而且要减去1
			//然后写入缓存
			fread(DATA_resend, (int)len, 1, fr); // 将fr中以一个字节为单位，len那么长的数据写入DATA_send中
			SA_DATA(sabuf, DATA_resend, (U8)dst_IP, control, &seq, &ack, len);   //填充包头，可以发了
			free(sabuf);
			SendInfo.resend_switch = 0;
			//handshake_break = 0;
		}
		//跟据ack包的信息处理窗口大小
		if (SendInfo.abs_tail_seq - SendInfo.abs_front_seq > SendInfo.linear && SendInfo.scale != SendInfo.recv_ack)
			// 如果当前窗口大于指-线界限，增大窗口只能线性，但是减少窗口是倍数的
		{
			SendInfo.scale = SendInfo.recv_ack;
			if (SendInfo.recv_win_size <= 200 || SendInfo.winCounter > 3 || SendInfo.recv_win_size < 4)
			{
				SendInfo.abs_tail_seq = (SendInfo.abs_front_seq + SendInfo.abs_tail_seq) / 2 + 1;   // 超级减倍
				//printf("超级减倍\n");

				if (SendInfo.linear < 10)
					SendInfo.linear = 10;    //不能太小
				else
					SendInfo.linear -= 10;
			}
			if (SendInfo.recv_win_size > 200 && SendInfo.resend_switch < 1)
			{
				//printf("微小加倍\n");
				SendInfo.abs_tail_seq += 10;
			}
		}
		if ((SendInfo.abs_tail_seq - SendInfo.abs_front_seq <= SendInfo.linear && SendInfo.scale != SendInfo.recv_ack))//(SendInfo.abs_tail_seq - SendInfo.abs_front_seq <= SendInfo.linear) // 这时候增大是加倍的
		{
			SendInfo.scale = SendInfo.recv_ack;
			if (SendInfo.recv_win_size > 400 && SendInfo.resend_switch < 1)
			{
				//printf("加倍前头部在%ld，尾部在%ld ", SendInfo.abs_front_seq, SendInfo.abs_tail_seq);
				SendInfo.abs_tail_seq = SendInfo.abs_tail_seq + (SendInfo.abs_tail_seq - SendInfo.abs_front_seq);  // 超级加倍
				//printf("超级加倍,尾部会在%ld\n", SendInfo.abs_tail_seq);
			}
		}


	}





	//握握手，我们都是好朋友**************************************************************
	switch (iWorkMode / 10) // iWorkMode十位 
	{
	case 0:  // 被动方的人道主义精神吧
		if (handshake_case == 2 && handshake_break % 20 == 0) // 被动方的握手包超时重传，这是对收不到33包的超时重传
		{
			sendBuf[2] = handshake_load + (handshake_Recvbuf[1] + 1) * 10; // 把收到的数加一然后放到十位
			sendBuf[1] = 22;  // 控制字段为22，表示发送的是第二次握手包
			sendBuf[0] = dst_IP;
			SendtoLower(sendBuf, 3, 0);
			//printf("\n被动方超时重传，发送的是%d\n",sendBuf[3]);
		}
		break;
	case 1:
		// 选择1，开始建立连接
		switch (OK_to_commu)
		{
		case 1:  //已经建好连接了
			break;
		case 0:  //还没建立连接
			switch (handshake_case)
			{
			case 0:
				if (handshake_load_updataed == 0)
				{
					handshake_load = rand() % 8;  //生成一个[0,7]的int数
					handshake_load_updataed = 1;
				}


				sendBuf[2] = handshake_load;
				sendBuf[1] = 11;  // 控制字段为11，表示发送的是第一次握手包，请求确立连接
				sendBuf[0] = dst_IP;
				SendtoLower(sendBuf, 3, 0);
				handshake_case = 1;
				handshake_break = 0;
				break;
			case 1: // 主动方尽可能争取连上啊
				if (handshake_break % 10 == 0)
				{
					sendBuf[2] = handshake_load;
					sendBuf[1] = 11;  // 控制字段为11，表示发送的是第一次握手包，请求确立连接
					sendBuf[0] = dst_IP;
					SendtoLower(sendBuf, 3, 0);
					//printf("\n握手超时重传,重传的是%d\n",sendBuf[2]);
				}
				break;

			case 2:  //由于被动方的iWorkmode没有设置，所以这个case只有主动方才会运行
				sendBuf[2] = handshake_Recvbuf[1] % 10 + 1;
				sendBuf[1] = 33;  // 控制字段为33，表示发送的是第三次握手包
				sendBuf[0] = dst_IP;
				SendtoLower(sendBuf, 3, 0);
				handshake_case = 3;
				OK_to_commu = 1;  //对于主动建立连接的一方，可以认为连接建立了
				handshake_break = 0;
				printf("主动方：连接OK；发送的是%d\n", sendBuf[2]);
				iWorkMode = 0;
				break;
			}
			break;
		case 3:
			break;
		default:
			break;
		}
		//定时发送, 每间隔autoSendTime * DEFAULT_TIMER_INTERVAL ms 发送一次
		/*if (printCount % autoSendTime == 0) {
			for (i = 0; i < min(autoSendSize, 8); i++) {
				//开头几个字节在26个字母中间轮流，便于观察
				autoSendBuf[i] = 'a' + printCount % 26;
			}

			len = autoSendSize; //每次发送数量
			if (lowerMode[0] == 0) {
				//自动发送模式下，只向接口0发送
				bufSend = (U8*)malloc(len * 8);
				//下层接口是比特流数组
				iSndRetval = ByteArrayToBitArray(bufSend, len * 8, autoSendBuf, len);
				iSndRetval = SendtoLower(bufSend, iSndRetval, 0);

				free(bufSend);
			}
			else {
				//下层接口是字节数组，直接发送
				for (i = 0; i < min(autoSendSize, 8); i++) {
					//开头几个字节在26个字母中间轮流，便于观察
					autoSendBuf[i] = 'a' + printCount % 26;
				}
				iSndRetval = SendtoLower(autoSendBuf, len, 0);
				iSndRetval = iSndRetval * 8; //换算成位
			}
			//发送统计
			if (iSndRetval > 0) {
				iSndTotalCount++;
				iSndTotal += iSndRetval;
			}
			else {
				iSndErrorCount++;
			}
			//看要不要打印数据
			switch (iWorkMode % 10) {
			case 1:
				print_data_bit(autoSendBuf, len, 1);
				break;
			case 2:
				print_data_byte(autoSendBuf, len, 1);
				break;
			case 0:
				break;
			}
		}
		*/
		break;
	}

	/*if (OK_to_commu == 1) // 这是一切通信的前提
	{
		switch (commu_mod)
		{
		case 1: // 接收模式
			break;

		case 0: // 发送模式
			break;

		default:
			break;
		}
	}*/



	//定期打印统计数据
	//print_statistics();

}
//------------华丽的分割线，以下是数据的收发,--------------------------------------------

//***************重要函数提醒******************************
//名称：RecvfromUpper
//功能：本函数被调用时，意味着收到一份高层下发的数据
//      函数内容全部可以替换成设计者自己的
//      例程功能介绍
//         1)通过低层的数据格式参数lowerMode，判断要不要将数据转换成bit流数组发送，发送只发给低层接口0，
//           因为没有任何可供参考的策略，讲道理是应该根据目的地址在多个接口中选择转发的。
//         2)判断iWorkMode，看看是不是需要将发送的数据内容都打印，调试时可以，正式运行时不建议将内容全部打印。
//输入：U8 * buf,高层传进来的数据， int len，数据长度，单位字节
//输出：
void RecvfromUpper(U8* buf, int len)
{
	//应用层不会收到“高层”的数据，都是自己产生
}
//***************重要函数提醒******************************
//名称：RecvfromLower
//功能：本函数被调用时，意味着得到一份从低层实体递交上来的数据
//      函数内容全部可以替换成设计者想要的样子
//      例程功能介绍：
//          1)例程实现了一个简单粗暴不讲道理的策略，所有从接口0送上来的数据都直接转发到接口1，而接口1的数据上交给高层，就是这么任性
//          2)转发和上交前，判断收进来的格式和要发送出去的格式是否相同，否则，在bite流数组和字节流数组之间实现转换
//            注意这些判断并不是来自数据本身的特征，而是来自配置文件，所以配置文件的参数写错了，判断也就会失误
//          3)根据iWorkMode，判断是否需要把数据内容打印
//输入：U8 * buf,低层递交上来的数据， int len，数据长度，单位字节，int ifNo ，低层实体号码，用来区分是哪个低层
//输出：
void RecvfromLower(U8* buf, int len, int ifNo)
{
	int retval = 0;
	if (buf == NULL)
		return;

	if (lowerMode[ifNo] == 0)
	{
		//低层是bit流数组格式，需要转换，才方便打印
		/*bufRecv = (U8*)malloc(len / 8 + 1);
		if (bufRecv == NULL) {
			return;
		}
		//如果接口0是比特数组格式，先转换成字节数组，再向上递交
		retval = BitArrayToByteArray(buf, len, bufRecv, len / 8 + 1);
		retval = len;*/
	}
	else
	{  //这里是APP层，下一层是NET(或LNK)，所以是字节流数组格式
		switch (OK_to_commu)
		{
		case 1:
			if (buf[1] == 22)  // 收到回应了
			{
				printf("handshake_load:%d，buf[2]:%d\n", handshake_load, buf[2]);
				// 判断是否符合握手条件
				if (buf[2] / 10 == handshake_load + 1)  // 判断十位是不是原来发出去的数加1
				{
					handshake_Recvbuf[0] = buf[1];
					handshake_Recvbuf[1] = buf[2];  // 存起来
					handshake_case = 2;  //请求方进入2阶段
					OK_to_commu = 0;
					iWorkMode = 10;
					printf("请求方返回进入2阶段\n");
				}
			}
			break;
		case 0: // 此时连接尚未确立
			switch (handshake_case)
			{
			case 0:
				if (buf[1] == 11)  // 收到对面请求建立连接的握手包
				{
					handshake_Recvbuf[0] = buf[1];
					handshake_Recvbuf[1] = buf[2];  // 存起来
					dst_IP = buf[0];                // 记下谁在和我说话
					// 然后发送22包
					handshake_load = rand() % 8;  //生成一个[0,7]的int数
					U8* sendBuf = (U8*)malloc(3);
					if (sendBuf == NULL)
						return;
					sendBuf[2] = handshake_load + (handshake_Recvbuf[1] + 1) * 10; // 把收到的数加一然后放到十位
					sendBuf[1] = 22;  // 控制字段为22，表示发送的是第二次握手包
					sendBuf[0] = dst_IP;
					printf("被动方进入2阶段,发送的是%d\n", sendBuf[2]);
					SendtoLower(sendBuf, 3, 0);
					free(sendBuf);
					handshake_case = 2;  // 收方进入2阶段
					handshake_break = 0;

				}
				break;
			case 1:  //这是请求方才会进入的阶段
				if (buf[1] == 22)  // 收到回应了
				{
					// 判断是否符合握手条件
					if (buf[2] / 10 == handshake_load + 1)  // 判断十位是不是原来发出去的数加1
					{
						handshake_Recvbuf[0] = buf[1];
						handshake_Recvbuf[1] = buf[2];  // 存起来
						handshake_case = 2;  //请求方进入2阶段
						handshake_break = 0;
						printf("请求方进入2阶段,收到的是%d\n", handshake_Recvbuf[1]);
					}
				}
				break;
			case 2: // 这是收方的2阶段的事
				if (buf[1] == 33) // 收到了请求的33包
				{
					if (buf[2] == handshake_load + 1)
					{
						handshake_case = 3;
						OK_to_commu = 1;  // 收到这个包之后收方可以认为连接确立了
						handshake_break = 0;
						printf("被动方：连接OK\n");
						iWorkMode = 0;
					}
				}
				if (buf[1] == 22)
				{
					// 判断是否符合握手条件
					if (buf[2] / 10 == handshake_load + 1)  // 判断十位是不是原来发出去的数加1
					{
						handshake_Recvbuf[0] = buf[1];
						handshake_Recvbuf[1] = buf[2];  // 存起来
						handshake_case = 2;  //请求方进入2阶段
						handshake_break = 0;
						printf("请求方重传后进入2阶段\n");
					}
				}
				if (buf[1] == 11)
				{
					handshake_Recvbuf[0] = buf[1];
					handshake_Recvbuf[1] = buf[2];  // 存起来
					// 然后发送22包

					U8* sendBuf = (U8*)malloc(3);
					if (sendBuf == NULL)
						return;
					sendBuf[2] = handshake_load + (handshake_Recvbuf[1] + 1) * 10; // 把收到的数加一然后放到十位
					sendBuf[1] = 22;  // 控制字段为22，表示发送的是第二次握手包
					sendBuf[0] = dst_IP;
					//printf("被动方没有成功回应11包而重新进入2阶段,发送的是%d\n", sendBuf[2]);
					SendtoLower(sendBuf, 3, 0);
					free(sendBuf);
					handshake_case = 2;  // 收方进入2阶段
					handshake_break = 0;
				}
				break;
			case 3:
				break;
			default:
				break;
			}
			break;
		}

		retval = len * 8;//换算成位,进行统计
	}

	//Recv方，收到ack*******************************************************
	//在握手成功后，待机模式下，收到索取请求包后，进入发送状态
	if (OK_to_commu == 1 /*&& commu_mod == 7*/ && (buf[1] == 32 || buf[1] == 64) && buf[0] == (U8)dst_IP)    //知道对面即将发送图片，要予以回应，并允许Recv信息集初始化
	{
		U8 control;
		if (buf[1] == 32)
		{
			DATA_type = 0;    // 数据是图
			control = 35;  // 35包为回应包，让对方继续发送图片
		}
		else
		{
			DATA_type = 1;
			control = 67;
		}
		commu_mod = 1;            // 进入接受模式
		RecvInfo.Info_init = 1;  // 允许信息集的初始化
		printf("即将收到消息\n");
		//回应对方，让对方发送
		U8* sabuf = (U8*)malloc(19);

		long int seq = 0;
		long int ack = 0;
		if (sabuf == NULL)
			return;
		SA_DATA(sabuf, NULL, (U8)dst_IP, control, &seq, &ack, 0);
	}




	//Send方收到35包，才会停下发送"发送图片请求包"，就是不允许信息初始化
	if (OK_to_commu == 1 /*&& commu_mod == 7*/ && (buf[1] == 35 || buf[1] == 67) && buf[0] == (U8)dst_IP) // 收到了控制字段35
	{
		printf("即将发送\n");
		SendInfo.Info_init = 1;  // 允许初始化
		iWorkMode = 0;           // 结束狂发
		commu_mod = 0;
	}
	//Send方收到FIN包
	if (OK_to_commu == 1 && (commu_mod == 0 || commu_mod == 6) && buf[1] == 4 && buf[0] == (U8)dst_IP) // 收到了控制字段4
	{
		printf("信息传输完毕......\n");
		SendInfo.OK_to_fin = 1;  // 即将断开
	}
	//Send方收到ACK包
	if (OK_to_commu == 1 && commu_mod == 0 && buf[1] == 1 && SendInfo.OK_to_fin != 1 && buf[0] == (U8)dst_IP)   //成为被索取方后，收到ack包了,处理它
	{
		//解ack包中
		if (*((long int*)(buf + 10)) == SendInfo.recv_ack)  // 这个ack是不是和之前的一样
		{
			SendInfo.resend_switch++;
		}
		SendInfo.recv_ack = *((long int*)(buf + 10));
		SendInfo.recv_win_size = *((long int*)(buf + 1));  //包的seq就是剩余窗口大小
		SendInfo.recv_len = (*(buf + 18));
		//printf("收到ack%ld\n", SendInfo.recv_ack);
	}
	if ((SendInfo.recv_ack >= SendInfo.file_size || SendInfo.abs_front_seq >= SendInfo.abs_tail_seq) && OK_to_commu == 1 && buf[0] == (U8)dst_IP && buf[1] == 1)
	{

		printf("传输已经完毕，别给我发ack了，再发fin\n");
		//为了防止对面苦等，给对面一个提示尽快结束包
		U8* sabuf = (U8*)malloc(19);//能装下一个头部就行
		if (sabuf == NULL) return;
		U8 control = 4;             // 0000,0100
		long int seq = 0;        // 皮短腿
		long int ack = 0;
		SA_DATA(sabuf, NULL, (U8)dst_IP, control, &seq, &ack, 0);
		free(sabuf);
		//return;                          // 后面的工作都是无意义的啊

	}




	//Recv方*************************************************************************
	//握手后，OK_to_commu = 1；成为接收方，commu_mod == 1
	if (OK_to_commu == 1 && (commu_mod == 1 || commu_mod == 6) && buf[1] == 4 && buf[0] == (U8)dst_IP)  //对面的结束包
	{
		//printf("收到结束包，即将断开连接！！！！！！！！！！\n");
		U8* sabuf = (U8*)malloc(19);//能装下一个头部就行
		if (sabuf == NULL) return;
		U8 control = 4;             // 0000,0100
		long int seq = 0;
		long int ack = 0;
		SA_DATA(sabuf, NULL, (U8)dst_IP, control, &seq, &ack, 0);
		free(sabuf);              //接收方方也会发控制字段4，来回应
		RecvInfo.OK_to_fin = 1;
	}
	if (commu_mod == 1 && OK_to_commu == 1 && buf[1] == 2 && (*((long int*)(buf + 2)) >= RecvInfo.front_seq))
		//控制字段为00000010 ，表示这是DATA发送包，而且我们要收的包要在front之后，这样才能写文件
	{
		long int pack_seq;
		U8  DATA_len;
		U8  base = 0;
		long int w_move;
		int i;
		int w_p = RecvInfo.front; // 写指针在front处呆着

		handshake_break = 0;
		// 解包
		//printf("解包中\n");
		pack_seq = *((long int*)(buf + 2));  // 包的seq
		DATA_len = *((U8*)(buf + 18));   //数据长度
		U8* DATA = buf + 19;
		/*for (i = 0; i < DATA_len + 1; i++)
		{
			printf("%c ", DATA[i]);
		}*/

		//tail移动
		long int temp = RecvInfo.tail_seq;
		if (RecvInfo.tail_seq < pack_seq + (DATA_len - base))
		{
			//printf("\npack_seq：%d, (DATA_len - base)：%d，RecvInfo.tail_seq：%d\n", pack_seq, DATA_len - base, RecvInfo.tail_seq);
			for (i = 0; i < pack_seq + (DATA_len - base) - temp; i++)
			{
				//printf("tail移动%d格\n",i+1);
				if ((RecvInfo.tail + 1) % RecvInfo.total_win_size == RecvInfo.front)
				{
					//printf("tail不能穿越front\n");
					break;
				}
				RecvInfo.tail = (RecvInfo.tail + 1) % RecvInfo.total_win_size; // 循环队列
				RecvInfo.tail_seq++;
			}
		}
		//写窗口，不要写窗口外的
		w_move = pack_seq - RecvInfo.front_seq;
		if (w_move < RecvInfo.total_win_size - 1)
		{
			w_p = (w_p + w_move) % RecvInfo.total_win_size; // 循环队列,w_p找到写入位置
			//printf("写窗口中\n");
			for (i = 0; i < (DATA_len - base); i++)
			{
				RecvWin[w_p] = DATA[i];
				//printf("在%d写入了%d ", w_p, RecvWin[w_p]);
				w_p = (w_p + 1) % RecvInfo.total_win_size; // w_p前进1字节
				if (w_p == RecvInfo.tail) // 写位置不能超越tail
				{
					//printf("写入位置不能超越tail\n");
					break;
				}
			}
			RecvInfo.write_time++; // 写操作次数加一，其实没“写”也会加
		}
		//看看窗口前十个
		/*for (i = 0; i < 10; i++)
		{
			printf("%c ", RecvWin[i]);
		}*/


		//err_tail移动
		//printf("\nerr_tail移动中\n");
		for (i = 0; i < RecvInfo.total_win_size - 1; i++)
		{
			if (RecvInfo.err_tail == RecvInfo.tail)//不会超过tail
			{
				//printf("err_tail不会超过tail\n");
				break;
			}
			//当前为'?'但是下一个位置不是'?'
			if (RecvWin[RecvInfo.err_tail] == (char)-1 && RecvWin[(RecvInfo.err_tail + 1) % RecvInfo.total_win_size] != (char)-1)
			{
				//printf("当前为'?'但是下一个位置不是'?'\n");
				break;
			}
			RecvInfo.err_tail = (RecvInfo.err_tail + 1) % RecvInfo.total_win_size;
			//printf("err_tail移动%d格\n",i+1);
		}

		//front移动，同时写文件，移动后的窗口为'?'
		//printf("front移动中\n");
		for (i = 0; i < RecvInfo.total_win_size - 1; i++)
		{
			//下一个为'?'就不能前进了
			if (RecvWin[(RecvInfo.front + 1) % RecvInfo.total_win_size] == (char)-1)
			{
				//printf("front前面一格为'?'\n");
				break;
			}
			//下一个为tail也不能前进
			if ((RecvInfo.front + 1) % RecvInfo.total_win_size == RecvInfo.tail)
			{
				//printf("front前面一格为tail\n");
				break;
			}
			// 前进
			//printf("front前进一格\n");
			RecvInfo.front = (RecvInfo.front + 1) % RecvInfo.total_win_size;
			RecvInfo.front_seq += 1;
			//printf("现在front在%d,写入文件：%c ",RecvInfo.front, *(RecvWin + RecvInfo.front));
			fwrite(RecvWin + RecvInfo.front, sizeof(U8), 1, fp); //写入一个字节
			RecvWin[RecvInfo.front] = (char)-1;  // 窗口初始化
		}
		//发送ack前，得出ack要发出去的信息
		//printf("ack报文初始化中\n");
		long int seq = RecvInfo.front - RecvInfo.tail - 1;
		if (seq < 0)
			seq = RecvInfo.total_win_size + seq;
		seq = seq + 1;
		long int ack = RecvInfo.front_seq + 1;
		int len = RecvInfo.err_tail - RecvInfo.front;
		if (len < 0)
			len = len + RecvInfo.total_win_size;
		if (len > 100)
			len = 100;

		//发送ack
		if (RecvInfo.write_time % 2 == 1) // 奇数就会发
		{
			printf("ack报文发送,ack=%ld,seq(剩余窗口大小)=%ld\n", ack, seq);
			RecvInfo.ack_Timeout = 0;
			U8* sabuf = (U8*)malloc(19);
			if (sabuf == NULL)
				return;
			SA_DATA(sabuf, NULL, (U8)dst_IP, 1, &seq, &ack, (U8)len);
			free(sabuf);
		}







	}
	if (OK_to_commu == 1 && buf[1] == 8 && buf[0] == (U8)dst_IP)  // 收到强断包，8包
	{
		recv_connection_break();
	}











	iRcvTotal += retval;
	iRcvTotalCount++;

	/*if (bufRecv != NULL) {
		free(bufRecv);
	}*/








	//打印
	/*switch (iWorkMode % 10) {
	case 1:
		cout <<endl<< "接收接口 " <<ifNo <<" 数据："<<endl;
		print_data_bit(buf, len, lowerMode[ifNo]);
		break;
	case 2:
		cout << endl << "接收接口 " << ifNo << " 数据：" << endl;
		print_data_byte(buf, len, lowerMode[ifNo]);
		break;
	case 0:
		break;
	}*/
}
//打印统计信息
void print_statistics()
{
	if (printCount % 10 == 0) {
		switch (spin) {
		case 1:
			printf("\r-");
			break;
		case 2:
			printf("\r\\");
			break;
		case 3:
			printf("\r|");
			break;
		case 4:
			printf("\r/");
			spin = 0;
			break;
		}
		cout << "共发送 " << iSndTotal << " 位," << iSndTotalCount << " 次," << "发生 " << iSndErrorCount << " 次错误;";
		cout << " 共接收 " << iRcvTotal << " 位," << iRcvTotalCount << " 次";
		spin++;
	}
}
void menu()
{
	int selection = 666;
	char s_selection[10];
	char dst[10];
	unsigned short port;
	int iSndRetval;
	char kbBuf[100];
	int len;
	U8* bufSend;
	InputBox(s_selection, 10, "请输入命令：1-主动连接对方;2-从键盘输入发送;3-发送给对面一张图;4-断开连接;0-退出;");
	sscanf(s_selection, "%d", &selection);
	if (selection == 2)
	{
		selection = 3;
	}
	else if (selection == 3)
	{
		selection = 7;
	}
	else if (selection == 4)
	{
		selection = 8;
	}
	//发送|打印：[发送控制（0，等待键盘输入；1，自动）][打印控制（0，仅定期打印统计信息；1，按bit流打印数据，2按字节流打印数据]
	/*cout << endl << endl << "设备号:" << strDevID << ",    层次:" << strLayer << ",    实体号:" << strEntity;
	cout << endl << "1-主动连接对方;" << endl << "2-刷新; " << endl << "3-从键盘输入发送; ";
	cout << endl << "4-仅打印统计信息; " << endl << "5-按比特流打印数据内容;" << endl << "6-按字节流打印数据内容;"<<endl << "7-发送给对面一张图;";
	cout << endl << "0-取消" << endl << "请输入数字选择命令：";
	cin >> selection;*/
	switch (selection) {
	case 0:

		break;
	case 1:
		InputBox(dst, 10, "对方设备号是什么？");
		sscanf(dst, "%d", &dst_IP);
		//cout << endl << "对方设备号是什么？ ";
		//cin >> dst_IP;
		iWorkMode = 10 + iWorkMode % 10;
		break;
	case 2:
		iWorkMode = iWorkMode % 10;
		break;
	case 3:
		if (OK_to_commu == 1)
		{
			//cout << "输入字符串(不超过100字符)：";
			//cin >> kbBuf;
			FILE* f_menu = fopen("D://wangke//COMPUTER_NETWORKS//project1_code//send.txt", "wb+");
			InputBox(kbBuf, 100, "请输入语句");
			int kbBuf_len = strlen(kbBuf);
			fwrite(kbBuf, kbBuf_len, 1, f_menu);
			fclose(f_menu);
			DATA_type = 1;
			iWorkMode = 3;
		}


		break;
	case 4:
		iWorkMode = (iWorkMode / 10) * 10 + 0;
		break;
	case 5:
		iWorkMode = (iWorkMode / 10) * 10 + 1;
		break;
	case 6:
		iWorkMode = (iWorkMode / 10) * 10 + 2;
		break;
	case 7:          // 将要发送一张图
		if (OK_to_commu == 1)
		{
			iWorkMode = 7;
			DATA_type = 0;
		}
		break;
	case 8:
		connection_break();
		break;
	default:
		break;
	}

}

int SA_DATA(U8* sabuf, U8* DATA, U8 dst, U8 control, long int* seq, long int* ack, U8 len) // 目的：往sabuf中填东西
{
	if (sabuf == NULL)
		return -1;
	sabuf[0] = dst;

	sabuf[1] = control;

	sabuf[2] = *((char*)seq);
	sabuf[3] = *((char*)seq + 1);
	sabuf[4] = *((char*)seq + 2);
	sabuf[5] = *((char*)seq + 3);
	sabuf[6] = *((char*)seq + 4);
	sabuf[7] = *((char*)seq + 5);
	sabuf[8] = *((char*)seq + 6);
	sabuf[9] = *((char*)seq + 7);

	sabuf[10] = *((char*)ack);
	sabuf[11] = *((char*)ack + 1);
	sabuf[12] = *((char*)ack + 2);
	sabuf[13] = *((char*)ack + 3);
	sabuf[14] = *((char*)ack + 4);
	sabuf[15] = *((char*)ack + 5);
	sabuf[16] = *((char*)ack + 6);
	sabuf[17] = *((char*)ack + 7);

	sabuf[18] = len;             // 头部19字节 

	char base = 0;
	int int_len = len - base;
	for (int i = 0; i < int_len; i++)
	{
		if (DATA == NULL)
			break;
		sabuf[19 + i] = DATA[i]; // DATA是一个数据的缓存，所以从0开始即可，无需seq
	}
	int ret;
	if (DATA != NULL)
	{
		ret = SendtoLower(sabuf, int_len + 19, 0);
	}
	else
		ret = SendtoLower(sabuf, 19, 0); // 没有数据，就发个包头
	return ret;
}













// 关于base64 和 图片互转部分****************************************************
//工作在发送方，输出一个send.txt，方便传输
void code_image(void)  // 将图片编码成Base64格式,但是我要让他输出成一个input.txt文件，所以是void型
{
	FILE* fpointer = NULL;
	FILE* ftxt;
	unsigned int imageSize;        //图片字节数
	char* imageBin;
	char* imageBase64;

	size_t result;
	//char* ret;
	//unsigned int base64StrLength;
	errno_t err;

	ftxt = fopen("D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\send.txt", "w");
	err = fopen_s(&fpointer, "D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\input.jpg", "rb");   //待编码图片
	if (NULL == fpointer)
	{
		printf("failed to open a file\n");
		return;
	}
	//获取图片大小
	fseek(fpointer, 0L, SEEK_END);
	imageSize = ftell(fpointer);
	fseek(fpointer, 0L, SEEK_SET);

	//分配内存存储整个图片
	imageBin = (char*)malloc(sizeof(char) * imageSize);
	if (NULL == imageBin)
	{
		printf("malloc failed");
		return;
	}

	//读取图片
	result = fread(imageBin, 1, imageSize, fpointer);
	if (result != imageSize)
	{
		printf("file reading failed");
		return;
	}

	//分配编码后图片所在buffer
	imageBase64 = (char*)malloc(sizeof(char) * imageSize * 2);//因为编码一般会比源数据大1/3的样子，这里直接申请源文件一倍的空间
	if (NULL == imageBase64)
	{
		printf("malloc failed");
		return;
	}

	//base64编码
	base64_encode((const unsigned char*)imageBin, imageBase64, imageSize);
	fwrite(imageBase64, strlen(imageBase64), 1, ftxt);
	fclose(ftxt);
	fclose(fpointer);
	//base64StrLength = strlen(imageBase64);
	//printf("base64 str length:%d\n", base64StrLength);
	//printf("\ncode_pic->\n%s\n", imageBase64);
	//return imageBase64;
}

//工作在索取方，收到的txt文件为output.txt
void decode_image(void) // 将Base64译码回图片，让他自动将写死的路径下output.txt文件转为图片output.jpg吧
{
	FILE* ftxt = fopen("D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\recv.txt", "r"); // 只读
	long int txt_size;        // base码的长度 
	fseek(ftxt, 0, SEEK_END);
	txt_size = ftell(ftxt);
	fseek(ftxt, 0, SEEK_SET);
	FILE* fpointer = NULL;
	errno_t err_1;
	err_1 = fopen_s(&fpointer, "D:\\wangke\\COMPUTER_NETWORKS\\project1_code\\output.jpg", "wb");
	if (fpointer == NULL)
	{
		printf("faile to open a file\n");
		return;
	}
	char* imageBase64 = (char*)malloc(sizeof(char) * txt_size);
	if (imageBase64 == NULL)
		return;
	fread(imageBase64, txt_size, 1, ftxt);



	//图片字节数
	//char* imageBin;
	char* imageOutput;
	//分配存储解码数据buffer
	imageOutput = (char*)malloc(sizeof(char) * txt_size);//解码后应该和源图片大小一致
	if (imageOutput == NULL) return;
	if (NULL == imageBase64)
	{
		printf("malloc failed");
		//return -1;
	}
	base64_decode(imageBase64, (unsigned char*)imageOutput);

	fwrite(imageOutput, 1, txt_size, fpointer);
	fclose(fpointer);
	fclose(ftxt);

	//free(imageBin);
	free(imageBase64);
	free(imageOutput);


}

char* base64_encode(const unsigned char* bindata, char* base64, int binlength)
{
	int i, j;
	unsigned char current;

	for (i = 0, j = 0; i < binlength; i += 3)
	{
		current = (bindata[i] >> 2);
		current &= (unsigned char)0x3F;
		base64[j++] = base64char[(int)current];

		current = ((unsigned char)(bindata[i] << 4)) & ((unsigned char)0x30);
		if (i + 1 >= binlength)
		{
			base64[j++] = base64char[(int)current];
			base64[j++] = '=';
			base64[j++] = '=';
			break;
		}
		current |= ((unsigned char)(bindata[i + 1] >> 4)) & ((unsigned char)0x0F);
		base64[j++] = base64char[(int)current];

		current = ((unsigned char)(bindata[i + 1] << 2)) & ((unsigned char)0x3C);
		if (i + 2 >= binlength)
		{
			base64[j++] = base64char[(int)current];
			base64[j++] = '=';
			break;
		}
		current |= ((unsigned char)(bindata[i + 2] >> 6)) & ((unsigned char)0x03);
		base64[j++] = base64char[(int)current];

		current = ((unsigned char)bindata[i + 2]) & ((unsigned char)0x3F);
		base64[j++] = base64char[(int)current];
	}
	base64[j] = '\0';
	return base64;
}

int base64_decode(const char* base64, unsigned char* bindata)
{
	int i, j;
	unsigned char k;
	unsigned char temp[4];
	for (i = 0, j = 0; base64[i] != '\0'; i += 4)
	{
		memset(temp, 0xFF, sizeof(temp));
		for (k = 0; k < 64; k++)
		{
			if (base64char[k] == base64[i])
				temp[0] = k;
		}
		for (k = 0; k < 64; k++)
		{
			if (base64char[k] == base64[i + 1])
				temp[1] = k;
		}
		for (k = 0; k < 64; k++)
		{
			if (base64char[k] == base64[i + 2])
				temp[2] = k;
		}
		for (k = 0; k < 64; k++)
		{
			if (base64char[k] == base64[i + 3])
				temp[3] = k;
		}

		bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2)) & 0xFC)) |
			((unsigned char)((unsigned char)(temp[1] >> 4) & 0x03));
		if (base64[i + 2] == '=')
			break;

		bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4)) & 0xF0)) |
			((unsigned char)((unsigned char)(temp[2] >> 2) & 0x0F));
		if (base64[i + 3] == '=')
			break;

		bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6)) & 0xF0)) |
			((unsigned char)(temp[3] & 0x3F));
	}
	return j;
}




// 关于可视化的函数定义
//按钮判断函数
int button_judge(int x, int y)
{
	if (x > r[0][0] && x<r[0][2] && y>r[0][1] && y < r[0][3])return 1;
	if (x > r[1][0] && x<r[1][2] && y>r[1][1] && y < r[1][3])return 2;
	if (x > r[2][0] && x<r[2][2] && y>r[2][1] && y < r[2][3])return 3;
	if (x > r[3][0] && x<r[3][2] && y>r[3][1] && y < r[3][3])return 4;
	return 0;
}
//初始化图像
void init_figure()
{
	int i;
	setrop2(R2_COPYPEN);//当前颜色
	setlinecolor(BLACK);
	setlinestyle(PS_SOLID);//实线
	rectangle(30, 100, 420, 330);//外框线
	setlinestyle(PS_DOT);//点线
	for (i = 30 + 39; i < 420; i += 39)
	{
		line(i, 100, i, 330);//竖直辅助线
	}
	for (i = 100 + 23; i < 330; i += 23)
	{
		line(30, i, 420, i);//水平辅助线
	}
}



void show_main_win(void)
{
	const char DevID[2] = { strDevID[0] ,'\0' };
	//DevID[0] = strDevID[0] - 48;
	//DevID[1] = '\0';

	short win_width, win_height;//定义窗口的宽度和高度
	win_width = 480; win_height = 200;
	initgraph(win_width, win_height);//初始化窗口（黑屏）

	setbkcolor(RGB(250, 250, 250));
	cleardevice();//清屏（取决于背景色）

	RECT R1 = { r[0][0],r[0][1],r[0][2],r[0][3] };
	RECT R2 = { r[1][0],r[1][1],r[1][2],r[1][3] };
	RECT R3 = { r[2][0],r[2][1],r[2][2],r[2][3] };
	RECT R4 = { r[3][0],r[3][1],r[3][2],r[3][3] };
	RECT R5 = { r[4][0],r[4][1],r[4][2],r[4][3] };

	LOGFONT f;//字体样式指针
	gettextstyle(&f);					//获取字体样式
	_tcscpy(f.lfFaceName, _T("宋体"));	//设置字体为宋体
	f.lfQuality = ANTIALIASED_QUALITY;    // 设置输出效果为抗锯齿  
	settextstyle(&f);                     // 设置字体样式
	settextcolor(BLACK);				//BLACK在graphic.h头文件里面被定义为黑色的颜色常量
	drawtext("建连接", &R1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R1内输入文字，水平居中，垂直居中，单行显示
	drawtext("发文字", &R2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R2内输入文字，水平居中，垂直居中，单行显示
	drawtext("发图片", &R3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R3内输入文字，水平居中，垂直居中，单行显示
	drawtext("断连接", &R4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R3内输入文字，水平居中，垂直居中，单行显示
	drawtext(DevID, &R5, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R3内输入文字，水平居中，垂直居中，单行显示
	setlinecolor(BLACK);
	rectangle(r[0][0], r[0][1], r[0][2], r[0][3]);
	rectangle(r[1][0], r[1][1], r[1][2], r[1][3]);
	rectangle(r[2][0], r[2][1], r[2][2], r[2][3]);
	rectangle(r[3][0], r[3][1], r[3][2], r[3][3]);
	rectangle(r[4][0], r[4][1], r[4][2], r[4][3]);

}

int win_case_judge(void)
{

	m = GetMouseMsg();


	switch (m.uMsg)
	{
	case WM_LBUTTONDOWN:
		switch (button_judge(m.x, m.y))
		{
			//复原按钮原型
		case 1:
			char dst[10];
			//int dst;
			InputBox(dst, 10, "请输入连接对象");
			sscanf(dst, "%d", &dst_IP);//将输入字符串依次扫描到全局变量里面
			iWorkMode = 10;
			FlushMouseMsgBuffer();//单击事件后清空鼠标消息
			break;
		case 2:
			if (OK_to_commu == 1)
			{
				char kbBuf[100];
				FILE* f_menu = fopen("D://wangke//COMPUTER_NETWORKS//project1_code//send.txt", "wb+");
				InputBox(kbBuf, 100, "请输入语句");
				int kbBuf_len = strlen(kbBuf);
				fwrite(kbBuf, kbBuf_len, 1, f_menu);
				fclose(f_menu);
				DATA_type = 1;
				iWorkMode = 3;
			}
			FlushMouseMsgBuffer();//单击事件后清空鼠标消息
			break;
		case 3:
			if (OK_to_commu == 1)
			{
				MessageBox(NULL, "即将发送图片", "消息", MB_OK | MB_ICONINFORMATION);
				iWorkMode = 7;
				DATA_type = 0;
			}
			FlushMouseMsgBuffer();//单击事件后清空鼠标消息
			break;
		case 4:
			connection_break();
			FlushMouseMsgBuffer();//单击事件后清空鼠标消息
			break;
		default:
			FlushMouseMsgBuffer();//单击事件后清空鼠标消息
			//printf("\r\n(%d,%d)",m.x,m.y);//打印鼠标坐标，方便调试时确定区域
			break;
		}
		return 0;
		break;
	default:
		FlushMouseMsgBuffer();//单击事件后清空鼠标消息
		break;
	}



	return 0;
}


void txt_in(char* s)
{
	InputBox(s, 100, "请输入语句");
}

void connection_break(void)  // 强行断连
{
	if (OK_to_commu == 1)
	{
		U8 break_pack[2];
		break_pack[0] = (U8)dst_IP;
		break_pack[1] = 8;
		SendtoLower(break_pack, 2, 0);
		SendtoLower(break_pack, 2, 0);
		SendtoLower(break_pack, 2, 0);
		OK_to_commu = 0;
		dst_IP = 0;
		commu_mod = 7;
		iWorkMode = 0;
		handshake_case = 0;
		RecvInfo.Info_init = 7;
		SendInfo.Info_init = 7;
		SendInfo.file_open = 0;
		RecvInfo.file_open = 0;
		DATA_type = 7;
		MessageBox(NULL, "已断开连接", "消息", MB_OK | MB_ICONINFORMATION);
	}
}

void recv_connection_break(void)
{
	OK_to_commu = 0;
	dst_IP = 0;
	commu_mod = 7;
	iWorkMode = 0;
	handshake_case = 0;
	RecvInfo.Info_init = 7;
	SendInfo.Info_init = 7;
	SendInfo.file_open = 0;
	RecvInfo.file_open = 0;
	DATA_type = 7;
	MessageBox(NULL, "对方已断开连接", "消息", MB_OK | MB_ICONINFORMATION);
}