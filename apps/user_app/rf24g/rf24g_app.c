/*
适用用2.4G遥控
基于中道版本2.4G遥控
1、app_config.h,把宏CONFIG_BT_GATT_CLIENT_NUM设置1
2、apps\spp_and_le\examples\trans_data\ble_trans.c  @bt_ble_init() 加入multi_client_init()
3、@le_gatt_client.c
   __resolve_adv_report()
   HCI_EIR_DATATYPE_MORE_16BIT_SERVICE_UUIDS 加入键值处理函数
4、在key_driver.c 注册rf24g_scan_para
5、board_ac632n_demo_cfg.h 使能TCFG_RF24GKEY_ENABLE
6、@app_tuya.c tuya_key_event_handler()加入上层应用的键值处理函数
7、底层无法判断长按，需要靠上层应用实现

以上该思路方法，在 CONFIG_APP_SPP_LE 这个demo上，实现起来很麻烦，不建议使用
 */


#include "system/includes.h"

#include "task.h"
#include "event.h"
#include "rf24g.h"
#include "led_strip_sys.h"
#include "board_ac632n_demo_cfg.h"
#include "led_strand_effect.h"
 // #include "tuya_ble_type.h"
#include "one_wire.h"
#include "btstack/btstack_typedef.h"
#include "ble_multi_profile.h"
#include "att.h"
#include "asm/mcpwm.h"
// #if TCFG_RF24GKEY_ENABLE
#include "ir_key_app.h"
#include "../../../apps/user_app/led_strip/led_strand_effect.h"

#if 1
#pragma pack (1)
typedef struct
{
    u8 pair[3];
    u8 flag;    //0:表示该数组没使用，0xAA：表示改数组已配对使用
}rf24g_pair_t;
#pragma pack ()
/***********************************************************移植须修改****************************************************************/

#define PAIR_TIME_OUT 5*1000    //3秒
static u16 pair_tc = 0;
// 配对计时，10ms计数一次
void rf24g_pair_tc(void)
{
    if (pair_tc <= PAIR_TIME_OUT)
    {
        pair_tc += 10;
    }
}

#define PAIR_MAX    1

/***********************************************************移植须修改 END****************************************************************/

rf24g_pair_t rf24g_pair[PAIR_MAX];        //需要写flash


/***********************************************************API*******************************************************************/




//-------------------------------------------------效果



// -----------------------------------------------声控


// -----------------------------------------------灵敏度




/***********************************************************APP*******************************************************************/

extern rf24g_ins_t rf24g_ins;
// pair_handle是长按执行，长按时会被执行多次
// 所以执行一次后，要把pair_tc = PAIR_TIME_OUT，避免误触发2次
static void pair_handle(void)
{
    extern void save_rf24g_pair_data(void);
    u8 op = 0;//1:配对，2：解码
    u8 i;
#if 0
    // 开机3秒内
    if (pair_tc < PAIR_TIME_OUT)
    {
        printf("\n pair_tc=%d", pair_tc);
        pair_tc = PAIR_TIME_OUT;//避免误触发2次


        memcpy((u8*)(&rf24g_pair[0].pair), (u8*)(&rf24g_ins.pair), 3);
        rf24g_pair[0].flag = 0xaa;
        save_rf24g_pair_data();
        printf("\n pair");
        printf_buf(&rf24g_pair[0].pair, 3);
        extern void fc_24g_pair_effect(void);
        fc_24g_pair_effect();
        // 查找表是否存在
#if 0
        for (i = 0; i < PAIR_MAX; i++)
        {
            // 和现有客户码匹配,解绑
            if (memcmp((u8*)(&rf24g_pair[i].pair), (u8*)(&rf24g_ins.pair), 3) == 0)
            {
                op = 2;
                pair_tc = PAIR_TIME_OUT;//避免误触发2次，
                rf24g_pair[i].flag = 0;
                rf24g_pair[i].pair[0] = 0;
                rf24g_pair[i].pair[1] = 0;
                rf24g_pair[i].pair[2] = 0;
#warning"save flash"
                printf("\n dis pair");
                break;
            }
        }

        if (i == PAIR_MAX)
        {
            op = 1;
            for (i = 0; i < PAIR_MAX; i++)
            {
                if (rf24g_pair[i].flag == 0)
                {
                    pair_tc = PAIR_TIME_OUT;//避免误触发2次
                    memcpy((u8*)(&rf24g_pair[i].pair), (u8*)(&rf24g_ins.pair), 3);
#warning"save flash"
                    printf("\n pair");
                    printf_buf(&rf24g_pair[i].pair, 3);
                    break;
                }
            }

        }
#endif

    }
#endif
}

