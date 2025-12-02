/****************************
@led_strand_effect.c
适用：
产品ID: yxwh27s5
品类：幻彩户外串灯-蓝牙
协议：BLE
负责幻彩灯串效果制作
*****************************/
#include "system/includes.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "ws2812fx_effect.h"
#include "Adafruit_NeoPixel.H"
#include "led_strip_drive.h"
#include "app_main.h"
#include "asm/mcpwm.h"
extern void printf_buf(u8 *buf, u32 len);
static void static_mode(void);
static void fc_smear_adjust(void);
static void fc_pair_effect(void);
static void ls_scene_effect(void);
void fc_set_style_custom(void);
void custom_effect(void);
static void strand_rainbow(void);
void strand_jump_change(void) ;
void standard_jump(void);
void standart_jump_fix(void);
void strand_breath(void) ;
void strand_twihkle(void) ;
void strand_flow_water(void) ;
void strand_chas_light(void) ;
void strand_colorful(void) ;
void strand_grandual(void) ;
void set_static_mode(u8 r, u8 g, u8 b);
void mutil_c_grandual(void);
void ls_strobe(void);
void w_grandual(void);
void single_c_breath(void);
void custom_meteor_effect(void);
void fb_led_on_off_state(void);
void strand_meteor(u8 index);
void double_meteor(void);
// void fc_music(void);



extern LED_STATE led_state;
extern u8 is_rgbw;
fc_effect_t fc_effect;//幻彩灯串效果数据


// 效果数据初始化
void fc_data_init(void)
{
    u16 num;
//灯具
    fc_effect.on_off_flag = DEVICE_ON;  //灯为开启状态
    fc_effect.led_num = 16;        //灯带的总灯珠数量
    fc_effect.Now_state = IS_STATIC;  //当前运行状态 静态
    fc_effect.rgb.r = 255;
    fc_effect.rgb.g = 255;
    fc_effect.rgb.b = 255;
    fc_effect.dream_scene.c_n = 1;  //颜色数量为1
    fc_effect.b = 255;
    fc_effect.dream_scene.speed =  100;
    fc_effect.sequence = NEO_RGB;
//流星
    fc_effect.metemor_on_off = 0x01;        //开关
    fc_effect.metemor_effect_index = 1;     //效果编号
    fc_effect.speed=100;                    //变化速度
    fc_effect.meteor_period = 8;            //默认8秒  周期值
    fc_effect.period_cnt = fc_effect.meteor_period*1000;  //ms,运行时的计数器
    fc_effect.mode_cycle = 0;   //模式完成一个循环的标志
//电机
    fc_effect.base_ins.mode = 4;   //360转
    fc_effect.base_ins.period = 8;  //速度8s
    fc_effect.base_ins.dir = 0 ;  // 0: 正转  1：
    fc_effect.base_ins.music_mode = 0;
//声控部分
    fc_effect.sound.c_v = 0;
    fc_effect.sound.v = 0;
    fc_effect.sound.sensitive = 30;
    fc_effect.sound.valid = 0;
//闹钟
    zd_countdown[0].set_on_off = DEVICE_OFF;
    zd_countdown[1].set_on_off = DEVICE_OFF;
    zd_countdown[2].set_on_off = DEVICE_OFF;


}
// WS2812FX_mode_comet
// WS2812FX_mode_scan
extern uint16_t WS2812FX_mode_comet_1(void) ;


//全彩效果初始化
void full_color_init(void)
{
    printf("full_color_init");
    WS2812FX_init(fc_effect.led_num + 1,fc_effect.sequence);     //初始化ws2811
    WS2812FX_setBrightness( fc_effect.b );
    soft_turn_on_the_light();  //软开灯，灯为开状态
    set_fc_effect();  //效果实现调度
    custom_meteor_effect(); //实现上电，LED的流星效果

}

/**
 * @brief 向app反馈灯开关状态
 *
 */
void fb_led_on_off_state(void)
{
    uint8_t Send_buffer[6];
    Send_buffer[0] = 0x01;
    Send_buffer[1] = 0x01;
    Send_buffer[2] = fc_effect.on_off_flag; //
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 3);


}
/**
 * @brief 反馈音乐模式
 * 
 */
void fb_led_music_mode(void)
{
    uint8_t Send_buffer[6]; 
    Send_buffer[0] = 0x06;
    Send_buffer[1] = 0x06;
    Send_buffer[2] = fc_effect.music.m ;// 
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 3);

}


/**
 * @brief 反馈流星速度
 *
 */
void fd_meteor_speed(void)
{
    uint8_t Send_buffer[6];
    Send_buffer[0] = 0x2F;
    Send_buffer[1] = 0x01;
    Send_buffer[2] = 110 - (fc_effect.speed /3);
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 3);

}

/**
 * @brief 反馈流星周期
 *
 */
void fd_meteor_cycle(void)
{
    uint8_t Send_buffer[6];
    Send_buffer[0] = 0x2F;
    Send_buffer[1] = 0x03;
    Send_buffer[2] = fc_effect.meteor_period;
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 3);
}

/**
 * @brief 反馈流星开关
 *
 */
void fd_meteor_on_off(void)
{
    uint8_t Send_buffer[6];
    Send_buffer[6] = 0x2F;
    Send_buffer[7] = 0x02;
    Send_buffer[8] = fc_effect.metemor_on_off;
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 3);
}


void turn_on_meteor(void)
{
    fc_effect.metemor_on_off  = 0x01;

}
void turn_off_meteor(void)
{
    fc_effect.metemor_on_off  = 0x02;
}




/**************************************************效果调度函数*****************************************************/
/**
 * @brief 效果调度函数，想要实现效果，必须要有调度函数
 *
 */
