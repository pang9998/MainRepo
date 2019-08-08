/******************************************************************************************
 * 文件名		  :	VoiceProject.c
 * 功能		  	:	声音检测系统
 * 作者		  	:	cp1300@139.com
 * 创建时间		:	2018-03-09
 * 最后修改时间	:	2018-03-09
 * 详细:
*******************************************************************************************/

#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "lcd.h"
#include "key.h"
#include "malloc.h"
#include "MMC_SD.h"
#include "ff.h"
#include "exfuns.h"
#include "fontupd.h"
#include "text.h"
#include "vs10XX.h"
#include "recorder.h"
#include "exti.h"
#include "adc.h"
#include "gps.h"
#include "usart2.h"
#include "touch.h"
#include "touch_test.h"
#include "flash.h"
#include "common.h"

#define LCD_W 320    // 横屏
#define LCD_H 240    // 横屏
#define MAX_WAVE_Y 190
#define MIN_WAVE_Y 10
#define Y_SCOPE  180 // y区域



extern u8 ipbuf[16];

extern u8 voice_status;
#define RECV_SIZE 100 // 10 *512 =5.2kB
nmea_msg gpsx; 											    // GPS信息
__align(4) u8 dtbuf[50];   								    // 打印缓存器
const u8 *fixmode_tbl[4] = {"Fail", "Fail", " 2D ", " 3D "};// fix mode字符串

extern u16 sample_buff[SAMPLE_SIZE];

/*****************************************************************************************
 * 函数			:	void Gps_Msg_Show(void)
 * 功能			:	显示GPS定位信息
 * 参数			:	AddrOffset:地址偏移，0-4KB范围；pData：要读取的数据缓冲区；DataLen：要读取的数据长度
 * 返回			:	读取的数据长度
 * 依赖			:	底层宏定义
 * 作者			:	123@139.com
 * 时间			:	2018-03-09
 * 修改时间 	: 2018-03-09
 * 说明			: 注意：地址偏移+写入的数据长度不能超过4KB
 ******************************************************************************************/
void Gps_Msg_Show(void)
{
    float tp;
    POINT_COLOR = BLUE;
    tp = gpsx.longitude;
    sprintf((char *)dtbuf, "Longitude:%.5f %1c   ", tp /= 100000, gpsx.ewhemi);	// 得到经度字符串
    LCD_ShowString(30, 120, 200, 16, 16, dtbuf);
    tp = gpsx.latitude;
    sprintf((char *)dtbuf, "Latitude:%.5f %1c   ", tp /= 100000, gpsx.nshemi);	// 得到纬度字符串
    LCD_ShowString(30, 140, 200, 16, 16, dtbuf);
    tp = gpsx.altitude;
    sprintf((char *)dtbuf, "Altitude:%.1fm     ", tp /= 10);	    			// 得到高度字符串
    LCD_ShowString(30, 160, 200, 16, 16, dtbuf);
    tp = gpsx.speed;
    sprintf((char *)dtbuf, "Speed:%.3fkm/h     ", tp /= 1000);		    		// 得到速度字符串
    LCD_ShowString(30, 180, 200, 16, 16, dtbuf);
    if(gpsx.fixmode <= 3)														// 定位状态
    {
        sprintf((char *)dtbuf, "Fix Mode:%s", fixmode_tbl[gpsx.fixmode]);
        LCD_ShowString(30, 200, 200, 16, 16, dtbuf);
    }
    sprintf((char *)dtbuf, "GPS+BD Valid satellite:%02d", gpsx.posslnum);	 	// 用于定位的GPS卫星数
    LCD_ShowString(30, 220, 200, 16, 16, dtbuf);
    sprintf((char *)dtbuf, "GPS Visible satellite:%02d", gpsx.svnum % 100);	 	// 可见GPS卫星数
    LCD_ShowString(30, 240, 200, 16, 16, dtbuf);

    sprintf((char *)dtbuf, "BD Visible satellite:%02d", gpsx.beidou_svnum % 100);// 可见北斗卫星数
    LCD_ShowString(30, 260, 200, 16, 16, dtbuf);

    sprintf((char *)dtbuf, "UTC Date:%04d/%02d/%02d   ", gpsx.utc.year, gpsx.utc.month, gpsx.utc.date);	// 显示UTC日期
    LCD_ShowString(30, 280, 200, 16, 16, dtbuf);
    sprintf((char *)dtbuf, "UTC Time:%02d:%02d:%02d   ", gpsx.utc.hour, gpsx.utc.min, gpsx.utc.sec);	// 显示UTC时间
    LCD_ShowString(30, 300, 200, 16, 16, dtbuf);
}

