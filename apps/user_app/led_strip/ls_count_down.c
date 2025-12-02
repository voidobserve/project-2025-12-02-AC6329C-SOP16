// 倒计时功能
#include "system/includes.h"

typedef enum
{
    IR_TIMER_NO = 0,        //无定时
    IR_TIMER_1H = 1*60*60*1000,
    IR_TIMER_2H = 2*60*60*1000,

    IR_TIMER_4H = 4*60*60*1000,
    IR_TIMER_8H = 8*60*60*1000,

}timer_e;

/* 
0：无定时
1：2h
2：4h
3：8h
 */
u8 cd_state = 0; 
u32 setting_time;

void change_count_down(void)
{
    cd_state++;
    cd_state%= 4;

    switch(cd_state)
    {
        case 0: setting_time=0;            break;
        case 1: setting_time=IR_TIMER_2H;  break;
        case 2: setting_time=IR_TIMER_4H;  break;
        case 3: setting_time=IR_TIMER_8H;  break;
    }
}

void set_count_down(u8 t)
{
    cd_state = 5;
    // setting_time = t*10*1000;
    setting_time = t*60*60*1000;
    run_white_tips();
}

void set_dly_pwr_off(void)
{
    cd_state = 5;
    setting_time = 60 * 1000;
    
}

// 每10ms跑一次
void count_down_run(void)
{

    if(cd_state != 0)
    {
        if(setting_time >= 10)
        {
            setting_time -=  10;
        }
        else
        {
            cd_state = 0;
            soft_rurn_off_lights();  //关灯.
            
        }
    }
}