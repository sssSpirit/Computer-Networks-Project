//Nettester 的功能文件
#include <iostream>
#include <conio.h>
#include <cstring>
#include<cmath>
#include<algorithm>
#include "winsock.h"
#include "stdio.h"
#include "CfgFileParms.h"
#include "function.h"
using namespace std;

//以下为重要的变量
U8* sendbuf;        //用来组织发送数据的缓存，大小为MAX_BUFFER_SIZE,可以在这个基础上扩充设计，形成适合的结构，例程中没有使用，只是提醒一下
int printCount = 0; //打印控制
int spin = 0;  //打印动态信息控制

//------华丽的分割线，一些统计用的全局变量------------
int iSndTotal = 0;  //发送数据总量
int iSndTotalCount = 0; //发送数据总次数
int iSndErrorCount = 0;  //发送错误次数
int iRcvForward = 0;     //转发数据总量
int iRcvForwardCount = 0; //转发数据总次数
int iRcvToUpper = 0;      //从低层递交高层数据总量
int iRcvToUpperCount = 0;  //从低层递交高层数据总次数
int iRcvUnknownCount = 0;  //收到不明来源数据总次数

struct list {
	int to, next, ifNo;
}tree_list[10];
int tree_top,K=0;
//打印统计信息
void print_statistics();
void menu();
//***************重要函数提醒******************************
//名称：InitFunction
//功能：初始化功能面，由main函数在读完配置文件，正式进入驱动机制前调用
//输入：
//输出：
void InitFunction(CCfgFileParms& cfgParms)
{
	sendbuf = (char*)malloc(MAX_BUFFER_SIZE);
	if (sendbuf == NULL ) {
		cout << "内存不够" << endl;
		//这个，计算机也太，退出吧
		exit(0);
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
//	if(sendbuf != NULL)
//		free(sendbuf);
	return;
}

//***************重要函数提醒******************************
//名称：TimeOut
//功能：本函数被调用时，意味着sBasicTimer中设置的超时时间到了，
//      函数内容可以全部替换为设计者自己的想法
//      例程中实现了几个同时进行功能，供参考
//      1)根据iWorkMode工作模式，判断是否将键盘输入的数据发送，
//        因为scanf会阻塞，导致计时器在等待键盘的时候完全失效，所以使用_kbhit()无阻塞、不间断地在计时的控制下判断键盘状态，这个点Get到没？
//      2)不断刷新打印各种统计值，通过打印控制符的控制，可以始终保持在同一行打印，Get？
//输入：时间到了就触发，只能通过全局变量供给输入
//输出：这就是个不断努力干活的老实孩子
int bag_size = 10;
int IP2MAC[1000];
int treetable[1000],is_send_ok[1000],is_tree_cre=0;
int tree_fa,tree_de=-1;
int add_bag(U8* buf, int len, int seq, U8* send_buf) {
	
	send_buf[0] = 126;
	int x = strEntity[0] - '0';
	send_buf[1] = buf[0] * 10 + x;
	if (IP2MAC[buf[1]] == 0) {
		send_buf[2] = buf[1] * 10 + 9;
	}
	else send_buf[2] = IP2MAC[buf[1]];
	send_buf[3] = 3;//类型
	send_buf[4] = seq;//序号
	send_buf[5] = buf[3];//剩下的帧长
	send_buf[6] = len;//剩下的字节长度
	send_buf[7] = 0;
	send_buf[8] = 0;
	send_buf[len + bag_size - 1] = 126;//包尾
	for (int i = 0; i < len; i++) {
		send_buf[bag_size-1 + i] = buf[i];
	}
	return len + bag_size;
}
int add_bag_chen(U8* buf, int len, U8* send_buf) {
	send_buf[0] = 126;
	int x = strEntity[0] - '0';
	send_buf[1] = buf[0] * 10 + x;
	if (IP2MAC[buf[1]] == 0) {
		send_buf[2] = buf[1] * 10 + 9;
	}
	else send_buf[2] = IP2MAC[buf[1]];
	send_buf[3] = 3;//类型
	send_buf[4] = 0;//序号
	send_buf[5] = 0;//剩下的帧长
	send_buf[6] = len-1;//剩下的字节长度
	send_buf[7] = 0;
	send_buf[8] = 0;

	//删网关：
	send_buf[len + bag_size - 2] = 126;//包尾
	send_buf[9] = buf[0];
	for (int i = 2; i < len; i++) {
		send_buf[bag_size - 2 + i] = buf[i];
	}
	return len + bag_size-1;
	//不删网关：
	/*send_buf[len + bag_size - 1] = 126;//包尾
	for (int i = 0; i < len; i++) {
		send_buf[bag_size - 1 + i] = buf[i];
	}
	return len + bag_size;*/
}
int add_bag_tongbu(U8* buf, int len, U8* send_buf) {
	send_buf[0] = 126;
	send_buf[len + 1] = 126;
	for (int i = 0; i < len; i++) {
		send_buf[i + 1] = buf[i];
	}
	return len + 2;
}

int add_bag_tree(U8* buf,int cost) {
	int x = strDevID[0] - '0';
	int y = strEntity[0] - '0';
	int my_dr = x * 10 + y;
	buf[0] = 126;
	buf[1] = my_dr;
	buf[2] = 99;
	buf[3] = 2;//类型为2，建立树的帧
	buf[4] = cost;//距离
	buf[5] = 0;//剩下的帧长
	buf[6] = 0;//剩余的字节数
	buf[7] = 0;
	buf[8] = 0;//CRC校验
	buf[bag_size - 1] = 126;
	return bag_size;
}
void add_bag_tree_ack(U8* buf,int ds_dr) {
	int x = strDevID[0] - '0';
	int y = strEntity[0] - '0';
	int my_dr = x * 10 + y;
	buf[0] = 126;
	buf[1] = my_dr;
	buf[2] = ds_dr;
	buf[3] = 1;//建立确认帧
	buf[4] = 0;
	buf[5] = 0;//剩下帧长
	buf[6] = 0;//剩下字节
	buf[7] = 0;
	buf[8] = 0;//CRC
	buf[bag_size - 1] = 126;
}
void add_bag_tree_ack_pc(U8* buf, int ds_dr) {
	int x = strDevID[0] - '0';
	int y = strEntity[0] - '0';
	int my_dr = x * 10 + y;
	buf[0] = 126;
	buf[1] = my_dr;
	buf[2] = ds_dr;
	buf[3] = 1;//建立确认帧
	buf[4] = 1;//是否是pc机的确认帧
	buf[5] = 0;//剩下帧长
	buf[6] = 0;//剩下字节
	buf[7] = 0;
	buf[8] = 0;//CRC
	buf[bag_size - 1] = 126;
}
int add_bag_ack(U8* buf,int dst, int ack) {
	buf[0] = 126;
	int x = strDevID[0] - '0';
	int y = strEntity[0] - '0';
	int my_dr = x * 10 + y;
	buf[1] = my_dr;
	buf[2] = dst;
	buf[3] = 4;//ack确认
	buf[4] = ack;
	buf[5] = 0;
	buf[6] = 0;
	buf[7] = 0;
	buf[8] = 0;
	buf[9] = 126;
	return bag_size;
}
void add_bag_arp(U8* buf, int src, int dst) {
	buf[0] = 126;
	buf[1] = src;
	buf[2] = dst;
	buf[3] = 0;//arp类型
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = 0;
	buf[7] = 0;
	buf[8] = 0;//CRC
	buf[bag_size - 1] = 126;
}
void add_bag_arp_ack(U8* buf, int src, int dst,int net_dst) {
	buf[0] = 126;
	buf[1] = src;
	buf[2] = dst;
	buf[3] = 0;//arp类型
	buf[4] = 1;//ack确认
	buf[5] = net_dst;
	buf[6] = 0;
	buf[7] = 0;
	buf[8] = 0;//CRC
	buf[bag_size - 1] = 126;
}
void win_send(U8* buf, int len,int ifNo) {
	int iSndRetval;
	if (lowerMode[ifNo]== 0) {
		U8* bufSend;
		bufSend = (char*)malloc(len * 8);
		if (bufSend == NULL) {
			return;
		}
		iSndRetval = ByteArrayToBitArray(bufSend, len * 8, buf, len);
		//发送
		iSndRetval = SendtoLower(bufSend, iSndRetval, ifNo); //参数依次为数据缓冲，长度，接口号
	}
	else {
		//下层是字节数组接口，可直接发送
		iSndRetval = SendtoLower(buf, len, 0);
		iSndRetval = iSndRetval * 8;//换算成位
	}
}
void add_bag_net(U8* buf, int len, U8* send_buf) {
	send_buf[0] = 126;

	send_buf[1] = buf[0];
	send_buf[2] = buf[1];
	send_buf[3] = 5;//类型
	send_buf[4] = 0;//序号
	send_buf[5] = 0;//剩下的帧长
	send_buf[6] = len;//剩下的字节长度
	send_buf[7] = 0;
	send_buf[8] = 0;
	send_buf[len + bag_size - 1] = 126;//包尾
	for (int i = 0; i < len; i++) {
		send_buf[bag_size-1 + i] = buf[i];
	}
}

int window_len = 4;
int clk[4], ack[4], win_len[4] = {0,0,0,0};
U8* win_data[4];

int da_fr_up_sz = 450, da_fr_up_fg = 0, da_fr_up_ff = 0, seq_size = 7, seq = 0;
int da_fr_up_len[500];
U8* da_fr_up[500];

void TimeOut()
{
	int time_out = 100;
	printCount++;/*原代码部分
	for (int i = 0; i < window_len; i++) {
		if (win_len[i] == 0) {
			if (da_fr_up_len[da_fr_up_fg] != 0) {//写到这里截至，下步分别判断是交换机的滑动窗口发送还是pc机的窗口，数据处理不一样，交换机转发就行，主机要加包头。而且只有主机才有IPmac的转换
				if (lowerNumber == 1) {
					if (treetable[IP2MAC[da_fr_up[da_fr_up_fg][1]]]) {
						win_data[i] = (U8*)malloc(sizeof(U8*) * (da_fr_up_len[da_fr_up_fg] + bag_size));
						win_len[i] = add_bag(da_fr_up[da_fr_up_fg], da_fr_up_len[da_fr_up_fg], seq++, win_data[i]);
						seq %= seq_size;
						win_send(win_data[i], win_len[i], treetable[IP2MAC[da_fr_up[da_fr_up_fg][1]]]-1);

						free(da_fr_up[da_fr_up_fg]);
						da_fr_up_len[da_fr_up_fg] = 0;
						da_fr_up_fg = (da_fr_up_fg + 1) % da_fr_up_sz;
			//			printf("窗口发送数据：");
			//			print_data_byte(win_data[i], win_len[i], 1);
					}
					else {
						U8* buf_arp;
						int buf_arp_len = bag_size;
						buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
						int x = strDevID[0] - '0';
						int y = strEntity[0] - '0';
						int my_dr = x * 10 + y;
						add_bag_arp(buf_arp, my_dr, da_fr_up[da_fr_up_fg][1] * 10 + 9);
						if (lowerNumber == 1) {//主机的代码
							if (lowerMode[0] == 0) {
								U8* buf_arp_send;
								int buf_arp_send_len = buf_arp_len * 8;
								buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
								ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
								printf("主机发数据了！");
								SendtoLower(buf_arp_send, buf_arp_send_len, 0);
							}
							else {
								SendtoLower(buf_arp, buf_arp_len, 0);
							}
						}
					}
					clk[i] = printCount;
				}/*
				else if (lowerNumber > 1) {
					printf("交换机开始发送数据包");
					printf("%d#%d!%d\n", treetable[da_fr_up[da_fr_up_fg][1]], da_fr_up[da_fr_up_fg][0], da_fr_up[da_fr_up_fg][1]);
					if (treetable[da_fr_up[da_fr_up_fg][1]]) {

						win_data[i] = (U8*)malloc(sizeof(U8*) * (da_fr_up_len[da_fr_up_fg] + 2));
						win_len[i] = add_bag_tongbu(da_fr_up[da_fr_up_fg], da_fr_up_len[da_fr_up_fg], win_data[i]);

						win_send(win_data[i], win_len[i], treetable[da_fr_up[da_fr_up_fg][1]]-1);
			//			seq %= seq_size;
						free(da_fr_up[da_fr_up_fg]);
						da_fr_up_len[da_fr_up_fg] = 0;
						da_fr_up_fg = (da_fr_up_fg + 1) % da_fr_up_sz;
			//			printf("窗口发送数据：");
			//			print_data_byte(win_data[i], win_len[i], 1);
					}
					clk[i] = printCount;
				}
				
			}
		}
		if (printCount - clk[i] > time_out) {
			if (win_len[i] != 0) {
		//		printf("超时重传：\n");
		//		print_data_byte(win_data[i], win_len[i], 1);
				win_send(win_data[i], win_len[i],treetable[win_data[i][2]]-1);
				clk[i] = printCount;
			}
		}
	}
	*///超时重传结束
	if (_kbhit()) {
		//键盘有动作，进入菜单模式
		menu();
	}

	print_statistics();
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
	printf("\n收到上层数据\n");
//	print_data_byte(buf, len, 1);
//	printf("!!%d %d\n", buf[1],(unsigned char)buf[1]);
//	while (1)printf("2 ");
	if (((unsigned char)buf[1] == 240 || (unsigned char)buf[1] == 241)) {
		U8* buf_send;
		int buf_send_len = len + bag_size;
		buf_send = (U8*)malloc(sizeof(U8) * buf_send_len);
		printf("\n收到上层%d包，向下转发,端口为：0",(unsigned char)buf[1]);
		add_bag_net(buf, len, buf_send);
	//	print_data_byte(buf_send, bag_size, 1);
		win_send(buf_send, buf_send_len,0);
	}
	else if ((unsigned char)buf[2] == 225) {
		U8* buf_send;
		int buf_send_len = bag_size;
		buf_send = (U8*)malloc(sizeof(U8) * buf_send_len);
		int x = strDevID[0] - '0';
		int y = strEntity[0] - '0';
		int my_dr = x * 10 + y;
		printf("\n收到上层arp确认，向下转发,端口为：0");
		add_bag_arp_ack(buf_send, my_dr, buf[1],x);
	//	print_data_byte(buf_send, bag_size, 1);
		win_send(buf_send, buf_send_len, 0);
	}
	else if (treetable[IP2MAC[buf[1]]]) {
		
		U8* buf_data;
		int buf_data_len = len + bag_size;//删网关就-1
		buf_data = (U8*)malloc(sizeof(U8) * buf_data_len);
		buf_data_len=add_bag_chen(buf, len, buf_data);
		printf("\n转发上层的数据,端口为：0");
	//	print_data_byte(buf_data, bag_size,1);
		win_send(buf_data, buf_data_len, treetable[IP2MAC[buf[1]]] - 1);
		//			printf("窗口发送数据：");
		//			print_data_byte(win_data[i], win_len[i], 1);

	}
	else {
		U8* buf_arp;
		int buf_arp_len = bag_size;
		buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
		int x = strDevID[0] - '0';
		int y = strEntity[0] - '0';
		int my_dr = x * 10 + y;
		add_bag_arp(buf_arp, my_dr, buf[1] * 10 + 9);
		if (lowerNumber == 1) {//主机的代码
			if (lowerMode[0] == 0) {
				U8* buf_arp_send;
				int buf_arp_send_len = buf_arp_len * 8;
				buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
				ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
				printf("\n发送arp包,端口为：0");
		//		print_data_byte(buf_arp, bag_size,1);
				SendtoLower(buf_arp_send, buf_arp_send_len, 0);
			}
			else {
				SendtoLower(buf_arp, buf_arp_len, 0);
			}
		}
	}

	/*原代码部分
	da_fr_up[da_fr_up_ff] = (U8*)malloc(len * sizeof(U8));//缓存数据应该在da_fr_up_ff里，取的时候从fg里取
	for (int i = 0; i < len; i++) {
		da_fr_up[da_fr_up_ff][i] = buf[i];
	}
	da_fr_up_len[da_fr_up_ff] = len;
	printf("从上层收到数据：");
	print_data_byte(da_fr_up[da_fr_up_ff], da_fr_up_len[da_fr_up_ff], 1);
	da_fr_up_ff = (da_fr_up_ff + 1) % da_fr_up_sz;*/


//	printf("%d\n")
	/*
	if (!IP2MAC[buf[1]]) {
		U8* buf_arp;
		int buf_arp_len=bag_size;
		buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
		int x = strDevID[0] - '0';
		int y = strEntity[0] - '0';
		int my_dr = x * 10 + y;
		add_bag_arp(buf_arp, my_dr, buf[1] * 10 + 9);
		print_data_byte(buf_arp, buf_arp_len, 1);
		if (lowerNumber == 1) {//主机的代码
			if (lowerMode[0] == 0) {
				U8* buf_arp_send;
				int buf_arp_send_len = buf_arp_len * 8;
				printf("主机：arp包为：");
				print_data_byte(buf_arp, buf_arp_len, 1);
				buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
				ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
				SendtoLower(buf_arp_send, buf_arp_send_len, 0);
			}
			else {
				SendtoLower(buf_arp, buf_arp_len, 0);
			}
		}
		else {
			//路由器添加的代码
		}
	}*/

/*
	if (!is_tree_cre) {
		U8* cre_buf;
		cre_buf = (U8*)malloc(sizeof(U8) * bag_size);
		add_bag_tree(cre_buf, 0);
		tree_fa = my_dr;
		lowerNumber
	}
	if (!IP2MAC[buf[1]]) {
		if (is_tree_cre) {

		}
	}
*/
	/*
	int iSndRetval;
	U8* bufSend = NULL;
	//是高层数据，只从接口0发出去,高层接口默认都是字节流数据格式
	if (lowerMode[0] == 0) {
		//接口0的模式为bit数组，先转换成bit数组，放到bufSend里
		bufSend = (char*)malloc(len * 8);
		if (bufSend == NULL) {
			return;
		}
		iSndRetval = ByteArrayToBitArray(bufSend, len * 8, buf, len);
		//发送
		iSndRetval = SendtoLower(bufSend, iSndRetval, 0); //参数依次为数据缓冲，长度，接口号
	}
	else {
		//下层是字节数组接口，可直接发送
		iSndRetval = SendtoLower(buf, len, 0);
		iSndRetval = iSndRetval * 8;//换算成位
	}
	//如果考虑设计停等协议等重传协议，这份数据需要缓冲起来，应该另外申请空间，把buf或bufSend的内容保存起来，以备重传
	if (bufSend != NULL) {
		//保存bufSend内容，CODES NEED HERE

		//本例程没有设计重传协议，不需要保存数据，所以将空间释放
		free(bufSend);
	}
	else {
		//保存buf内容，CODES NEED HERE

		//本例程没有设计重传协议，不需要保存数据，buf是输入参数，也不需要释放空间

	}

	//统计
	if (iSndRetval <= 0) {
		iSndErrorCount++;
	}
	else {
		iSndTotal += iSndRetval;
		iSndTotalCount++;
	}
	//printf("\n收到上层数据 %d 位，发送到接口0\n", retval * 8);
	//打印
	switch (iWorkMode % 10) {
	case 1:
		cout << endl << "高层要求向接口 " << 0 << " 发送数据：" << endl;
		print_data_bit(buf, len, 1);
		break;
	case 2:
		cout << endl << "高层要求向接口 " << 0 << " 发送数据：" << endl;
		print_data_byte(buf, len, 1);
		break;
	case 0:
		break;
	}
*/
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
void adde(int to, int ifNo) {
	tree_list[++K].next = tree_top;
	tree_list[K].to = to;
	tree_list[K].ifNo = ifNo;
	tree_top = K;
	treetable[to] = ifNo+1;//ifno+1!!!!!
	is_tree_cre = 1;
	is_send_ok[ifNo] = 1;
	printf("\n端口可以发送情况：");
	for (int i = 0; i < lowerNumber; i++) {
		printf("%d:%d\n", i, is_send_ok[i]);
	}
	printf("\n地址转发表：\n");
	for (int i = 0; i <= 99; i++) {
		if (treetable[i]) {
			printf("%02x：%d ", (unsigned char)i,treetable[i]-1);
		}
	}
}/*
U8 crc[17] = { 1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1 };
int crc_len = 17;
void cala_crc(U8* buf, int len) {
	for (int i = 0; i < len; i++) {
		if (buf[i] != 0 && i + crc_len <= len) {
			for (int j = 0; j < crc_len; j++) {
				buf[i + j] ^= crc[j];
			}
		}
	}
}*/

int tongbu(U8* buf, int len) {
	U8 s[8] = { 0,1,1,1,1,1,1,0 };
	int tot = 0;

	int ff = 0;
	int fg = 0;
	for (int i = 0; i + 7 < len; i++) {
		int flag = 0;
		for (int j = 0; j <= 7; j++) {
			if (buf[i + j] != s[j]) {
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			i = i + 8;
			fg = 1;
			U8* bag_bit;
			U8* bag_byte;
			int bag_bit_len = (bag_size - 2) * 8,bag_byte_len= bag_size - 2;
			bag_bit = (U8*)malloc(sizeof(U8) * bag_bit_len);
			bag_byte = (U8*)malloc(sizeof(U8) * bag_byte_len);
			for (int j = 0; j < bag_bit_len; j++) {
				bag_bit[j] = buf[i + j];
				buf[tot++] = buf[i + j];
			}
			BitArrayToByteArray(bag_bit, bag_bit_len, bag_byte,bag_byte_len );
		//	printf("收到包，端口为：包头为：");
		//	print_data_byte(bag_byte, bag_byte_len, 1);
			i = i + bag_bit_len;
			if (bag_byte[5]<0 || bag_byte[5] * 8 + bag_bit_len>len) {
				return 0;
			}
			for (int j = 0; j < bag_byte[5]*8; j++) {
				buf[tot++] = buf[i + j];
			}
			
			U8* data_bit;
			int data_bit_len = bag_byte[5] * 8 + bag_bit_len;
			data_bit = (U8*)malloc(sizeof(U8) * data_bit_len);
			for (int j = 0; j < data_bit_len; j++) {
				if (j < (bag_size-4) * 8) {
					data_bit[j] = buf[j];
				}
				else if ((bag_size - 4) * 8 <= j && j < data_bit_len-16) {
					data_bit[j] = buf[j + 16];
				}
				else {
					data_bit[j] = buf[j - (data_bit_len - 16) + (bag_size - 4) * 8];
				}
			}
			cala_crc(data_bit, data_bit_len);
			for (int j = 0; j < 16; j++) {
				if (data_bit[data_bit_len - 16 + j] != 0) {
					return 0;
				}
			}
			return tot;
		}
	}
//	printf("帧同步出错");
	return 0;
}
int ACK = 0;
void RecvfromLower(U8* buf, int len, int ifNo)
{
	int iSndRetval;
	U8* bufSend = NULL;
/*	if (ifNo == 0 && lowerNumber > 1) {
		//从接口0收到的数据，直接转发到接口1 —— 仅仅用于测试
		if (lowerMode[0] == lowerMode[1]) {
			//接口0和1的数据格式相同，直接转发
			iSndRetval = SendtoLower(buf, len, 1);
			if (lowerMode[0] == 1) {
				iSndRetval = iSndRetval * 8;//如果接口格式为bit数组，统一换算成位，完成统计
			}
		}
		else {
			//接口0与接口1的数据格式不同，需要转换后，再发送
			if (lowerMode[0] == 1) {
				//从接口0到接口1，接口0是字节数组，接口1是比特数组，需要扩大8倍转换
				bufSend = (U8*)malloc(len * 8);
				if (bufSend == NULL) {
					cout << "内存空间不够，导致数据没有被处理" << endl;
					return;
				}
				//byte to bit
				iSndRetval = ByteArrayToBitArray(bufSend, len * 8, buf, len);
				iSndRetval = SendtoLower(bufSend, iSndRetval, 1);
			}
			else {
				//从接口0到接口1，接口0是比特数组，接口1是字节数组，需要缩小八分之一转换
				bufSend = (U8*)malloc(len / 8 + 1);
				if (bufSend == NULL) {
					cout << "内存空间不够，导致数据没有被处理" << endl;
					return;
				}
				//bit to byte
				iSndRetval = BitArrayToByteArray(buf, len, bufSend, len / 8 + 1);
				iSndRetval = SendtoLower(bufSend, iSndRetval, 1);
				
				iSndRetval = iSndRetval * 8;//换算成位，做统计

			}
		}
		//统计
		if (iSndRetval <= 0) {
			iSndErrorCount++;
		}
		else {
			iRcvForward += iSndRetval;
			iRcvForwardCount++;
		}
	}
	else {*/
		//非接口0的数据，或者低层只有1个接口的数据，都向上递交
		if (lowerMode[ifNo] == 0) {
			int send_len;
			U8* buf_send;
			 
			send_len = tongbu(buf, len);
			
			if (send_len == 0) {
				printf("\n帧出错\n");
				return;
			}
			buf_send = (U8*)malloc(sizeof(U8) * send_len);/////////////////////////////////////??????????????????????????????????????

			for (int i = 0; i < send_len; i++)buf_send[i] = buf[i];
			
			//如果接口0是比特数组格式，高层默认是字节数组，先转换成字节数组，再向上递交
			bufSend = (U8*)malloc(send_len / 8+1);
			if (bufSend == NULL) {
				cout << "内存空间不够，导致数据没有被处理" << endl;
				return;
			}
			iSndRetval = BitArrayToByteArray(buf_send, send_len, bufSend, send_len / 8+1);
			int bufSend_len=iSndRetval;
		//	printf("\n收到包，端口为：%d",ifNo);
		//	print_data_byte(bufSend, bag_size - 2, 1);
		//	printf("收到下层数据：");
		//	print_data_byte(bufSend, iSndRetval, 1);
			if ((unsigned char)bufSend[1] == 240 || (unsigned char)bufSend[1] == 241) {
				if (lowerNumber == 1) {
					bufSend = bufSend + (bag_size - 2);
					iSndRetval = iSndRetval - (bag_size - 2);
					printf("\n收到%d包,向上递交帧为",(unsigned char)bufSend[1]);
				//	print_data_byte(bufSend, iSndRetval, 1);
					iSndRetval = SendtoUpper(bufSend, iSndRetval);
					iSndRetval = iSndRetval * 8;//换算成位,进行统计
				}
				else if (lowerNumber > 1) {
					if (!is_tree_cre) {
						U8* cre_buf;
						int cre_buf_len = bag_size;
						cre_buf = (U8*)malloc(sizeof(U8) * bag_size);
						add_bag_tree(cre_buf, 0);
						is_tree_cre = 1;
						int x = strDevID[0] - '0';
						int y = strEntity[0] - '0';
						int my_dr = x * 10 + y;
						tree_fa = my_dr;
						tree_de = 0;
						for (int i = 0; i < lowerNumber; i++) {
							if (lowerMode[i] == 0) {
								printf("\n发送建立生成树的包,端口为：%d", i);
							//	print_data_byte(cre_buf, cre_buf_len, 1);
								U8* cre_buf_send;
								int cre_buf_send_len = cre_buf_len * 8;
								cre_buf_send = (U8*)malloc(sizeof(U8) * cre_buf_send_len);
								ByteArrayToBitArray(cre_buf_send, cre_buf_send_len, cre_buf, cre_buf_len);
								SendtoLower(cre_buf_send, cre_buf_send_len, i);
							}
							else {
								SendtoLower(cre_buf, cre_buf_len, i);
							}
						}
					}
					else 
					for (int i = 0; i < lowerNumber; i++) {
						if (i != ifNo && is_send_ok[i]) {//源代码：加  
							U8* buf_arp;
							int buf_arp_len = bufSend_len+2;
							buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
							add_bag_tongbu(bufSend, bufSend_len, buf_arp);
							printf("\n交换机转发%d包,端口为：%d",(unsigned char)bufSend[1], i);
						//	print_data_byte(buf_arp, buf_arp_len, 1);
							if (lowerMode[i] == 0) {
								U8* buf_arp_send;
								int buf_arp_send_len = buf_arp_len * 8;
								buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
								ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
								SendtoLower(buf_arp_send, buf_arp_send_len, i);
							}
							else {
								SendtoLower(buf_arp, buf_arp_len, i);
							}
						}
					}
				}
			}
			else if (bufSend[2] == 0) {//arp
				if (lowerNumber == 1) {
					if (bufSend[3] == 0) {//询问arp
						printf("\n收到arp包,端口为：0");
						IP2MAC[bufSend[0] / 10] = bufSend[0];
						treetable[bufSend[0]] = ifNo+1;
						U8* buf_arp;
						int buf_arp_len=3;
						buf_arp = (U8*)malloc(sizeof(U8) * 3);
						if (buf_arp == NULL)return;
						buf_arp[0] = bufSend[1] / 10;
						buf_arp[1] = bufSend[0];
						buf_arp[2] = 224;
						SendtoUpper(buf_arp, 3);
						free(buf_arp);
						/*
						if (bufSend[1] / 10 == strDevID[0] - '0') {
							U8* buf_arp;
							int buf_arp_len = bag_size;
							buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
							int x = strDevID[0] - '0';
							int y = strEntity[0] - '0';
							int my_dr = x * 10 + y;
							add_bag_arp_ack(buf_arp, my_dr, bufSend[0]);
							if (lowerMode[ifNo] == 0) {
								U8* buf_arp_send;
								int buf_arp_send_len = buf_arp_len * 8;
								buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
								ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
								printf("\n发送确认arp帧,端口为：%d，包头为：",ifNo);
								print_data_byte(buf_arp, buf_arp_len, 1);
								SendtoLower(buf_arp_send, buf_arp_send_len, ifNo);
							}
							else {
								SendtoLower(buf_arp, buf_arp_len, ifNo);
							}
						}*/
					}
					else if (bufSend[3] == 1) {//确认arp
						printf("\n收到arp确认包,端口为：0");
						IP2MAC[bufSend[4]] = bufSend[0];
						treetable[bufSend[0]] = ifNo+1;
					}
					
				}
				else if (lowerNumber > 1) {
					printf("\n收到arp包,端口为：%d",ifNo);
			//		if (treetable[bufSend[0]]) {// 1,0表示没有值,.....判断生成树上有没有这条边，没有的话，就不管这个广播的包
						treetable[bufSend[0]] = ifNo+1;
						if (!is_tree_cre) {
							U8* cre_buf;
							int cre_buf_len = bag_size;
							cre_buf = (U8*)malloc(sizeof(U8) * bag_size);
							add_bag_tree(cre_buf, 0);
							is_tree_cre = 1;
							int x = strDevID[0] - '0';
							int y = strEntity[0] - '0';
							int my_dr = x * 10 + y;
							tree_fa = my_dr;
							tree_de = 0;
							for (int i = 0; i < lowerNumber; i++) {
								if (lowerMode[i] == 0) {
									printf("\n发送建立生成树的包,端口为：%d", i);
								//	print_data_byte(cre_buf, cre_buf_len, 1);
									U8* cre_buf_send;
									int cre_buf_send_len = cre_buf_len * 8;
									cre_buf_send = (U8*)malloc(sizeof(U8) * cre_buf_send_len);
									ByteArrayToBitArray(cre_buf_send, cre_buf_send_len, cre_buf, cre_buf_len);
									SendtoLower(cre_buf_send, cre_buf_send_len, i);
								}
								else {
									SendtoLower(cre_buf, cre_buf_len, i);
								}
							}
						}
						else {
							if (treetable[bufSend[1]]) {
								
								int dk = treetable[bufSend[1]] - 1;
								U8* buf_arp;
								int buf_arp_len = bufSend_len + 2;
								buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
								add_bag_tongbu(bufSend, bufSend_len,buf_arp);
								if (lowerMode[dk] == 0) {
									U8* buf_arp_send;
									int buf_arp_send_len = buf_arp_len * 8;
									buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
									printf("\n转发arp包,端口为：%d",dk);
								//	print_data_byte(buf_arp, buf_arp_len, 1);
									ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
									SendtoLower(buf_arp_send, buf_arp_send_len, dk);
									
								}
								else {
									SendtoLower(buf_arp, buf_arp_len, dk);
								}
							}
							else {
								for (int i = 0; i < lowerNumber; i++) {
									if (i != ifNo && is_send_ok[i]) {//源代码：加  
										U8* buf_arp;
										int buf_arp_len = bufSend_len+2;
										buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
										add_bag_tongbu(bufSend, bufSend_len, buf_arp);
										printf("\n转发arp包,端口为：%d",i);
									//	print_data_byte(buf_arp, buf_arp_len, 1);
										if (lowerMode[i] == 0) {
											U8* buf_arp_send;
											int buf_arp_send_len = buf_arp_len * 8;
											buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
											ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
											SendtoLower(buf_arp_send, buf_arp_send_len, i);
										}
										else {
											SendtoLower(buf_arp, buf_arp_len, i);
										}
									}
								}
							}
						}
			//		}
				}
				
			}
			else if (bufSend[2] == 1) {//生成树建立确认帧
				if (bufSend[3] == 0) {
					adde(bufSend[0], ifNo);
				}
				else if (bufSend[3] == 1) {
					treetable[bufSend[0]] = ifNo+1;//ifNo+1!!!!!!
					is_send_ok[ifNo] = 1;
					printf("\n端口可以发送情况：");
					for (int i = 0; i < lowerNumber; i++) {
						printf("\n%d:%d\n", i, is_send_ok[i]);
					}
					printf("\n地址转发表：\n");
					for (int i = 0; i <= 99; i++) {
						if (treetable[i]) {
							printf("%02x：%d ", (unsigned char)i, treetable[i] - 1);
						}
					}
				}
			}
			else if (bufSend[2] == 2) {//建立生成树帧
				is_tree_cre = 1;
				printf("收到生成树建立包,端口为：%d",ifNo);
				if (lowerNumber == 1) {//PC收到生成树建立请求
					U8* buf_ack_pc;
					int buf_ack_pc_len=bag_size;
					buf_ack_pc = (U8*)malloc(sizeof(U8) * buf_ack_pc_len);
					add_bag_tree_ack_pc(buf_ack_pc,bufSend[0]);
					if (lowerMode[ifNo] == 0) {
						U8* buf_ack_pc_send;
						int buf_ack_pc_send_len = buf_ack_pc_len * 8;
						buf_ack_pc_send = (U8*)malloc(sizeof(U8) * buf_ack_pc_send_len);
						printf("发送生成树确认包,端口为：%d", ifNo);
						ByteArrayToBitArray(buf_ack_pc_send, buf_ack_pc_send_len, buf_ack_pc, buf_ack_pc_len);
						SendtoLower(buf_ack_pc_send, buf_ack_pc_send_len, ifNo);
					}
				}
				else if (lowerNumber > 1) {//交换机收到生成树建立请求
					if (tree_de == -1) {
						tree_de = bufSend[3] + 1;
						tree_fa = bufSend[0];
						
						adde(bufSend[0], ifNo);
						U8* tree_ack;
						int tree_ack_len = bag_size;
						tree_ack = (U8*)malloc(sizeof(U8) * bag_size);//生成建立确认帧
						add_bag_tree_ack(tree_ack, bufSend[0]);
						if (lowerMode[ifNo] == 0) {
							U8* tree_ack_send;
							int tree_ack_send_len = 8 * tree_ack_len;
							tree_ack_send = (U8*)malloc(sizeof(U8) * tree_ack_send_len);
							printf("发送生成树确认包,端口为：%d", ifNo);
							ByteArrayToBitArray(tree_ack_send, tree_ack_send_len, tree_ack, tree_ack_len);
							SendtoLower(tree_ack_send, tree_ack_send_len, ifNo);
						}
						else {
							SendtoLower(tree_ack, tree_ack_len, ifNo);
						}//确认帧发送完成
						for (int i = 0; i < lowerNumber; i++) {
							if (i != ifNo) {
								U8* cre_tree;
								int cre_tree_len = bag_size;
								printf("发送生成树建立包,端口为：%d", i);
								cre_tree = (U8*)malloc(sizeof(U8) * cre_tree_len);
								add_bag_tree(cre_tree, tree_de);
								if (lowerMode[i] == 0) {
									U8* cre_tree_send;
									int cre_tree_send_len = cre_tree_len * 8;
									cre_tree_send = (U8*)malloc(sizeof(U8) * cre_tree_send_len);
									ByteArrayToBitArray(cre_tree_send, cre_tree_send_len, cre_tree, cre_tree_len);
									SendtoLower(cre_tree_send, cre_tree_send_len, i);
								}
								else {
									SendtoLower(cre_tree, cre_tree_len, i);
								}
							}
						}
					}
					else if (tree_de > bufSend[3]) {
						//后面来添加，如果有松弛cost的情况
					}
				}
			}
			else if (bufSend[2] == 3) {//类型为3时，即数据帧
			/*	printf("\n地址转发表：\n");
				for (int i = 0; i <= 99; i++) {
					if (treetable[i]) {
						printf("%02x：%d ", (unsigned char)i, treetable[i] - 1);
					}
				}*/
				if (lowerNumber == 1) {//
					bufSend = bufSend + (bag_size - 2);
					iSndRetval = iSndRetval - (bag_size - 2);
					printf("向上递交数据帧");
				//	print_data_byte(bufSend, iSndRetval, 1);
					iSndRetval = SendtoUpper(bufSend, iSndRetval);
					iSndRetval = iSndRetval * 8;//换算成位,进行统计
					/*原代码主机部分
					U8* buf_ack;
					int need_send=0;
					buf_ack = (U8*)malloc(sizeof(U8) * bag_size);
					int dst = bufSend[0];
					if (bufSend[3] == ACK) {
						ACK = (ACK + 1) % seq_size;
						bufSend = bufSend + (bag_size-2);
						iSndRetval = iSndRetval - (bag_size-2);
			//			printf("向上递交帧为：");
			//			print_data_byte(bufSend, iSndRetval, 1);
						iSndRetval = SendtoUpper(bufSend, iSndRetval);
						iSndRetval = iSndRetval * 8;//换算成位,进行统计
					}
					else need_send = 1;
					int ack_len = add_bag_ack(buf_ack, dst, ACK);
			//		printf("收到数据帧，生成确认帧为：");
			//		print_data_byte(buf_ack, bag_size, 1);
					if (1||need_send) {//必发确认帧
						U8* buf_send_ack;
						int buf_send_ack_len;
						buf_send_ack = (U8*)malloc((ack_len * 8) * sizeof(U8));
						buf_send_ack_len = ByteArrayToBitArray(buf_send_ack, ack_len * 8, buf_ack, ack_len);
						buf_send_ack_len = SendtoLower(buf_send_ack, buf_send_ack_len, 0);
			//			printf("发送成功");
					}*/
				}
				else if (lowerNumber > 1) {//交换机部分
					U8* buf_data;
					int buf_data_len = bufSend_len + 2;
					buf_data = (U8*)malloc(sizeof(U8) * buf_data_len);
					add_bag_tongbu(bufSend, bufSend_len, buf_data);
					if (treetable[buf_data[2]]) {
						if (lowerMode[treetable[buf_data[2]]-1]==0) {
							U8* buf_data_send;
							int buf_data_send_len = buf_data_len * 8;
							buf_data_send = (U8*)malloc(sizeof(U8) * buf_data_send_len);
							ByteArrayToBitArray(buf_data_send, buf_data_send_len, buf_data, buf_data_len);
							SendtoLower(buf_data_send, buf_data_send_len, treetable[buf_data[2]]-1);
							printf("交换机发送数据了！，发送目标为：%d,发送端口为：%d", buf_data[2], treetable[buf_data[2]] - 1);
						}
					}
					else {
						printf("交换机找不到 路了！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！");
					}
				}
			}
			else if (bufSend[2] == 4) {//确认帧
				if (lowerNumber == 1) {
					//	printf("收到确认帧");
					int win_l = -1, win_r = -1;
					for (int i = 0; i < window_len; i++) {
						if (win_len[i] != 0) {
							if (win_l == -1) {
								win_l = (win_data[i][4] + 1) % seq_size;
							}
							win_r = (win_data[i][4] + 1) % seq_size;
						}
					}
					int fg = 0;
					if (bufSend[3] >= win_l) {
						if (bufSend[3] <= win_r && win_r >= win_l) {
							fg = 1;
						}
						else if (bufSend[3] >= win_r && win_r <= win_l) {
							fg = 1;
						}
					}
					else {
						if (bufSend[3] <= win_r && win_r <= win_l) {
							fg = 1;
						}
					}
					printf("窗口左边:%d,窗口右边：%d,ack:%d,fg:%d", win_l, win_r, bufSend[3], fg);
					if (fg == 1) {
						for (int i = 0; i < window_len; i++) {

							if (win_len[i] != 0 && win_data[i][4] != bufSend[3]) {
								free(win_data[i]);
								win_len[i] = 0;
								clk[i] = 0;
							}
							else {
								for (int j = i; j < window_len; j++) {
									swap(win_data[j], win_data[j - i]);
									swap(win_len[j], win_len[j - i]);
									swap(clk[j], clk[j - i]);
								}
								for (int j = window_len - i; j < window_len; j++) {
									if (da_fr_up_len[da_fr_up_fg] != 0) {
										if (lowerNumber == 1) {
											if (treetable[IP2MAC[da_fr_up[da_fr_up_fg][1]]]) {
												win_data[j] = (U8*)malloc(sizeof(U8*) * (da_fr_up_len[da_fr_up_fg] + bag_size));
												win_len[j] = add_bag(da_fr_up[da_fr_up_fg], da_fr_up_len[da_fr_up_fg], seq++, win_data[j]);
												seq %= seq_size;
												win_send(win_data[j], win_len[j], treetable[IP2MAC[da_fr_up[da_fr_up_fg][1]]] - 1);
												free(da_fr_up[da_fr_up_fg]);
												da_fr_up_len[da_fr_up_fg] = 0;
												da_fr_up_fg = (da_fr_up_fg + 1) % da_fr_up_sz;
												//				printf("窗口发送数据：");
												//				print_data_byte(win_data[i], win_len[i], 1);

											}
											clk[j] = printCount;
										}
									}
								}
								break;
							}
						}
					}
					//		printf("确认帧处理完成");
				}
				else if (lowerNumber > 1) {
					U8* buf_data;
					int buf_data_len = bufSend_len + 2;
					buf_data = (U8*)malloc(sizeof(U8) * buf_data_len);
					add_bag_tongbu(bufSend, bufSend_len, buf_data);
					if (lowerMode[treetable[buf_data[2]] - 1] == 0) {
						U8* buf_data_send;
						int buf_data_send_len = buf_data_len * 8;
						buf_data_send = (U8*)malloc(sizeof(U8) * buf_data_send_len);
						ByteArrayToBitArray(buf_data_send, buf_data_send_len, buf_data, buf_data_len);
						SendtoLower(buf_data_send, buf_data_send_len, treetable[buf_data[2]] - 1);
						printf("交换机发送确认帧了！，发送目标为：%d,发送端口为：%d", buf_data[2], treetable[buf_data[2]] - 1);
					}
				}
			}
			else if (bufSend[2] == 5) {
				if (lowerNumber > 1) {
					for (int i = 0; i < lowerNumber; i++) {
						if (i != ifNo && is_send_ok[i]) {//源代码：加  
							U8* buf_arp;
							int buf_arp_len = bufSend_len+2;
							buf_arp = (U8*)malloc(sizeof(U8) * buf_arp_len);
							add_bag_tongbu(bufSend, bufSend_len, buf_arp);
							printf("\n交换机转发5类型包,端口为：%d,源:%d,目的：%d", i,(unsigned char)bufSend[0], (unsigned char)bufSend[1]);
						//	print_data_byte(buf_arp, buf_arp_len, 1);
							if (lowerMode[i] == 0) {
								U8* buf_arp_send;
								int buf_arp_send_len = buf_arp_len * 8;
								buf_arp_send = (U8*)malloc(sizeof(U8) * buf_arp_send_len);
								ByteArrayToBitArray(buf_arp_send, buf_arp_send_len, buf_arp, buf_arp_len);
								SendtoLower(buf_arp_send, buf_arp_send_len, i);
							}
							else {
								SendtoLower(buf_arp, buf_arp_len, i);
							}
						}
					}
				}
				else {
					bufSend = bufSend + (bag_size - 2);
					iSndRetval = iSndRetval - (bag_size - 2);
					printf("向上递交帧");
				//	print_data_byte(bufSend, iSndRetval, 1);
					iSndRetval = SendtoUpper(bufSend, iSndRetval);
					iSndRetval = iSndRetval * 8;
				}
			}
			iSndRetval = iSndRetval * 8;//换算成位,进行统计

		}
		else {
			//低层是字节数组接口，可直接递交
			iSndRetval = SendtoUpper(buf, len);
			iSndRetval = iSndRetval * 8;//换算成位，进行统计
		}
		//统计
		if (iSndRetval <= 0) {
			iSndErrorCount++;
		}
		else {
			iRcvToUpper += iSndRetval;
			iRcvToUpperCount++;
		}
//	}
// 
	//如果需要重传等机制，可能需要将buf或bufSend中的数据另外申请空间缓存起来
/*	if (bufSend != NULL) {
		//缓存bufSend数据，如果有必要的话

		//本例程中没有停等协议，bufSend的空间在用完以后需要释放
		free(bufSend);
	}
	else {
		//缓存buf里的数据，如果有必要的话

		//buf空间不需要释放
	}
	printf("~~~~~~~~~~~~~~~~~~~~~~~~");
	*/
	//打印
	switch (iWorkMode % 10) {
	case 1:
		cout <<endl<< "接收接口 " << ifNo << " 数据：" << endl;
	//	print_data_bit(buf, len, lowerMode[ifNo]);
		break;
	case 2:
		cout << endl << "接收接口 " << ifNo << " 数据：" << endl;
	//	print_data_byte(buf, len, lowerMode[ifNo]);
		break;
	case 0:
		break;
	}
}
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
		cout << "共转发 "<< iRcvForward<< " 位，"<< iRcvForwardCount<<" 次，"<<"递交 "<< iRcvToUpper<<" 位，"<< iRcvToUpperCount<<" 次,"<<"发送 "<< iSndTotal <<" 位，"<< iSndTotalCount<<" 次，"<< "发送不成功 "<< iSndErrorCount<<" 次,""收到不明来源 "<< iRcvUnknownCount<<" 次。";
		spin++;
	}

}
void menu()
{
	int x = strDevID[0] - '0';
	int y = strEntity[0] - '0';
	int my_dr = x * 10 + y;
	printf("%d %d@@%d\n", x, y,my_dr);
	int selection;
	unsigned short port;
	int iSndRetval;
	char kbBuf[100];
	int len;
	U8* bufSend;
	//发送|打印：[发送控制（0，等待键盘输入；1，自动）][打印控制（0，仅定期打印统计信息；1，按bit流打印数据，2按字节流打印数据]
	cout << endl << endl << "设备号:" << strDevID << ",    层次:" << strLayer << ",    实体号:" << strEntity;
	cout << endl << "1-启动自动发送(无效);" << endl << "2-停止自动发送（无效）; " << endl << "3-从键盘输入发送; ";
	cout << endl << "4-仅打印统计信息; " << endl << "5-按比特流打印数据内容;" << endl << "6-按字节流打印数据内容;";
	cout << endl << "0-取消" << endl << "请输入数字选择命令：";
	cin >> selection;
	switch (selection) {
	case 0:

		break;
	case 1:
		iWorkMode = 10 + iWorkMode % 10;
		break;
	case 2:
		iWorkMode = iWorkMode % 10;
		break;
	case 3:
		cout << "输入字符串(,不超过100字符)：";
		cin >> kbBuf;
		cout << "输入低层接口号：";
		cin >> port;

		len = (int)strlen(kbBuf) + 1; //字符串最后有个结束符
		if (port >= lowerNumber) {
			cout << "没有这个接口" << endl;
			return;
		}
		if (lowerMode[port] == 0) {
			//下层接口是比特流数组,需要一片新的缓冲来转换格式
			bufSend = (U8*)malloc(len * 8);

			iSndRetval = ByteArrayToBitArray(bufSend, len * 8, kbBuf, len);
			iSndRetval = SendtoLower(bufSend, iSndRetval, port);
			free(bufSend);
		}
		else {
			//下层接口是字节数组，直接发送
			iSndRetval = SendtoLower(kbBuf, len, port);
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
		cout << endl << "向接口 " << port << " 发送数据：" << endl;
		switch (iWorkMode % 10) {
		case 1:
			print_data_bit(kbBuf, len, 1);
			break;
		case 2:
			print_data_byte(kbBuf, len, 1);
			break;
		case 0:
			break;
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
	}
}