// GPS只需要配置一次即可，后面重新上电之后会以一定的频率向单片机发送数据。
void GPSTest()
{
    u16 rxlen;
    u16 lenx;
    u8 key = 0XFF;
    u8 upload = 0;
    // init
    LCD_Clear(WHITE);
    POINT_COLOR = RED;
    LCD_ShowString(30, 20, 200, 16, 16, "ALIENTEK STM32F1 ^_^");
    LCD_ShowString(30, 40, 200, 16, 16, "SkyTraF8-BD TEST");
    LCD_ShowString(30, 60, 200, 16, 16, "ATOM@ALIENTEK");
    LCD_ShowString(30, 80, 200, 16, 16, "KEY0:Upload NMEA Data SW");
    LCD_ShowString(30, 100, 200, 16, 16, "NMEA Data Upload:OFF");
    uart_init(72, 115200);      // 串口2初始化
    if(SkyTra_Cfg_Rate(5) != 0)	// 设置定位信息更新速度为5Hz,顺便判断GPS模块是否在位.
    {
        LCD_ShowString(30, 120, 200, 16, 16, "SkyTraF8-BD Setting...");
        do
        {
            uart_init(72, 9600);  					// 初始化串口3波特率为9600
            SkyTra_Cfg_Prt(3);	  					// 重新设置模块的波特率为38400
            uart_init(72, 38400);					// 初始化串口3波特率为38400
            key = SkyTra_Cfg_Tp(100000); 			// 脉冲宽度为100ms
        }
        while(SkyTra_Cfg_Rate(5) != 0 && key != 0); // 配置SkyTraF8-BD的更新速率为5Hz
        LCD_ShowString(30, 120, 200, 16, 16, "SkyTraF8-BD Set Done!!");
        delay_ms(500);
        LCD_Fill(30, 120, 30 + 200, 120 + 16, WHITE); // 清除显示
    }
    // get info
    while(1)
    {
        delay_ms(1);
        if(USART_RX_STA & 0X8000)		    // 接收到一次数据了
        {
            rxlen = USART_RX_STA & 0X7FFF;	// 得到数据长度
            USART_RX_STA = 0;		     	// 启动下一次接收
            //	USART1_TX_BUF[i]=0;			// 自动添加结束符
            USART_RX_BUF[rxlen + 1] = 0;
            GPS_Analysis(&gpsx, (u8 *)USART_RX_BUF); // 分析字符串
            Gps_Msg_Show();				     // 显示信息
            if(upload)printf("\r\n%s\r\n", (u8 *)USART_RX_BUF); // 发送接收到的数据到串口1
        }
        key = KEY_Scan(0);
        if(key == KEY0_PRES)
        {
            upload = !upload;
            POINT_COLOR = RED;
            if(upload)LCD_ShowString(30, 100, 200, 16, 16, "NMEA Data Upload:ON ");
            else LCD_ShowString(30, 100, 200, 16, 16, "NMEA Data Upload:OFF");
        }
        if((lenx % 500) == 0)
            LED0 = !LED0;
        lenx++;
    }
}