u8 off_long_cnt = 0;
extern void parse_zd_data(unsigned char* LedCommand);
extern void set_IS_light_scene_state(void);
u8 all_mode[3] = { 0x04, 0x02, 0x07 };  //j模式集合
u8 sevrn_color_breath[3] = { 0x04, 0x02, 0x0b };  //七色呼吸
u8 stepmotpor_speed_cnt = 0;
u8 dynamic_speed = 0;
u8 meteor_flag;
u8 meteor_music_flag;
u8 meteor_speed[5] = { 1, 25, 50, 75, 100 }; //1, 25, 50, 75, 100
u8 meteor_cycle[5] = { 2, 8, 12, 16, 20 }; //2s 8s 12s 16s 20s
u8 cycle_cntt = 0;
u8 meteor_tail = 0;
u8 meteor_direction = 0;  //0：顺向  1：正向
u8 single_meteor = 0;
u8 fc_music_cnt = 0;
extern u8 Ble_Addr[6]; //蓝牙地址
extern hci_con_handle_t fd_handle;



void rf24_key_handle(struct sys_event* event)
{
    u8 event_type = 0;
    u8 key_value = 0;
    uint8_t Send_buffer[50];        //发送缓存.
    u8 temp[3] = { 0x04, 0x02, 0x08 };
    event_type = event->u.key.event;
    key_value = event->u.key.value;

    // printf("\n event->u.key.type = %d",event->u.key.type);
    // printf("\n event->event_type = %d",event_type);
    printf("\n key_value = %d", key_value);

    if (event->u.key.type == KEY_DRIVER_TYPE_RF24GKEY)  //按键类型
    {

        //开/关
        if (key_value == RF24_ON_OFF && event_type == KEY_EVENT_CLICK)
        {

            if (fc_effect.on_off_flag == DEVICE_ON)
                soft_rurn_off_lights();  //关灯.
            else
                soft_turn_on_the_light();  //开灯
        }

        if (key_value == RFKEY_ON_OFF)
        {
            printf("\n key_value = %d", key_value);

            if (fc_effect.on_off_flag == DEVICE_OFF) soft_turn_on_the_light();  //开灯
            else if (fc_effect.on_off_flag == DEVICE_ON) soft_rurn_off_lights();  //关灯.
        }

        if (get_on_off_state())
        {
            if (
                key_value != RFKEY_PAUSE &&
                key_value != RFKEY_ON_OFF &&
                key_value != RF24_ON_OFF &&
                key_value != RFKEY_LIGHT_PLUS &&
                key_value != RFKEY_LIGHT_SUB &&
                key_value != RFKEY_SPEED_PLUS &&

                key_value != RFKEY_SPEED_SUB &&
                key_value != RFKEY_2H &&
                key_value != RFKEY_1H
                )
            {
                set_ir_auto(IR_PAUSE);
            }

            //速度/亮度 -
            if ((key_value == RF24_SPEED_BRIGHT_SUB || key_value == RFKEY_LIGHT_SUB) && event_type == KEY_EVENT_CLICK)
            {
                if (fc_effect.Now_state == IS_STATIC)
                {

                    extern void bright_sub(void);
                    bright_sub();
                    // memcpy(Send_buffer,Ble_Addr, 6);
                    // Send_buffer[6] = 0x04;
                    // Send_buffer[7] = 0x03;
                    // Send_buffer[8] = fc_effect.b*100/255;;
                    // ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();
                }
                if (fc_effect.Now_state == IS_light_scene)
                {
                    if (MODE_MIXED_WHITE_BREATH == fc_effect.dream_scene.change_type)
                    {
                        // 如果正处于混白色呼吸
                        // fc_effect.dream_scene.mixed_white_breath_speed = (u16)6000;
                        fc_effect.dream_scene.mixed_white_breath_speed = (u16)4000;
                    }
                    else
                    { 
                        fc_effect.dream_scene.speed = 350; 
                    }

                    set_fc_effect();
                    save_user_data_area3();
                }
                if (fc_effect.Now_state == IS_light_music)
                {

                    extern void ls_sensitive_plus(void);
                    ls_sensitive_plus();
                    // Send_buffer[6] = 0x2F;
                    // Send_buffer[7] = 0x05;
                    // Send_buffer[8] = 100-fc_effect.sound.sensitive;
                    // // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
                    // ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();

                }





            }


            //速度/亮度 +
            if ((key_value == RF24_SPEED_BRIGHT_PLUS || key_value == RFKEY_LIGHT_PLUS) && event_type == KEY_EVENT_CLICK)
            {
                if (fc_effect.Now_state == IS_STATIC)
                {

                    extern void bright_plus(void);
                    bright_plus();
                    // memcpy(Send_buffer,Ble_Addr, 6);
                    // Send_buffer[6] = 0x04;
                    // Send_buffer[7] = 0x03;
                    // Send_buffer[8] = fc_effect.b*100/255;;
                    // ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();
                }
                if (fc_effect.Now_state == IS_light_scene)
                {
                    if (MODE_MIXED_WHITE_BREATH == fc_effect.dream_scene.change_type)
                    {
                        // 如果正处于混白色呼吸
                        // fc_effect.dream_scene.mixed_white_breath_speed = (u16)10000;
                        fc_effect.dream_scene.mixed_white_breath_speed = (u16)8000;
                    }
                    else
                    {
                        fc_effect.dream_scene.speed = 200;
                    }


                    set_fc_effect();
                    save_user_data_area3();
                }

                if (fc_effect.Now_state == IS_light_music)
                {

                    extern void ls_sensitive_sub(void);
                    ls_sensitive_sub();
                    // Send_buffer[6] = 0x2F;
                    // Send_buffer[7] = 0x05;
                    // Send_buffer[8] = 100-fc_effect.sound.sensitive;
                    // // app_send_user_data(ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer,9, ATT_OP_AUTO_READ_CCC);
                    // ble_comm_att_send_data(fd_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                    save_user_data_area3();

                }




            }
            //红色
            if (key_value == RF24_RED && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(0);
                save_user_data_area3();

            }
            //绿色
            if (key_value == RF24_GREEN && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(1);
                save_user_data_area3();

            }
            //蓝色
            if (key_value == RF24_BLUE && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(2);
                save_user_data_area3();

            }
            //黄色
            if (key_value == RF24_YELLOW && event_type == KEY_EVENT_CLICK)
            {

                fc_static_effect(4);
                save_user_data_area3();

            }
            //天蓝色  85 250 255
            if (key_value == RF24_AZURE && event_type == KEY_EVENT_CLICK)
            {
                // set_static_mode(85, 250, 255);
                // 改成了 CYAN ， 蓝色和绿色分量最亮：
                set_static_mode(0, 255, 255);

                save_user_data_area3();
            }
            //玫红色  25 50 218  
            if (key_value == RF24_ROSE_RED && event_type == KEY_EVENT_CLICK)
            {
                // set_static_mode(255, 50, 218);
                // 改成 MAGENTA ， 红色和蓝色分量最亮
                set_static_mode(255, 0, 255);
                save_user_data_area3();
            }
            //纯白色   w b
            if (key_value == RF24_WHITE && event_type == KEY_EVENT_CLICK)
            {
                fc_static_effect(3);
                save_user_data_area3();

            }
            //模暖白光
            if (key_value == RF24_WARM_WHITE && event_type == KEY_EVENT_CLICK)
            {

                set_static_mode(255, 180, 20);
                save_user_data_area3();

            }
            //模式集合区
            if (key_value == RF24_ALL_MODE && event_type == KEY_EVENT_CLICK)
            {
                parse_zd_data(all_mode);
                all_mode[2] += 1;
                if (all_mode[2] > 0x1c)
                {
                    all_mode[2] = 0;
                }


            }
            //7色渐变
            if (key_value == RF24_SEVEN_COLOR_GRADUAL && event_type == KEY_EVENT_CLICK)
            {
                u8 temp[3] = { 0x04, 0x02, 0x0A };
                parse_zd_data(temp);

            }
            //七色呼吸
            if (key_value == RF24_SEVEN_COLOR_BREATHE && event_type == KEY_EVENT_CLICK)
            {
                // printf("breath\n");
                u8 temp_buff[3] = { 0x04, 0x02, 254 }; // 对应混白色呼吸

                // if (fc_effect.dream_scene.mixed_white_breath_speed != (u16)6000 && fc_effect.dream_scene.mixed_white_breath_speed != (u16)10000)
                // {
                //     fc_effect.dream_scene.mixed_white_breath_speed = 6000;
                //     // fc_effect.dream_scene.mixed_white_breath_speed = 10000;
                // }

                parse_zd_data(temp_buff);

#if 0
                parse_zd_data(sevrn_color_breath);
                sevrn_color_breath[2] += 1;
                if (sevrn_color_breath[2] > 0x11)
                {
                    sevrn_color_breath[2] = 0x0B;
                }
#endif

            }
            //7色跳变
            if (key_value == RF24_SEVEN_COLOR_JUMP && event_type == KEY_EVENT_CLICK)
            {
                u8 temp[3] = { 0x04, 0x02, 0x08 };
                parse_zd_data(temp);

            }
            //电机转速调节 5挡   8s 13s 18s 21s 26s 35s
            if (key_value == RF24_STEMPMOTOR_SPEED && event_type == KEY_EVENT_CLICK)
            {



                extern void Motor_Switch(void);

                Motor_Switch();
                // one_wire_set_period(period[stepmotpor_speed_cnt]);
                // enable_one_wire();
                // stepmotpor_speed_cnt++;
                // if(stepmotpor_speed_cnt == 6) stepmotpor_speed_cnt = 0;
                // save_user_data_area3();

            }
            //声控1    4音乐律动减
            if (key_value == RF24_SOUND_ONE && event_type == KEY_EVENT_CLICK)
            {

                if (fc_music_cnt > 0)
                    fc_music_cnt--;
                set_music_mode(fc_music_cnt);

            }

            //声控2   4音乐律动加
            if (key_value == RF24_SOUND_TWO && event_type == KEY_EVENT_CLICK)
            {

                if (fc_music_cnt < 3)
                    fc_music_cnt++;

                set_music_mode(fc_music_cnt);

            }

            if (key_value == RF24_ONE_TOW_METEOR && event_type == KEY_EVENT_CLICK)
            {
                one_wire_set_period(period[0]);
                enable_one_wire();
                save_user_data_area3();

            }
            if (key_value == RF24_METEOR_SOUND_ONE_TWO && event_type == KEY_EVENT_CLICK)
            {

                one_wire_set_period(period[1]);
                enable_one_wire();
                save_user_data_area3();
            }
            if (key_value == RF24_DIRECTION && event_type == KEY_EVENT_CLICK)
            {

                one_wire_set_period(period[2]);
                enable_one_wire();
                save_user_data_area3();
            }
            if (key_value == RF24_METEOR_SPEED && event_type == KEY_EVENT_CLICK)
            {

                one_wire_set_period(period[3]);
                enable_one_wire();
                save_user_data_area3();
            }
            if (key_value == RF24_METEOR_FREQUENCY && event_type == KEY_EVENT_CLICK)
            {
                one_wire_set_period(period[4]);
                enable_one_wire();
                save_user_data_area3();
            }
            if (key_value == RF24_METEOR_TAIL && event_type == KEY_EVENT_CLICK)
            {
                one_wire_set_period(period[5]);
                enable_one_wire();
                save_user_data_area3();
            }
#if 0
            //单流星  双流星  
            if (key_value == RF24_ONE_TOW_METEOR && event_type == KEY_EVENT_CLICK && fc_effect.metemor_on_off == 0x01)
            {

                if (get_custom_index() != 19)
                    set_mereor_mode(19);  //单流星 有掉电保存
                else
                    set_mereor_mode(22);  //双流星


            }
            //声控流星1 2 两个律动
            if (key_value == RF24_METEOR_SOUND_ONE_TWO && event_type == KEY_EVENT_CLICK && fc_effect.metemor_on_off == 0x01)
            {

                if (get_custom_index() != 17)
                    set_mereor_mode(17);  //单流星
                else
                    set_mereor_mode(18);  //双流星


            }
            //方向控制   流星方向
            if (key_value == RF24_DIRECTION && event_type == KEY_EVENT_CLICK && fc_effect.metemor_on_off == 0x01)
            {
                change_dir();
                custom_meteor_effect(); //有掉电保存

            }

            //流星速度调节 5挡  1 25 50 75 100
            if (key_value == RF24_METEOR_SPEED && event_type == KEY_EVENT_CLICK && fc_effect.metemor_on_off == 0x01)
            {

                extern void adjust_meteor_speed(void);
                extern void fd_meteor_speed(void);
                adjust_meteor_speed();
                fd_meteor_speed();
            }
            //流星频率调节 5挡  4s 8s 12s 16s 20s
            if (key_value == RF24_METEOR_FREQUENCY && event_type == KEY_EVENT_CLICK && fc_effect.metemor_on_off == 0x01)
            {
                u8 temp[3] = { 0x2F, 0x03, 0x00 };
                temp[2] = meteor_cycle[cycle_cntt];
                parse_zd_data(temp);
                custom_meteor_effect();
                cycle_cntt++;
                if (cycle_cntt > 4) cycle_cntt = 0;

                extern void fd_meteor_cycle(void);
                fd_meteor_cycle();


            }

            //流星拖尾长度调节 3 5 7  
            if (key_value == RF24_METEOR_TAIL && event_type == KEY_EVENT_CLICK && fc_effect.metemor_on_off == 0x01)
            {

                u8 p;
                p = get_custom_index();
                // printf("p  =%d", p);
                if (p == 19 || p == 20 || p == 21)
                {
                    fc_effect.metemor_effect_index++;
                    if (fc_effect.metemor_effect_index > 21)
                        fc_effect.metemor_effect_index = 19;
                    p = fc_effect.metemor_effect_index;
                    // printf("fc_effect.metemor_effect_index = %d",fc_effect.metemor_effect_index);
                    set_mereor_mode(p);  //单流星

                }
                else
                {
                    set_mereor_mode(19);
                }


            }

#endif
            //第二个遥控
#if 0
            if (event_type == KEY_EVENT_CLICK)
            {
                switch (key_value)
                {
                case RFKEY_AUTO:
                    ls_set_speed(90);
                    all_mode[2] = 7;
                    extern void change_ir_auto();
                    change_ir_auto();
                    break;
                case RFKEY_PAUSE:
                    void WS2812FX_play(void);
                    void WS2812FX_pause();
                    extern uint8_t _running;
                    if (_running == 0) WS2812FX_play();
                    else WS2812FX_pause();
                    break;

                    // case RFKEY_LIGHT_PLUS:  break;
                    // case RFKEY_LIGHT_SUB:  break;
                case RFKEY_SPEED_PLUS:
                    // extern void ls_speed_plus(void);
                    // printf("\n speed+ ");
                    // ls_speed_plus();
                    fc_effect.dream_scene.speed = 200;
                    set_fc_effect();


                    save_user_data_area3();
                    break;
                case RFKEY_SPEED_SUB:
                    // printf("\n speed- ");

                    // extern void ls_speed_sub(void);
                    // ls_speed_sub();

                    fc_effect.dream_scene.speed = 350;
                    set_fc_effect();



                    save_user_data_area3();
                    break;
                case RFKEY_R:  fc_static_effect(0); break;
                case RFKEY_G:  fc_static_effect(1); break;
                case RFKEY_B:  fc_static_effect(2); break;
                case RFKEY_YELLOW:  fc_static_effect(4); break;
                case RFKEY_CYAN:  fc_static_effect(5); break;
                case RFKEY_PURPLE:  fc_static_effect(7);  break;
                case RFKEY_W:   set_w(255);break;
                case RFKEY_MODE_ADD:
                    ls_set_speed(90);

                    if (all_mode[2] < 0x15)
                    {
                        all_mode[2] += 1;
                    }
                    parse_zd_data(all_mode);

                    break;
                case RFKEY_MODE_DEC:
                    if (all_mode[2] > 7)
                    {
                        all_mode[2] -= 1;
                    }

                    parse_zd_data(all_mode);


                    break;
                case RFKEY_7COL_JUMP:
                    temp[2] = 0x08;
                    parse_zd_data(temp);
                    break;
                case RFKEY_W_BREATH:
                    ls_set_speed(90);
                    temp[2] = 0x12;
                    parse_zd_data(temp);
                    break;
                case RFKEY_7COL_GRADUAL:
                    temp[2] = 0x0A;
                    parse_zd_data(temp);
                    break;
                case RFKEY_MUSIC1:  set_music_mode(1); break; //声控渐变
                case RFKEY_MUSIC2:  set_music_mode(3); break; //爆闪
                case RFKEY_MUSIC3:  set_music_mode(2); break; //静态
                case RFKEY_2H:
                    extern void set_count_down(u8 t);
                    set_count_down(2);
                    break;
                case RFKEY_1H:
                    set_count_down(1);
                    break;
                }
            }

#endif
        }

    }


}


#endif


