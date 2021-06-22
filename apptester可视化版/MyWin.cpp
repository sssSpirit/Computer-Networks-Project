#include <graphics.h>              // 引用图形库头文件
#include <conio.h>
#include <windows.h>				//用到了定时函数sleep()
#include <math.h>
#include <string.h>

#include <iostream>
#include "stdio.h"
#include "CfgFileParms.h"
#include "function.h"

/****

int r[3][4] = { {30,20,130,60},{170,20,220,60},{260,20,310,60} };//三个按钮的二维数组
float alpha, R, h0, sim_t;//碰撞时的能量损失率，球的半径、初始高度、仿真时间

MOUSEMSG m;

//按钮判断函数
int button_judge(int x, int y)
{
	if (x > r[0][0] && x<r[0][2] && y>r[0][1] && y < r[0][3])return 1;
	if (x > r[1][0] && x<r[1][2] && y>r[1][1] && y < r[1][3])return 2;
	if (x > r[2][0] && x<r[2][2] && y>r[2][1] && y < r[2][3])return 3;
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
	short win_width, win_height;//定义窗口的宽度和高度
	win_width = 480; win_height = 360;
	initgraph(win_width, win_height);//初始化窗口（黑屏）
	
	RECT R1 = { r[0][0],r[0][1],r[0][2],r[0][3] };
	RECT R2 = { r[1][0],r[1][1],r[1][2],r[1][3] };
	RECT R3 = { r[2][0],r[2][1],r[2][2],r[2][3] };

	LOGFONT f;//字体样式指针
	gettextstyle(&f);					//获取字体样式
	_tcscpy(f.lfFaceName, _T("宋体"));	//设置字体为宋体
	f.lfQuality = ANTIALIASED_QUALITY;    // 设置输出效果为抗锯齿  
	settextstyle(&f);                     // 设置字体样式
	settextcolor(BLACK);				//BLACK在graphic.h头文件里面被定义为黑色的颜色常量
	drawtext("建连接", &R1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R1内输入文字，水平居中，垂直居中，单行显示
	drawtext("发图片", &R2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R2内输入文字，水平居中，垂直居中，单行显示
	drawtext("发文字", &R3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);//在矩形区域R3内输入文字，水平居中，垂直居中，单行显示
	setlinecolor(BLACK);
	rectangle(r[0][0], r[0][1], r[0][2], r[0][3]);
	rectangle(r[1][0], r[1][1], r[1][2], r[1][3]);
	rectangle(r[2][0], r[2][1], r[2][2], r[2][3]);
	
}

int win_case_judge(void)
{
	m = GetMouseMsg();

	switch (button_judge(m.x, m.y))
	{
		//复原按钮原型
	case 1:
		char dst[1];
		int dst;
		InputBox(dst, 1, "请输入连接对象");
		sscanf(dst, "%d", &dst);//将输入字符串依次扫描到全局变量里面
		FlushMouseMsgBuffer();//单击事件后清空鼠标消息
		break;
	case 2:
		
		FlushMouseMsgBuffer();//单击事件后清空鼠标消息
		break;
	case 3:
		
		FlushMouseMsgBuffer();//单击事件后清空鼠标消息
	default:
		FlushMouseMsgBuffer();//单击事件后清空鼠标消息
		//printf("\r\n(%d,%d)",m.x,m.y);//打印鼠标坐标，方便调试时确定区域
		break;
	}

}


void txt_in(char* s)
{
	InputBox(s, 100, "请输入语句");
}


****/