void VS_Test()
{
    while(1)
    {
        Show_Str(40, 180, 200, 16, "存储器测试...", 16, 0);
        printf("Ram Test:0X%04X\r\n", VS_Ram_Test());  // 打印RAM测试结果
        Show_Str(40, 200, 200, 16, "正弦波测试...", 16, 0);
        VS_Sine_Test();
        Show_Str(60, 170, 220, 16, "<<录音机实验>>", 16, 0);
        LCD_DrawRectangle(60, 220, 200, 185);
        recoder_play();
    }



}

//连接服务器进入透传模式
void ConnectServer()
{
    u8 *p;
    p = mymalloc(32);		 // 申请32字节内存
    printf("ip:%s", ipbuf);

    atk_8266_quit_trans(); 	 // 退出透传
    atk_8266_quit_trans();	 // 退出透传
    atk_8266_send_cmd("AT+CIPMODE=0", "OK", 20);  // 关闭透传模式
    atk_8266_send_cmd("AT+CIPMUX=0", "OK", 20);   // 0：单连接，1：多连接
    sprintf((char *)p, "AT+CIPSTART=\"TCP\",\"%s\",%s", ipbuf, (u8 *)portnum); // 配置目标TCP服务器
    while(atk_8266_send_cmd(p, "OK|ALREADY", 200))
    {
        LCD_Clear(WHITE);
        POINT_COLOR = RED;
        Show_Str_Mid(0, 40, "WK_UP:返回重选", 16, 240);
        Show_Str_Mid(0, 80, "ATK-ESP 连接TCP Server失败", 12, 240); // 连接失败
    }
    atk_8266_send_cmd("AT+CIPMODE=1", "OK", 200);    // 传输模式为：透传
    atk_8266_send_cmd("AT+CIPSEND", "OK", 20);       // 开始透传
    myfree(p);										 // 释放内存
}

void TCP_Upload_Test()
{
    ConnectServer();
    rec_upload_wav("0:RECORDER/REC00002.wav");
}


// 波形测试
void DrawWaveTest()
{
    u16 i = 0, y = 0, lasx = 0, lasy = 0;
    u8 a = 0;
    float VolValue = 0;
    VolValue = 0 / 0X0FFF;
    POINT_COLOR = BLUE;      // 设置字体为红色
    for(i = 0; i < LCD_W; i++)
    {

        if(!a)
        {
            if(y < 200)
            {
                y = y + 10;
            }
            else
            {
                a = 1;
            }
        }
        else
        {
            if(y > 0)
            {
                y = y - 10;
            }
            else
            {
                a = 0;
            }
        }
        LCD_DrawLine(lasx, lasy, i, y); 
        lasx = i;
        lasy = y;
        delay_ms(10);
    }
    POINT_COLOR = RED; // 设置字体为红色

}

// 波形显示
void DrawWave()
{
    u16 i = 0, y = 0, lasx = 0, lasy = MAX_WAVE_Y, adc_tmp;
    u8 a = 0;
    float VolValue = 0;
    u16 cc[100] = {0};
    POINT_COLOR = BLUE; //设置字体为红色
    for(i = 0; i < LCD_W; )
    {
        adc_tmp = Get_Adc(1);
        VolValue = (float)adc_tmp / 0X0FFF;
        y = Y_SCOPE * (1 - VolValue) + MIN_WAVE_Y;
        //printf("y：%d\r\n", adc_tmp);
        if(lasx != 160 && i != 160 && lasy != 100 && y != 100)LCD_DrawLine(lasx, lasy, i, y); //5
        lasx = i;
        lasy = y;
        i = i + 1;
    }
    POINT_COLOR = RED; //设置字体为红色
}