void   flash_printf(void);
void set_fc_effect(void)
{
    printf("set_fc_effect");

    if(fc_effect.on_off_flag == DEVICE_ON)
    {
        fc_effect.period_cnt = 0;
        switch (fc_effect.Now_state)
        {
            //幻彩场景
            case IS_light_scene:
                fc_effect.w = 0;
                set_bright(100);
                ls_scene_effect();
                break;
            //配对模式
            case ACT_TY_PAIR:  // 配对完成，要恢复fc_effect.Now_state
                fc_pair_effect();
                break;
            //自定义效果模式
            case ACT_CUSTOM:
                // custom_effect();

              //  custom_meteor_effect();
                break;
            case IS_light_music:

                fc_effect.w = 0;
                set_bright(100);
                /* code */

                extern uint16_t fc_music_gradual(void);
                extern uint16_t fc_music_breath(void) ;
                extern uint16_t fc_music_static(void) ;
                extern uint16_t fc_music_twinkle(void) ;

                if(fc_effect.music.m == 0)   //能量1
                {
                    // WS2812FX_setMode(&fc_music_gradual);
                    WS2812FX_setSegment_colorOptions(
                        0,                                      //第0段
                        0,0,                  //起始位置，结束位置
                        &fc_music_gradual,        //效果 // 渐变，触发变色
                        0,                                      //颜色，WS2812FX_setColors设置
                        fc_effect.dream_scene.speed,            //速度
                        0);

                    WS2812FX_start();

                }

                if(fc_effect.music.m == 1)  //节奏1
                {
                    // WS2812FX_setMode(&fc_music_breath);
                    WS2812FX_setSegment_colorOptions(
                        0,                                      //第0段
                        0,0,                  //起始位置，结束位置
                        &fc_music_breath,        //效果// 呼吸，触发渐亮-》渐暗，最后黑，每次变色
                        0,                                      //颜色，WS2812FX_setColors设置
                        fc_effect.dream_scene.speed,            //速度
                        0);

                    WS2812FX_start();

                }

                if(fc_effect.music.m == 2)  //频谱1
                {
                    WS2812FX_setSegment_colorOptions(
                        0,                                      //第0段
                        0,0,                  //起始位置，结束位置
                        &fc_music_static,        //效果  // 定色，触发换颜色
                        0,                                      //颜色，WS2812FX_setColors设置
                        fc_effect.dream_scene.speed,            //速度
                        0);                            //选项，这里像素点大小：1

                    WS2812FX_start();

                }

                if(fc_effect.music.m == 3)  //滚动
                {
                    // WS2812FX_setMode(&fc_music_twinkle);
                    WS2812FX_setSegment_colorOptions(
                        0,                                      //第0段
                        0,0,                  //起始位置，结束位置
                        &fc_music_twinkle,        //效果 定色，触发黑->爆闪一下，每次变色
                        0,                                      //颜色，WS2812FX_setColors设置
                        fc_effect.dream_scene.speed,            //速度
                        0);   //选项，这里像素点大小：1
                    WS2812FX_start();

                }
                break;
            case IS_smear_adjust:
                // fc_smear_adjust();
                break;
            //静态模式
            case IS_STATIC:
                static_mode();
                printf("IS_STATIC");
                break;
            default:
                break;
        }

    } //if(fc_effect.on_off_flag == DEVICE_ON)
}

// -------------------------------------------------------------------------------------------------工具
void ls_set_colors(uint8_t n, color_t *c)
{
    uint32_t colors[MAX_NUM_COLORS];
    uint8_t i;
    for(i=0; i < n; i++)
    {
        colors[i] = c[i].r << 16 | c[i].g << 8 | c[i].b;
    }
    WS2812FX_setColors(0,colors);
}

/***************************************************自定义效果*****************************************************/


extern uint16_t power_on_effect(void);
extern uint16_t power_off_effect(void);

// 1~4，正向流水效果
// 5~8：反向流水效果
// 9~10：音乐律动
u8 meteor_mode = 0;
// FADE_SLOW：12颗
// FADE_MEDIUM：6颗
// FADE_FAST：5颗灯
// FADE_XFAST:3颗灯
const u8 fade_type[3] =
{
    FADE_XFAST,FADE_FAST,FADE_MEDIUM //,FADE_SLOW
};

void set_power_off(void)
{
    fc_effect.metemor_effect_index = 1; //关机效果
    fc_set_style_custom(); //自定义效果
    set_fc_effect();
}

void change_meteor_mode(void)
{
    fc_effect.metemor_effect_index++;
    if(fc_effect.metemor_effect_index > 0x0a)
    {
        fc_effect.metemor_effect_index = 1;
    }
    set_fc_effect();
}

/**
 * @brief Set the mereor mode object
 * 设置流星效果
 *
 * @param m
 */
void set_mereor_mode(u8 m)
{
    if(m <= 22)  //void custom_meteor_effect(void)有关
    {
        fc_effect.metemor_effect_index = m;
          printf(" fc_effect.metemor_effect_index  = %d",  fc_effect.metemor_effect_index );
        custom_meteor_effect();
  
    }
}

//设置流星速度 0 - 100    0-330ms
void set_mereor_speed(u8 s)
{

    fc_effect.speed = 300 * (100-s+10) /100;
    custom_meteor_effect();
}

//调流星速度 1 25 50 75 100
void adjust_meteor_speed(void)
{
  
    u8 ss = 110 - (fc_effect.speed /3);
    if(ss < 100 -24)
        ss += 24;
    else
        ss = 1;
    fc_effect.speed = 300 * (100 - ss+10) /100;
    // printf("fc_effect.speed = %d",fc_effect.speed);
    // printf("ss = %d", ss);
    custom_meteor_effect();
  
}
//获取当前流星效果
u8 get_custom_index(void)
{
    return fc_effect.metemor_effect_index;
}

void change_dir(void)
{
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        fc_effect.dream_scene.direction = IS_back;

    }
    else{
        fc_effect.dream_scene.direction = IS_forward;
    }
}


#if 0
//------------------------------------------------------自动定义效果：流星效果
void custom_effect(void)
{
    extern uint16_t WS2812FX_mode_comet_2(void);
    extern uint16_t WS2812FX_mode_comet_3(void);

    //流星效果
    if( fc_effect.metemor_effect_index==1||fc_effect.metemor_effect_index==2||fc_effect.metemor_effect_index==3||fc_effect.metemor_effect_index==4)
    {

        WS2812FX_stop();

        // FADE_SLOW：12颗
        // FADE_MEDIUM：6颗
        // FADE_FAST：5颗灯
        // FADE_XFAST:3颗灯
        WS2812FX_setSegment_colorOptions(
            1,                                          //第1段
            1,fc_effect.led_num - 1 ,                      //起始位置，结束位置
            &WS2812FX_mode_comet_1,                     //效果  单灯色流程效果
            WHITE,                                      //颜色，WS2812FX_setColors设置
            fc_effect.speed,                            //速度
            fade_type[fc_effect.metemor_effect_index-1] | 0);             //选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_start();

    }
    else if( fc_effect.metemor_effect_index==5||fc_effect.metemor_effect_index==6||fc_effect.metemor_effect_index==7||fc_effect.metemor_effect_index==8)
    {

        WS2812FX_stop();  //库会停止向DMA写数据

        // FADE_SLOW：12颗
        // FADE_MEDIUM：6颗
        // FADE_FAST：5颗灯
        // FADE_XFAST:3颗灯
        WS2812FX_setSegment_colorOptions(
            1,                                          //第0段
            1,fc_effect.led_num - 1,                      //起始位置，结束位置
            &WS2812FX_mode_comet_1,                     //效果
            WHITE,                                      //颜色，WS2812FX_setColors设置
            fc_effect.speed,                            //速度
            fade_type[fc_effect.metemor_effect_index-5] | REVERSE);       //选项，这里像素点大小：3 REVERSE决定方向


        WS2812FX_start();



    }
    else if(fc_effect.metemor_effect_index == 9)
    {

        WS2812FX_stop();  //库会停止向DMA写数据sjf

        WS2812FX_setSegment_colorOptions(
            1,                                          //第0段
            1,fc_effect.led_num - 1,                      //起始位置，结束位置
            &WS2812FX_mode_comet_2,                     //效果
            WHITE,                                      //颜色，WS2812FX_setColors设置
            fc_effect.speed,                            //速度
            fade_type[0] | 0);             //选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_start();

    }
    else if(fc_effect.metemor_effect_index == 10)
    {

        WS2812FX_stop();  //库会停止向DMA写数据sjf

        WS2812FX_setSegment_colorOptions(
            1,                                          //第0段
            1,fc_effect.led_num - 1,                      //起始位置，结束位置
            &WS2812FX_mode_comet_3,                     //效果
            WHITE,                                      //颜色，WS2812FX_setColors设置
            fc_effect.speed,                            //速度
            fade_type[0] | 0);             //选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_start();


    }
    else if(fc_effect.metemor_effect_index == 11)
    {

        extern uint16_t music_mode1(void);
        extern uint16_t meteor(void);
        extern uint16_t meteor1(void);
        WS2812FX_setBrightness( 255 );
       extern uint16_t mode5(void);
        WS2812FX_stop();

        // FADE_SLOW：12颗
        // FADE_MEDIUM：6颗
        // FADE_FAST：5颗灯
        // FADE_XFAST:3颗灯
        WS2812FX_setSegment_colorOptions(
            1,                                          //第0段
            1,fc_effect.led_num - 1,                      //起始位置，结束位置
            // &meteor1,                               //效果
            &mode5,
            WHITE,                                    //颜色，WS2812FX_setColors设置
            fc_effect.speed,                          //速度
            0);                                     //选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_start();

    }
    else if(fc_effect.metemor_effect_index ==12)
    {

        extern uint16_t music_meteor3(void);
        WS2812FX_stop();

        // FADE_SLOW：12颗
        // FADE_MEDIUM：6颗
        // FADE_FAST：5颗灯
        // FADE_XFAST:3颗灯
        WS2812FX_setSegment_colorOptions(
            1,                                           //第0段
            1,fc_effect.led_num - 1,                      //起始位置，结束位置
            &music_meteor3,                            //效果
            WHITE,                                    //颜色，WS2812FX_setColors设置
            fc_effect.speed,                         //速度
            0);                                     //选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_start();

    }
    save_user_data_area3();//保存参数配置到flash
}
#endif

