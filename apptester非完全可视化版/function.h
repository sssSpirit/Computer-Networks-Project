#pragma once
//yeah once, sure
//一般故事都是从定义开始
#define MAX_BUFFER_SIZE 65000 //默认缓冲的最大大小，也是UDP报文的上限
#define DEFAULT_TIMER_INTERVAL 50 //默认计时器时间间隔20ms，单位是毫秒
#define DEFAULT_AUTO_SEND_TIME	5 //默认自动发送下，每autoSendTime*DEFAULT_TIMER_INTERVEL = 5 * 20ms = 100ms发送一次，
#define DEFAULT_AUTO_SEND_SIZE  100 //默认每次发送100字节
typedef char U8; //定义单字节无符号整数，这样就不纠结char这个类型了，现在是单字节整数

//============重点来了================================================================================================
//------------重要的控制参数------------------------------------------------------------
//以下从配置文件ne.txt中读取的重要参数
extern int lowerMode[10]; //如果低层是物理层模拟软件，这个数组放置的是每个接口对应的数据格式——0为比特数组，1为字节数组
extern int lowerNumber;   //低层实体数量，比如网络层之下可能有多个链路层
extern int autoSendTime;  //默认自动发送下，每autoSendTime*DEFAULT_TIMER_INTERVEL = 5 * 20ms = 100ms发送一次，
extern int autoSendSize; //默认每次发送100字节
extern int iWorkMode;     //实体的工作模式，十位数：1 自动发送，0 手动发送；个位数 1 打印数据内容，0 打印统计数据
extern string strDevID;    //设备号，字符串形式，从1开始
extern string strLayer;    //层次名
extern string strEntity;   //实体好，字符串形式，从0开始，可以通过atoi函数变成整数在程序中使用


// 
//start------华丽的分割线---------------------------------------------------------
//------------重要的需要考虑实现其内容的函数------------------------------------------------------------
void TimeOut();
void RecvfromLower(U8* buf, int len, int ifNo);
void RecvfromUpper(U8* buf, int len);
void InitFunction(CCfgFileParms& cfgParms);
void EndFunction();

//------------重要的会使用到的函数------------------------------------------------------------
//************发送数据的函数*************
int SendtoUpper(U8* buf, int len);
int SendtoLower(U8* buf, int len, int ifNo);
int SendtoCommander(U8* buf, int len);
//************一些工具函数***************
void print_data_bit(U8* A, int length, int iMode);
void print_data_byte(U8* A, int length, int iMode);
void code(unsigned long x, U8 A[], int length);
unsigned long decode(U8 A[], int length);
int ByteArrayToBitArray(U8* bitA, int iBitLen, U8* byteA, int iByteLen);
int BitArrayToByteArray(U8* bitA, int iBitLen, U8* byteA, int iByteLen);
void StartTimerOnce(ULONG ulInterval);
void StartTimerPeriodically(ULONG ulInterval);


int button_judge(int x, int y);
void init_figure(void);
void show_main_win(void);
int win_case_judge(void);