//字库初始化
void Init_Font()
{
    u8 key, fontok = 0;

RST:
    fontok = font_init();		// 检查字库是否OK
    if(fontok)			    	// 需要更新字库
    {

        while(SD_Initialize())
        {
            LCD_ShowString(60, 170, 200, 16, 16, "SD Card Error");
            delay_ms(200);
            LCD_Fill(20, 170, 200 + 20, 170 + 16, WHITE);
            delay_ms(200);
        }

        LCD_Clear(WHITE);		   		// 清屏
        POINT_COLOR = RED;				// 设置字体为红色
        LCD_ShowString(60, 50, 200, 16, 16, "ALIENTEK STM32");
        LCD_ShowString(60, 70, 200, 16, 16, "SD Card OK");
        LCD_ShowString(60, 90, 200, 16, 16, "Font Updating...");
        key = update_font(20, 110, 16); // 从SD卡更新字库
        while(key)						// 更新失败
        {
            LCD_ShowString(60, 110, 200, 16, 16, "Font Update Failed!");
            delay_ms(200);
            LCD_Fill(20, 110, 200 + 20, 110 + 16, WHITE);
            delay_ms(200);
        }
        LCD_ShowString(60, 110, 200, 16, 16, "Font Update Success!");
        delay_ms(1500);
        LCD_Clear(WHITE);				// 清屏
        goto RST;
    }
}

void WiFi_Config()
{
    u8 key = 0;
    u8 timex;
    POINT_COLOR = RED;
    Show_Str_Mid(0, 30, "ATK-ESP8266 WIFI模块测试", 16, 240);
    while(atk_8266_send_cmd("AT", "OK", 20))          // 检查WIFI模块是否在线
    {
        atk_8266_quit_trans();						  // 退出透传
        atk_8266_send_cmd("AT+CIPMODE=0", "OK", 200); // 关闭透传模式
        Show_Str(40, 55, 200, 16, "未检测到模块!!!", 16, 0);
        delay_ms(800);
        LCD_Fill(40, 55, 200, 55 + 16, WHITE);
        Show_Str(40, 55, 200, 16, "尝试连接模块...", 16, 0);
    }
    while(atk_8266_send_cmd("ATE0", "OK", 20));       // 关闭回显
    delay_ms(10);
    atk_8266_at_response(1);						  // 检查ATK-ESP8266模块发送过来的数据,及时上传给电脑
    LCD_Clear(WHITE);
    POINT_COLOR = RED;
    atk_8266_test();
    timex = 0;
}


void GPSInit()
{
    uart_init(72, 38400); 	// 串口1初始化
    ClearBuff();
    USART_RX_STA = 0;	    // 启动下一次接收
}