u8 get_meteor_on_off(void)
{

        return fc_effect.metemor_on_off;
}
//------------------------------------------------------自动定义效果：天奕流星效果
void custom_meteor_effect(void)
{
    printf("custom_meteor_effect");
    fc_effect.period_cnt = 0;
    if(fc_effect.metemor_on_off == 0x01)
    {
        //流星效果                                    单流星
        if( fc_effect.metemor_effect_index == 1)
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_1,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                fade_type[0] | 0);                      //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 2)  //单流星
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_1,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                fade_type[0] | REVERSE);                //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 3)  //双流星
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &fc_double_meteor,                          //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                fade_type[0] | 0);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 4)  //双流星
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &fc_double_meteor,                         //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                fade_type[0] | REVERSE);                               //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 5)   //频闪效果
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_3,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
          
        }
        else if(fc_effect.metemor_effect_index == 6)  //频闪效果
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_3,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                REVERSE);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
            
        }
        else if(fc_effect.metemor_effect_index == 7)
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &meteor_effect_G,                          //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 8)
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &meteor_effect_H,                          //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 9)  //堆积
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_4,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 10)  //堆积
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_4,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                REVERSE);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 11)   //逐点流水
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_5,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 12) //逐点流水
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_5,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
            REVERSE);                                   //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 13)   //中心靠拢
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_2,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
            fade_type[0] | 0);                          //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 14) //中心发撒
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_2,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                fade_type[0] | REVERSE);                //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 15)   //追逐流水
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_6,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 16) //追逐流水
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &WS2812FX_mode_comet_6,                    //效果
                WHITE,                                    //颜色
                fc_effect.speed,                         //速度
            REVERSE);                                   //选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if(fc_effect.metemor_effect_index == 17)   //音乐律动1
        {
            extern uint16_t meteor(void);
            WS2812FX_stop();

            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &meteor,                                   //效果
                WHITE,                                    //颜色，WS2812FX_setColors设置
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向

            WS2812FX_start();

        }
        else if(fc_effect.metemor_effect_index == 18)   //音乐律动2
        {
            extern uint16_t music_meteor3(void);
            WS2812FX_stop();

            WS2812FX_setSegment_colorOptions(
                1,                                           //第0段
                1,fc_effect.led_num,                      //起始位置，结束位置
                &music_meteor3,                            //效果
                WHITE,                                    //颜色，WS2812FX_setColors设置
                fc_effect.speed,                         //速度
                0);                                     //选项，这里像素点大小：3 REVERSE决定方向

            WS2812FX_start();

        }

        else if(fc_effect.metemor_effect_index == 19 || fc_effect.metemor_effect_index == 20 || fc_effect.metemor_effect_index == 21)   //
        {
            strand_meteor(fc_effect.metemor_effect_index);

        }
        else if(fc_effect.metemor_effect_index == 22)
        {
            double_meteor();
        }


    }
    save_user_data_area3();//保存参数配置到flash
}


void flash_printf(void)
{
    printf("fc_effect.on_off_flag  = %d",fc_effect.on_off_flag );
    printf("fc_effect.Now_state  = %d",fc_effect.Now_state );

    printf("fc_effect.rgb.r = %d",fc_effect.rgb.r);
    printf("fc_effect.rgb.g = %d",fc_effect.rgb.g);
    printf("fc_effect.rgb.b = %d",fc_effect.rgb.b);
    printf("fc_effect.w = %d",fc_effect.w);
    printf("fc_effect.b  = %d",fc_effect.b );


}



/***************************************************软件关机*****************************************************/
//附加功能的变量赋值
void external_devices_variable(void)
{
    extern u8 music_trigger;  //控制七彩灯的声控定色变换效果
    fc_effect.metemor_on_off  = 0x02;  //流星关机
    music_trigger = 0;           //声控
    extern u8 counting_flag ;
    extern u8 set_time;
    counting_flag = 1;        //无霍尔时，电机
    set_time = 1;

}

void soft_rurn_off_lights(void) //软关灯处理
{
    
    fc_effect.on_off_flag = DEVICE_OFF;
    external_devices_variable();   //附加功能的控制变量
    WS2812FX_stop();
    WS2812FX_strip_off();   // 从WS2812FX_stop() 搬出来，
    save_user_data_area3();//保存参数配置到flash
    close_fan();  //关闭风扇
    //关闭RGBW灯，这个设计时因为有W的控制灯
    mcpwm_set_duty(pwm_ch0, 0);
    mcpwm_set_duty(pwm_ch1, 0);
    mcpwm_set_duty(pwm_ch2, 0);
    mcpwm_set_duty(pwm_ch3, 0);
    fb_led_on_off_state();    //与app同步开关状态
    printf("soft_rurn_off_light!!\n");

}
/**************************************************软件开机*****************************************************/
 void soft_turn_on_the_light(void)   //软开灯处理
{

    //flash_printf();
    fc_effect.on_off_flag = DEVICE_ON;
    fc_effect.metemor_on_off  = 0x01;
    save_user_data_area3();  //保存参数配置到flash
    WS2812FX_start();        
    one_wire_set_mode(4);    //360正转
    enable_one_wire();       //启动发送电机数据
    open_fan();             //开启风扇
    fb_led_on_off_state();  //与app同步开关状态
    
    printf("soft_turn_on_the_light!!\n");
  //  flash_printf();
}






