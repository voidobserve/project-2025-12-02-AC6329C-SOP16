#ifndef _ONE_WIRE_H
#define _ONE_WIRE_H

#include "system/includes.h"


typedef struct
{
    // 000:回正
    // 001:区域1摇摆
    // 010:区域2摇摆
    // 011:区域1和区域2摇摆
    // 100:360°正转
    // 101:音乐律动
    // 110:关闭电机
    u8 mode;           //电机模式（带记忆，保存在flash；mode==6 表示电机关闭）
    u8 period;          //000:  8S; 001:  13S; 010:  18S ;011:  21S ;100:  26S  //转速  
    u8 dir;            //1:反转 0:正转  仅音乐律动模式有效
    u8 music_mode;     //音乐律动下的转动模式
}base_ins_t;


extern u8 period[6];

/* 电机模式/开关状态记忆相关接口 */
void one_wire_set_mode(u8 m);      //设置电机模式（关闭电机：m = 6）
void enable_one_wire(void);        //按当前模式打包并发送指令
void motor_state_recover(void);    //开机/上电/开灯时按记忆恢复电机状态（记忆为关闭则重发关闭信号）
void motor_stop_temporary(void);   //临时停止电机，不改变记忆的模式/开关状态
u8 get_motor_switch_state(void);   //获取记忆的电机开关状态：1开 0关



#endif