void GetGpsInfo()
{
    u16 rxlen;
    float tp;

    if(USART_RX_STA & 0X8000)	    			 // 接收到一次数据了
    {
        rxlen = USART_RX_STA & 0X3FFF;			 // 得到数据长度
        USART_RX_BUF[rxlen + 1] = 0;
        GPS_Analysis(&gpsx, (u8 *)USART_RX_BUF); // 分析字符串
        tp = gpsx.longitude;
        sprintf((char *)dtbuf, "经度:%.5f%1c", tp /= 100000, gpsx.ewhemi);	// 得到经度字符串
        Show_Str(10, 202, 200, DEFAULT_ZH_FONT_SIZE, dtbuf, DEFAULT_ZH_FONT_SIZE, 0);
        tp = gpsx.latitude;
        sprintf((char *)dtbuf, "纬度:%.5f%1c", tp /= 100000, gpsx.nshemi);	// 得到纬度字符串
        Show_Str(100, 202, 200, DEFAULT_ZH_FONT_SIZE, dtbuf, DEFAULT_ZH_FONT_SIZE, 0);
        tp = gpsx.altitude;
        sprintf((char *)dtbuf, "高度:%.1fm", tp /= 10);	    			    // 得到高度字符串
        Show_Str(190, 202, 200, DEFAULT_ZH_FONT_SIZE, dtbuf, DEFAULT_ZH_FONT_SIZE, 0);

        sprintf((char *)dtbuf, "时间:%04d/%02d/%02d %02d:%02d:%02d   ", gpsx.utc.year, gpsx.utc.month, gpsx.utc.date, gpsx.utc.hour, gpsx.utc.min, gpsx.utc.sec); // 显示UTC时间

        Show_Str(10, 220, 200, DEFAULT_ZH_FONT_SIZE, dtbuf, DEFAULT_ZH_FONT_SIZE, 0);

        LED0 = !LED0;

        ClearBuff();
        USART_RX_STA = 0;
    }
    else
    {
        delay_ms(200);
    }

}
// 声音识别逻辑
void recognize()
{
    u8 i = 0;
    u32 tmp = 0;
    for(i = 0; i < SAMPLE_SIZE; i++)
    {
        tmp += sample_buff[i];
        sample_buff[i] = 0;  // 清空
    }
    tmp = tmp / SAMPLE_SIZE; // 求均值
    //if(tmp<10)return;//太小，保留上次值
    printf("recog:%d\r\n", tmp);
    if(tmp < 167 && tmp > 136)
    {
        Show_Str(255, 202, 200, DEFAULT_ZH_FONT_SIZE, "结果:大叫", DEFAULT_ZH_FONT_SIZE, 0);
    }
    else if(tmp > 175 && tmp <= 180)
    {
        Show_Str(255, 202, 200, DEFAULT_ZH_FONT_SIZE, "结果:爆炸", DEFAULT_ZH_FONT_SIZE, 0);
    }
    else if(tmp > 997)
    {
        Show_Str(255, 202, 200, DEFAULT_ZH_FONT_SIZE, "结果:笑", DEFAULT_ZH_FONT_SIZE, 0);
    }
    else if(tmp < 255 && tmp > 180)
    {
        Show_Str(255, 202, 200, DEFAULT_ZH_FONT_SIZE, "结果:大哭", DEFAULT_ZH_FONT_SIZE, 0);
    }
    delay_ms(1000);

}

/** 录音测试 */
void RecordTest()
{
    u8 *pname;
    pname = audio_record(RECV_SIZE);
    while(1)
    {
        rec_play_wav(pname);
        delay_ms(1000);
    }
    myfree(pname);
    _STRACE_
}