ON_OFF_FLAG get_on_off_state(void)
{
    return fc_effect.on_off_flag;
}

void set_on_off_led(u8 on_off)
{
    fc_effect.on_off_flag = on_off;
    if(fc_effect.on_off_flag == DEVICE_ON)
    {
        soft_turn_on_the_light();  //开灯
    }
    else
    {
        soft_rurn_off_lights();  //关灯

    }
}


void set_on_off_meteor(u8 on_off)
{

    fc_effect.metemor_on_off = on_off;
    if(fc_effect.metemor_on_off == 0x01)
    {

         custom_meteor_effect();
    }

    else
    {

        WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(
            1,                                           //第0段
            1,fc_effect.led_num-1,                      //起始位置，结束位置
            &close_metemor,                          //效果
            0,                                    //颜色
            fc_effect.speed,                         //速度
            0);                                     //选项，这里像素点大小：3 REVERSE决定方向
        WS2812FX_start();

    }


}



/*-------------------------------------------声控----------------------------------------*/
void music_mode_plus(void)
{

    fc_effect.music.m++;
    fc_effect.music.m %= 12;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void music_mode_sub(void)
{
    if(fc_effect.music.m > 0)
        fc_effect.music.m--;
    else fc_effect.music.m = 11;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void set_music_mode(u8 m)
{
    printf("\n set_music_mode = %d", m);
    fc_effect.music.m = m;
    fc_effect.Now_state = IS_light_music;

    fb_led_music_mode();
    save_user_data_area3();
    set_fc_effect();
}

void set_music_type(u8 ty)
{
    fc_effect.music.m_type = ty;

}

void set_music_sensitive(u8 s)
{
    printf("\n music sensitive = %d", s);

    fc_effect.music.s = s;

}

void fc_music(void)
{
    extern uint16_t music_meteor(void);

    //频谱
    extern uint16_t music_fs(void) ;
    extern uint16_t music_fs_bc(void) ;
    extern uint16_t music_fs_green_blue(void) ;
    // 节奏
    extern uint16_t music_2_side_oc(void) ;
    extern uint16_t music_oc_2(void);
    extern uint16_t music_rainbow_flash(void);


    // 滚动
    extern uint16_t music_energy(void);
    extern uint16_t music_multi_c_flow(void) ;
    extern uint16_t music_meteor(void);


    // 能量
    extern uint16_t music_star(void) ; //七彩

    // extern void set_music_s_m(u8 m);

    void *p;
    switch(fc_effect.music.m)
    {
        case 0: //能量1
            // set_music_s_m(0);
            // p = &music_star;
            extern uint16_t fc_music_gradual(void);
            p = &fc_music_gradual;
            break;
        case 1: //能量2
            // set_music_s_m(1);
            // p = &music_star;
            extern uint16_t fc_music_breath(void);
            p = &fc_music_breath;
            break;
        case 2: //能量3
            // set_music_s_m(2);
            // p = &music_star;
            extern uint16_t fc_music_static(void);
            p = &fc_music_static;
            break;

        case 3://节奏1
            // p = &music_2_side_oc;
            extern uint16_t fc_music_twinkle(void);
            p = &fc_music_twinkle;
            break;

        case 4://节奏2
            p = &music_oc_2;
            break;

        case 5://节奏3
            p = &music_rainbow_flash;
            break;

        case 6://频谱1
            p = &music_fs;
            break;
        case 7://频谱2
            p = &music_fs_bc;
            break;
        case 8://频谱3
            p = &music_fs_green_blue;
            break;

        case 9://滚动1
            p = &music_energy;
            break;
        case 10://滚动2
            p = &music_multi_c_flow;
            break;
        case 11://滚动3
            p = &music_meteor;
            break;

    }

    WS2812FX_stop();

    WS2812FX_setSegment_colorOptions(
        0,                          //第0段
        0,fc_effect.led_num-1,                       //起始位置，结束位置
        p,              //效果
        WHITE,                      //颜色，WS2812FX_setColors设置
        fc_effect.music.s,                        //速度
        SIZE_MEDIUM);               //选项，这里像素点大小：3,反向/反向
    WS2812FX_start();

}






/**************************************************静态模式*****************************************************/
void set_static_mode(u8 r, u8 g, u8 b)
{
    fc_effect.Now_state = IS_STATIC;
    fc_effect.w = 0;
    fc_effect.rgb.r = r;
    fc_effect.rgb.g = g;
    fc_effect.rgb.b = b;
    set_fc_effect();  //效果调度
}


//静态效果
static void static_mode(void)
{
    printf("static_mode");
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(           //设置一段颜色的效果
        0,                                      //第0段
        0,0,                                    //起始位置，结束位置
        &WS2812FX_mode_static,                  //效果
        0,                                      //颜色，WS2812FX_setColors设置
        1000,                                      //速度
        0);                                     //选项，这里像素点大小：1
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);  // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
    ls_set_colors(1, &fc_effect.rgb);   //1:1个颜色    &fc_effect.rgb 这个颜色是什么色
    WS2812FX_start();
}


#if 0
/******************************************************************
 * 函数：更新涂抹效果数据
 * 形参1：tool       油桶、画笔、橡皮擦
 * 形参2：colour     hsv颜色
 * 形参3：led_place  灯点位置（0~47）
 * 返回：无
 *
 * 注：若选择IS_drum油桶，led_place参数无效
 *     若选择IS_eraser橡皮擦，colour参数无效，内部将colour设为黑色
 *****************************************************************/
void effect_smear_adjust_updata(smear_tool_e tool, hsv_t *colour,unsigned short *led_place)
{
    unsigned char num = 0;
    unsigned char max;

    //更新为涂抹功能状态
    fc_effect.Now_state = IS_smear_adjust;

    //更新工具
    fc_effect.smear_adjust.smear_tool = tool;
    printf("fc_effect.smear_adjust.smear_tool = %d\r\n",(uint8_t)fc_effect.smear_adjust.smear_tool);
    printf("\r\n");

    //清除rgb[0~n]数据
    // memset(fc_effect.smear_adjust.rgb, 0, sizeof(fc_effect.smear_adjust.rgb));

    /*HSV转换RGB*/
    if(fc_effect.smear_adjust.smear_tool == IS_drum) //油桶
    {
      m_hsv_to_rgb(&fc_effect.smear_adjust.rgb[0].r,
                   &fc_effect.smear_adjust.rgb[0].g,
                   &fc_effect.smear_adjust.rgb[0].b,
                   colour->h_val,
                   colour->s_val,
                   colour->v_val);
      max = fc_effect.led_num;
      for(num = 1; num < max; ++num)
      {
        fc_effect.smear_adjust.rgb[num].r = fc_effect.smear_adjust.rgb[0].r;
        fc_effect.smear_adjust.rgb[num].g = fc_effect.smear_adjust.rgb[0].g;
        fc_effect.smear_adjust.rgb[num].b = fc_effect.smear_adjust.rgb[0].b;

        // printf("fc_effect.smear_adjust.rgb[%d].r = %d\r\n", num,fc_effect.smear_adjust.rgb[num].r);
        // printf("fc_effect.smear_adjust.rgb[%d].g = %d\r\n", num,fc_effect.smear_adjust.rgb[num].g);
        // printf("fc_effect.smear_adjust.rgb[%d].b = %d\r\n", num,fc_effect.smear_adjust.rgb[num].b);
        // printf("\r\n");
      }
    }
    else if((fc_effect.smear_adjust.smear_tool == IS_pen) ||   //画笔
            (fc_effect.smear_adjust.smear_tool == IS_eraser))  //橡皮擦
    {
        m_hsv_to_rgb(&fc_effect.smear_adjust.rgb[*led_place].r,
                     &fc_effect.smear_adjust.rgb[*led_place].g,
                     &fc_effect.smear_adjust.rgb[*led_place].b,
                     colour->h_val,
                     colour->s_val,
                     colour->v_val);

        // printf("fc_effect.smear_adjust.rgb[%d].r = %d\r\n", *led_place, fc_effect.smear_adjust.rgb[dp_draw_tool.led_place].r);
        // printf("fc_effect.smear_adjust.rgb[%d].g = %d\r\n", *led_place, fc_effect.smear_adjust.rgb[dp_draw_tool.led_place].g);
        // printf("fc_effect.smear_adjust.rgb[%d].b = %d\r\n", *led_place, fc_effect.smear_adjust.rgb[dp_draw_tool.led_place].b);
        // printf("\r\n");
    }
}
/*----------------------------------涂抹模式----------------------------------*/
extern  Segment* _seg;
extern  uint16_t _seg_len;
extern Segment_runtime* _seg_rt;

/*******************************************
灯串涂抹效果实现方法
 ******************************************/
static uint16_t ls_smear_adjust_effect(void)
{
  unsigned char num;
  unsigned char max = fc_effect.led_num;
  if(max >= _seg_len) max = _seg_len;
  for(num = 0; num < max; ++num)
  {
      WS2812FX_setPixelColor_rgb(num,
        fc_effect.smear_adjust.rgb[num].r,
        fc_effect.smear_adjust.rgb[num].g,
        fc_effect.smear_adjust.rgb[num].b);
  }
  return _seg->speed;
}

static void fc_smear_adjust(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&ls_smear_adjust_effect,BLUE,100,0);
    WS2812FX_start();
}
#endif
/*----------------------------------涂鸦配网效果----------------------------------*/
static void fc_pair_effect(void)
{
    extern uint16_t unbind_effect(void);
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&unbind_effect,0,0,0);
    WS2812FX_start();
}

/*----------------------------------情景效果实现----------------------------------*/
static void ls_scene_effect(void)
{
    // printf("\n fc_effect.dream_scene.change_type=%d",fc_effect.dream_scene.change_type);

        switch (fc_effect.dream_scene.change_type)
        {

            case  MODE_MUTIL_RAINBOW:      //彩虹
                strand_rainbow();
            break;

            case MODE_MUTIL_JUMP://跳变模式
                strand_jump_change();
                break;

            case MODE_MUTIL_BRAETH://呼吸模式
                strand_breath();
                break;

            case MODE_MUTIL_TWIHKLE://闪烁模式
                strand_twihkle();

                break;

            case MODE_MUTIL_FLOW_WATER://流水模式
                strand_flow_water();

                break;

            case MODE_CHAS_LIGHT://追光模式
                strand_chas_light();

                break;

            case MODE_MUTIL_COLORFUL://炫彩模式
                strand_colorful();
                break;

            case MODE_MUTIL_SEG_GRADUAL://渐变模式
                strand_grandual();
                break;

            case MODE_JUMP:     //标准跳变
                // standard_jump();
                standart_jump_fix();  //准备天奕客户修改
                break;

            case MODE_MUTIL_C_GRADUAL:  //多段同时渐变
                mutil_c_grandual();
                break;

            case MODE_BREATH_W:    //白色渐变
                w_grandual();
                break;

            case MODE_STROBE:   //标准频闪
                ls_strobe();
                break;

            case MODE_SINGLE_C_BREATH:
                single_c_breath();
                break;
            default:
                break;
        }
   
}