int main(void)
{

    u8 *pname;
    Stm32_Clock_Init(9);	  // 系统时钟设置
    delay_init(72);			  // 延时初始化
    LED_Init();         	  // LED初始化
    uart_init(72, 115200);    // 串口1初始化
    RCC->AHBENR |= RCC_AHBENR_CRCEN; // 硬件crc初始化
    LCD_Init();				  // 初始化液晶
    KEY_Init();				  // 按键初始化
    VS_Init();				  // 初始化VS1053
    mem_init();				  // 初始化内存池
    exfuns_init();		      // 为fatfs相关变量申请内存
    f_mount(fs[0], "0:", 1);  // 挂载SD卡
    f_mount(fs[1], "1:", 1);  // 挂载FLASH.
    Init_Font();              // 因为flash 的cs引脚是uart_TX即pa2,所以先初始化

    tp_dev.init();			  // 触摸屏初始化
    EXTI_Init();
    Adc_Init();

    USART2_Init(36, 115200); // 串口2初始化

    POINT_COLOR = RED;       // 设置字体为红色
    LCD_ShowString(60, 30, 200, 16, 16, "Mini STM32");
    LCD_ShowString(60, 50, 200, 16, 16, "RECORDER TEST");
    LCD_ShowString(60, 70, 200, 16, 16, "ATOM@ALIENTEK");
    LCD_ShowString(60, 90, 200, 16, 16, "KEY0:STOP&SAVE");
    LCD_ShowString(60, 110, 200, 16, 16, "KEY1:REC/PAUSE");
    LCD_ShowString(60, 130, 200, 16, 16, "WK_UP:PLAY ");
    LCD_ShowString(60, 150, 200, 16, 16, "2014/3/26");

    IPConf_UI();
    LCD_Display_Dir(1);   	 		 // 横屏
    POINT_COLOR = RED; 		 		 // 设置字体为红色
    LCD_Fill(0, 0, 320, 200, WHITE); // 清除波形
    LCD_Clear(WHITE);
    //	_STRACE_
    printf("init ok\r\n");
    LCD_Fill(0, 0, 320, 200, WHITE); // 清除波形
    LCD_DrawLine(160, 0, 160, 200);  // y轴
    LCD_DrawLine(0, 100, 320, 100);  // x轴
    LCD_DrawLine(0, 200, 320, 200);  // x轴

    Show_Str(10, 202, 200, DEFAULT_ZH_FONT_SIZE, "经度:0.00000E", DEFAULT_ZH_FONT_SIZE, 0);
    Show_Str(100, 202, 200, DEFAULT_ZH_FONT_SIZE, "纬度:0.00000N", DEFAULT_ZH_FONT_SIZE, 0);
    Show_Str(190, 202, 200, DEFAULT_ZH_FONT_SIZE, "高度:0.0m", DEFAULT_ZH_FONT_SIZE, 0);
    //	Show_Str(255, 202, 200, DEFAULT_ZH_FONT_SIZE,"结果:大喜", DEFAULT_ZH_FONT_SIZE, 0);
    
    Show_Str(10, 220, 200, DEFAULT_ZH_FONT_SIZE, "时间:2019/04/03 12:21:05", DEFAULT_ZH_FONT_SIZE, 0);
    Show_Str(180, 220, 200, DEFAULT_ZH_FONT_SIZE,  "状态：无声音信号", DEFAULT_ZH_FONT_SIZE, 0);

    GPSInit();
    ConnectServer();
    while(1)
    {
        // 清空四象限
        LCD_Fill(0, MIN_WAVE_Y, 159, 99, WHITE);
        LCD_Fill(161, MIN_WAVE_Y, 320, 99, WHITE);

        LCD_Fill(0, 101, 159, 199, WHITE);
        LCD_Fill(161, 101, 320, 199, WHITE);

        DrawWave();

        GetGpsInfo();

        delay_ms(100);

        //	atk_8266_test(); // 进入ATK_ESP8266测试

        if(voice_status > VOICE_STATUS_SILENCE)
        {
            if(voice_status == VOICE_STATUS_START)
            {

                Show_Str(180, 220, 200, DEFAULT_ZH_FONT_SIZE, "状态：检测到声音信号 ", DEFAULT_ZH_FONT_SIZE, 0);
            }
            else if(voice_status == VOICE_STATUS_SAMPLE_END)  // 采样结束，开始录音
            {

                Show_Str(180, 220, 200, DEFAULT_ZH_FONT_SIZE, "状态：正在识别中...  ", DEFAULT_ZH_FONT_SIZE, 0);
                // recoder_play();
                //__enable_irq();    				  // 开启总中断
                delay_ms(500);
                pname = audio_record(RECV_SIZE);
                //rec_play_wav(pname);
                recognize();
                Show_Str(180, 220, 200, DEFAULT_ZH_FONT_SIZE, "状态：正在上传音频... ", DEFAULT_ZH_FONT_SIZE, 0);

                rec_upload_wav(pname);
                voice_status = VOICE_STATUS_END;
                __enable_irq();    					  // 开启总中断
                // rec_upload_wav("0:RECORDER/REC00024.wav");
            }
            else if(voice_status == VOICE_STATUS_END)
            {

                Show_Str(180, 220, 200, DEFAULT_ZH_FONT_SIZE,  "状态：上传完成!       ", DEFAULT_ZH_FONT_SIZE, 0);
                delay_ms(2000);
                voice_status = VOICE_STATUS_SILENCE;  // 重置状态
            }
        }
        else
        {
            Show_Str(180, 220, 200, DEFAULT_ZH_FONT_SIZE,  "状态：无声音信号       ", DEFAULT_ZH_FONT_SIZE, 0);
        }

    }
}