/*----------------------------------彩虹模式----------------------------------*/
static void strand_rainbow(void)
{
    WS2812FX_stop();
    // printf("\n fc_effect.dream_scene.c_n=%d",fc_effect.dream_scene.c_n);
    // printf("\n fc_effect.led_num=%d",fc_effect.led_num);
    // printf("\n fc_effect.dream_scene.speed=%d",fc_effect.dream_scene.speed);
    // printf("\n fc_effect.dream_scene.rgb");
    // printf_buf(fc_effect.dream_scene.rgb, fc_effect.dream_scene.c_n*sizeof(color_t));
    // printf("\n fc_effect.dream_scene.direction=%d",fc_effect.dream_scene.direction);

    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_fade,               //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------跳变模式----------------------------------*/
void strand_jump_change(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mode_single_block_scan,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

// 标准跳变，整体颜色跳变
void standard_jump(void)
{
    extern uint16_t WS2812FX_mutil_c_jump(void);
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mutil_c_jump,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}

//针对天奕光纤灯修改的跳变
void standart_jump_fix(void)
{
    extern uint16_t WS2812FX_mutil_c_jump(void);
    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mutil_c_jump,       //效果
        0,                                      //颜色，WS2812FX_setColors设置
        (fc_effect.dream_scene.speed * 40),            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();

}



/*----------------------------------呼吸模式----------------------------------*/
void strand_breath(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_breath,            //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------闪烁模式----------------------------------*/
void strand_twihkle(void)
{
    uint8_t option;
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_twihkle,           //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();

}

//多颜色频闪
void ls_strobe(void)
{
    extern uint16_t WS2812FX_mutil_strobe(void);

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                                    //起始位置，结束位置
        &WS2812FX_mutil_strobe,                 //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}


/************************  流星模式  正反方向***************************************/
void strand_meteor(u8 index)
{

    uint8_t option;
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else{
        option = REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        1,                                      //第0段
        1,fc_effect.led_num,                                    //起始位置，结束位置
        &WS2812FX_mode_comet_1,                          //效果
        WHITE,                                      //颜色，WS2812FX_setColors设置
        fc_effect.speed,            //速度
        fade_type[index - 19] | option);                                //选项，这里像素点大小：3,反向/反向
    WS2812FX_start();

}

void double_meteor(void)
{

    extern uint16_t fc_double_meteor(void);
    uint8_t option;
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else{
        option = REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        1,                                      //第0段
        1,fc_effect.led_num,                                    //起始位置，结束位置
        &fc_double_meteor,                          //效果
        WHITE,                                      //颜色，WS2812FX_setColors设置
        fc_effect.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_start();

}


/*----------------------------------流水模式----------------------------------*/
/*---------标准正反方向参考效果------------------------------------------------*/
void strand_flow_water(void)
{
    uint8_t option;
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else{
        option = SIZE_MEDIUM | REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mode_multi_block_scan,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        option);                                //选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------追光模式----------------------------------*/
void strand_chas_light(void)
{
    printf("\n fc_effect.dream_scene.c_n=%d",fc_effect.dream_scene.c_n);
    printf("\n fc_effect.led_num=%d",fc_effect.led_num);
    printf("\n fc_effect.dream_scene.speed=%d",fc_effect.dream_scene.speed);
    printf("\n fc_effect.dream_scene.rgb");
    printf_buf(fc_effect.dream_scene.rgb, fc_effect.dream_scene.c_n*sizeof(color_t));
    printf("\n fc_effect.dream_scene.direction=%d",fc_effect.dream_scene.direction);
    WS2812FX_stop();
    // 正向
    if(fc_effect.dream_scene.direction == IS_forward)
    {
        WS2812FX_setSegment_colorOptions(
            0,                                      //第0段
            0,0,                  //起始位置，结束位置
            &WS2812FX_mode_multi_forward_same,        //效果
            0,                                      //颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,            //速度
            0);                                     //选项
    }
    else
    {
        WS2812FX_setSegment_colorOptions(
            0,                                      //第0段
            0,0,                  //起始位置，结束位置
            &WS2812FX_mode_multi_back_same,        //效果
            0,                                      //颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,            //速度
            0);
    }
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------炫彩模式----------------------------------*/
void strand_colorful(void)
{
    uint8_t option;
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mode_multi_block_scan,        //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_SMALL);                            //选项，这里像素点大小：1
    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------渐变模式----------------------------------*/
void strand_grandual(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mode_mutil_fade,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                                //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}


// 整条灯带渐变，支持多种颜色之间切换
// 颜色池：fc_effect.dream_scene.rgb[]
// 颜色数量fc_effect.dream_scene.c_n
void mutil_c_grandual(void)
{
    extern uint16_t WS2812FX_mutil_c_gradual(void);

    //WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &WS2812FX_mutil_c_gradual,              //效果
        0,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        SIZE_MEDIUM);                           //选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}


//w通道渐变
void w_grandual(void)
{

    extern uint16_t breath_w(void) ;

    WS2812FX_setSegment_colorOptions(
        0,                                      //第0段
        0,0,                  //起始位置，结束位置
        &breath_w,                              //效果
        WHITE,                                      //颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,            //速度
        0);                                     //选项，这里像素点大小：3,反向/反向

    WS2812FX_start();
}

//
void single_c_breath(void)
{

    extern uint16_t WS2812FX_mode_breath(void) ;
    WS2812FX_setSegment_colorOptions(
    0,                                      //第0段
    0,0,                  //起始位置，结束位置
    &WS2812FX_mode_breath,            //效果
    0,                                      //颜色，WS2812FX_setColors设置
    fc_effect.dream_scene.speed,            //速度
    SIZE_MEDIUM);                           //选项，这里像素点大小：3

    WS2812FX_set_coloQty(0,fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}


// 触发提示效果，白光闪烁
void run_white_tips(void)
{
    extern uint16_t white_tips(void);
    WS2812FX_setSegment_colorOptions(0,0,fc_effect.led_num-1,&white_tips,0,0,0);
    WS2812FX_start();
}



/*----------------------------------API----------------------------------*/

// --------------------------------------速度
const uint16_t speed_map[]=
{
    // 30,
    // 100,
    // 150,
    // 200,
    // 250,
    // 300
    0,
    25,
    50,
    75,
    100

};
u8 speed_index=0;

u16 get_max_sp(void)
{
    u16 s;
    s = fc_effect.led_num*30/1000; //每个LED30us
    if(s<10) s= 10;
    return s; //

}
//---------------------------------------灵敏度

void ls_sensitive_plus(void)
{
    if( fc_effect.sound.sensitive < 100-10 )
    {
        fc_effect.sound.sensitive += 10;
    }
    else{
        fc_effect.sound.sensitive = 100;

    }
//    printf("fc_effect.music.s = %d", fc_effect.music.s);

    set_fc_effect();
}

void ls_sensitive_sub(void)
{

    if( fc_effect.sound.sensitive > 10 )
    {
        fc_effect.sound.sensitive-= 10;
    }
    else
    {
        fc_effect.sound.sensitive = 10;

    }
   
// printf("fc_effect.music.s = %d", fc_effect.music.s);
    set_fc_effect();
}


/*********************************************************/
//速度调节
// s:1%-100%
void ls_set_speed(uint8_t s)
{

    // fc_effect.speed = s;
    fc_effect.dream_scene.speed =  500 - (500 * s / 100);
    if(fc_effect.dream_scene.speed <= get_max_sp())
    {
        fc_effect.dream_scene.speed = get_max_sp();
    }
  set_fc_effect();
   
}

void ls_speed_plus(void)
{

      // if( speed_index < 10 )
    // {
    //     speed_index++;
    // }
    // ls_set_speed(speed_map[speed_index]);
   if(   fc_effect.dream_scene.speed > 50 )
    {
          fc_effect.dream_scene.speed -= 50;
    }
    else
    {
         fc_effect.dream_scene.speed = 10;

    }


// printf("fc_effect.dream_scene.speed= %d", fc_effect.dream_scene.speed);
    set_fc_effect();

}

void ls_speed_sub(void)
{


    if( fc_effect.dream_scene.speed< 500-50 )
    {
         fc_effect.dream_scene.speed += 50;
    }
    else{
         fc_effect.dream_scene.speed = 500;

    }   

// printf("fc_effect.dream_scene.speed= %d", fc_effect.dream_scene.speed);

    set_fc_effect();


}

// --------------------------------------播放
// 继续播放
void ls_play(void)
{
    WS2812FX_play();
}
// 暂停
void ls_pause(void)
{
    WS2812FX_pause();
}

// --------------------------------------流星灯周期
void set_meteor_p(u8 p)
{
    if(p >= 2 && p <= 20)
    {
        fc_effect.meteor_period = p;
        fc_effect.period_cnt = 0;
    }
}

// 时间递减
// sub:减数，ms
//放在了while循环，10ms减一次
// fc_effect.meteor_period = 8;//默认8秒  周期值
// fc_effect.period_cnt = fc_effect.meteor_period*1000;  //ms,运行时的计数器 8000ms
void meteor_period_sub(void)
{

    if(fc_effect.period_cnt > 10)
    {
        fc_effect.period_cnt -= 10;
    }
    else{
        fc_effect.period_cnt = 0;   //计数器清零
        if(fc_effect.mode_cycle)    //模式循环完成，更新
        {
            fc_effect.period_cnt = fc_effect.meteor_period*1000;
            fc_effect.mode_cycle = 0;
        }
    }
}

// 0:计时完成
// 1：计时中
u8 get_effect_p(void)
{
    if(fc_effect.period_cnt > 0) return 1;
    else return 0;
}

/* *********************************样式 */
// 自定义样式
void fc_set_style_custom(void)
{
    fc_effect.Now_state = ACT_CUSTOM;
}



// 涂鸦配对样式
void fc_set_style_ty_pair(void)
{
    fc_effect.Now_state = ACT_TY_PAIR;
}


// ------------------------------------------------亮度  0-100
// 0-100
void set_bright(u8 b)
{
    if(b == 0) b = 10;
    fc_effect.b = 255*b/100;

    WS2812FX_setBrightness(fc_effect.b);
}

void bright_plus(void)
{
    if(fc_effect.b < 255 - 50)
    {
        fc_effect.b += 50;
    }
    else
    {
        fc_effect.b = 255;
        // run_white_tips();
    }
    WS2812FX_setBrightness(fc_effect.b);
}

void bright_sub(void)
{
    if(fc_effect.b > 50)
    {
        fc_effect.b -= 50;
    }
    else
    {
        fc_effect.b = 10;
        // run_white_tips();
    }
    if(fc_effect.b <= 10)
    {
        fc_effect.b = 10;
    }
    WS2812FX_setBrightness(fc_effect.b);
}
void updata_sp(void)
{
    if(get_max_sp() > fc_effect.dream_scene.speed)
    {
        fc_effect.dream_scene.speed = get_max_sp();
        printf("\n updata_sp=%d",get_max_sp());
    }
}
// ------------------------------------------------灯带长度
void set_ls_lenght(u16 l)
{
    if(l>16)
        l=16;
    fc_effect.led_num = l;
    WS2812FX_stop();
    WS2812FX_init(fc_effect.led_num,fc_effect.sequence);
    WS2812FX_setBrightness( fc_effect.b );
    WS2812FX_start();
    updata_sp();

    set_fc_effect();

}


#define MAX_STATIC_N    11
// 静态效果颜色map
const u32 fc_static_map[MAX_STATIC_N] =
{
    RED,    //0
    GREEN,  //1
    BLUE,   //2
    WHITE,  //3
    YELLOW, //4
    CYAN,   //5
    MAGENTA,//6
    PURPLE, //7
    ORANGE, //8
    PINK,   //9
    GRAY,
};


void set_IS_light_scene_state(void)
{
    fc_effect.Now_state = IS_light_scene;

}

// 利用fc_effect结构体，构建内置效果
void fc_static_effect(u8 n)
{
    fc_effect.Now_state = IS_STATIC;
    fc_effect.dream_scene.c_n = 1;
    if(is_rgbw)
    {
        if(fc_static_map[n] != WHITE )
        {
            fc_effect.rgb.r = (fc_static_map[n] >> 16) & 0xff;
            fc_effect.rgb.g = (fc_static_map[n] >> 8) & 0xff;
            fc_effect.rgb.b = (fc_static_map[n] ) & 0xff;
            fc_effect.w = 0;

        }
        else
        {
        fc_effect.b = 255;
        fc_effect.w = 255;
        }
    }
    else
    {
        fc_effect.rgb.r = (fc_static_map[n] >> 16) & 0xff;
        fc_effect.rgb.g = (fc_static_map[n] >> 8) & 0xff;
        fc_effect.rgb.b = (fc_static_map[n] ) & 0xff;
    }

  save_user_data_area3();
    set_fc_effect();
}


// 设置fc_effect.dream_scene.rgb的颜色池
// n:0-MAX_NUM_COLORS
// c:WS2812FX颜色系，R<<16,G<<8,B在低8位
void ls_set_color(uint8_t n, uint32_t c)
{
    if(n < MAX_NUM_COLORS)
    {
        fc_effect.dream_scene.rgb[n].r = (c>>16) & 0xff;
        fc_effect.dream_scene.rgb[n].g = (c>>8) & 0xff;
        fc_effect.dream_scene.rgb[n].b = c & 0xff;
    }
}

// ------------------------------------------------W通道
void set_w(u8 w)
{

    fc_effect.w = w;
    fc_effect.Now_state = IS_STATIC;
    fc_effect.rgb.r = 0;
    fc_effect.rgb.g = 0;
    fc_effect.rgb.b = 0;
    // printf("w = %d", fc_effect.w );
    // printf("b = %d", fc_effect.b );
    set_fc_effect();
}

extern void WS2812FX_trigger();
extern u8 get_sound_result(void);
u8 music_trigger = 0;
/**
 * @brief 定色切换灯色，声控，
 *
 */
void music_static_sound(void)
{

    if(get_sound_result())  //采集声音有效
    {

        //七彩灯的声控
        if(fc_effect.on_off_flag == DEVICE_ON && fc_effect.Now_state == IS_light_music)
        {

          //  WS2812FX_trigger();
            music_trigger = 1;
        }

    }

}

/**********************************  中道闹钟  *******************************************************/
u8 set_week[ALARM_NUMBER][7];
ALARM_CLOCK alarm_clock[3];
TIME_CLOCK time_clock;
void set_zd_countdown_state(u8 s,u8 index)
{

     zd_countdown[index].set_on_off = s;   //计时开关或者闹钟开关

}

/**
 * @brief 闹钟数据出解析
 *
 * @param index
 */
void parse_alarm_data(int index)
{

    /*解析循环星期*/
    u8 p_mode;
    p_mode = alarm_clock[index].mode;
    for(int i = 0; i < 7; i++)
    {
        if(p_mode & 0x01)
            set_week[index][i] =  i + 1;

        else
            set_week[index][i] = 0;
        p_mode = p_mode >> 1;

    }


    if(alarm_clock[index].on_off == 0x80)  //闹钟开
    {
        printf("open alarm");
        set_zd_countdown_state(DEVICE_ON,index);    //开启闹钟
    }

    if(alarm_clock[index].on_off == 0x00)  //闹钟关
    {
        printf("close alarm");
        set_zd_countdown_state(DEVICE_OFF,index);  //关闭闹钟
    }



}

/**
 * @brief 关闭中道闹钟
 *
 */
void close_alarm(int index)
{
    uint8_t Send_buffer[6];        //发送缓存

    zd_countdown[index].set_on_off = 0;
    alarm_clock[index].on_off = 0;
    Send_buffer[0] = 0x05;
    Send_buffer[1] = index;
    Send_buffer[2] = alarm_clock[index].hour;
    Send_buffer[3] = alarm_clock[index].minute;
    Send_buffer[4] = 0;
    Send_buffer[5] = alarm_clock[index].mode;
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 6);
}




/**
 * @brief 闹钟处理
 *
 * @param index 闹钟编号
 */

void countdown_handler(int index)
{
    if( zd_countdown[index].set_on_off)  //闹钟开启
    {

        if(time_clock.hour == alarm_clock[index].hour &&  time_clock.minute == alarm_clock[index].minute)
        {

            if((alarm_clock[index].mode & 0x7f) == 0)  //没有星期
            {
                if((alarm_clock[index].mode >> 7))  //闹钟设置里灯的状态
                {
                    soft_turn_on_the_light();
                    close_alarm(index);
                    save_user_data_area3();
                }
                else
                {
                    soft_rurn_off_lights();
                    close_alarm(index);
                    save_user_data_area3();
                }


            }
            else
            {

                for(int i = 0; i < 7; i++)
                {

                    if(time_clock.week == set_week[index][i])
                    {


                        if((alarm_clock[index].mode >> 7))
                        {

                            soft_turn_on_the_light();
                            close_alarm(index);
                            save_user_data_area3();
                        }
                        else
                        {
                            soft_rurn_off_lights();
                            close_alarm(index);
                            save_user_data_area3();
                        }

                    }

                }


            }


        }

    }
}



u8 calculate_ms = 0;
/**
 * @brief 放在主函数的while中，10ms调用一次
 *
 */
void time_clock_handler(void)
{

    calculate_ms++;
    if(calculate_ms == 100)   //秒
    {
        calculate_ms = 0;
        time_clock.second++;

        if(time_clock.second == 60)  //分
        {
            time_clock.second = 0;
            time_clock.minute++;

            if(time_clock.minute == 60) //时
            {
                time_clock.minute = 0;
                time_clock.hour++;

                if(time_clock.hour == 24) //日
                {
                    time_clock.hour = 0;
                    time_clock.week++;
                    if(time_clock.week == 8)  //周
                    {
                        time_clock.week = 1;
                    }

                }
            }

        }
        // printf("time_clock.hour  = %d",time_clock.hour );
        // printf("time_clock.minute  = %d",time_clock.minute );
        // printf("time_clock.second  = %d",time_clock.second );
        // printf("time_clock.week  = %d",time_clock.week );

    }

    countdown_handler(0);  //闹钟0
    countdown_handler(1);
    countdown_handler(2);

}


// 利用fc_effect结构体，构建动态模式效果
void fc_dynamic_effect(u8 n)
{

    switch(n)
    {
        case 0x07:  //3色跳变
            ls_set_color(0, BLUE);
            ls_set_color(1, GREEN);
            ls_set_color(2, RED);
            fc_effect.dream_scene.change_type = MODE_JUMP;
            fc_effect.dream_scene.c_n = 3;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x08:  //7色跳变
            ls_set_color(0, BLUE);
            ls_set_color(1, GREEN);
            ls_set_color(2, RED);
            ls_set_color(3, WHITE);
            ls_set_color(4, YELLOW);
            ls_set_color(5, CYAN);
            ls_set_color(6, PURPLE);
            fc_effect.dream_scene.change_type = MODE_JUMP;
            fc_effect.dream_scene.c_n = 7;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x09:  //3色渐变
            ls_set_color(0, BLUE);
            ls_set_color(1, GREEN);
            ls_set_color(2, RED);
            fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
            fc_effect.dream_scene.c_n = 3;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x0A:  //七彩渐变
            ls_set_color(0, BLUE);
            ls_set_color(1, GREEN);
            ls_set_color(2, RED);
            ls_set_color(3, WHITE);
            ls_set_color(4, YELLOW);
            ls_set_color(5, CYAN);
            ls_set_color(6, PURPLE);
            fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
            fc_effect.dream_scene.c_n = 7;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x0B:
            ls_set_color(0, RED);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x0c:
            ls_set_color(0, BLUE);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;
        case 0x0D:
            ls_set_color(0, GREEN);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x0E:
            ls_set_color(0, CYAN);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x0F:
            ls_set_color(0, YELLOW);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x10:
            ls_set_color(0, PURPLE);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x11:                             //混白色渐变
            ls_set_color(0, WHITE);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x12:                             //纯白色渐变
            ls_set_color(0, WHITE);
            ls_set_color(1, BLACK);
            fc_effect.dream_scene.change_type = MODE_BREATH_W;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;


        case 0x13:
            ls_set_color(0, RED);
            ls_set_color(1, GREEN);
            fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x14:
            ls_set_color(0, BLUE);
            ls_set_color(1, RED);
            fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x15:
            ls_set_color(0, GREEN);
            ls_set_color(1, BLUE);
            fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
            fc_effect.dream_scene.c_n = 2;
            fc_effect.Now_state = IS_light_scene;
            break;

        case 0x16:  //七色频闪
            ls_set_color(0, BLUE);
            ls_set_color(1, GREEN);
            ls_set_color(2, RED);
            ls_set_color(3, WHITE);
            ls_set_color(4, YELLOW);
            ls_set_color(5, CYAN);
            ls_set_color(6, PURPLE);

            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 7;
            fc_effect.Now_state = IS_light_scene;

            break;

        case 0x17:
            ls_set_color(0, RED);
            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 1;
            fc_effect.Now_state = IS_light_scene;

            break;

        case 0x18:
            ls_set_color(0, BLUE);
            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 1;
            fc_effect.Now_state = IS_light_scene;

            break;

        case 0x19:
            ls_set_color(0, GREEN);
            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 1;
            fc_effect.Now_state = IS_light_scene;

            break;
        case 0x1a:

            ls_set_color(0, CYAN);
            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 1;
            fc_effect.Now_state = IS_light_scene;

            break;

        case 0x1b:

            ls_set_color(0, YELLOW);
            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 1;
            fc_effect.Now_state = IS_light_scene;

            break;
        case 0x1c:

            ls_set_color(0, PURPLE);
            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 1;
            fc_effect.Now_state = IS_light_scene;
            break;
        case 0x1e:
            ls_set_color(0, WHITE);
            fc_effect.dream_scene.change_type = MODE_STROBE;
            fc_effect.dream_scene.c_n = 1;
            fc_effect.Now_state = IS_light_scene;
            break;


    }
    set_fc_effect();

}


// 和通信协议对应
u8 rgb_sequence_map[6]=
{
    NEO_RGB,
    NEO_RBG,
    NEO_GRB,
    NEO_GBR,
    NEO_BRG,
    NEO_BGR,
};
// ------------------------------------------------RGB顺序
// s:0-5
void set_rgb_sequence(u8 s)
{
    if(s < 6)
    {
        fc_effect.sequence = rgb_sequence_map[s];
        //WS2812FX_stop();
        WS2812FX_init(fc_effect.led_num,fc_effect.sequence);
        // custom_index = 2;       //调整RGB顺序效果
        // fc_set_style_custom();  //自定义效果
        fc_effect.Now_state = IS_STATIC;  //当前运行状态 静态

        set_fc_effect();
    }
}


