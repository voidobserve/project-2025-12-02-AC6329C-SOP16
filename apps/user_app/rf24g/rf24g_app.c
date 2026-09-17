

#include "system/includes.h"

#include "task.h"
#include "event.h"
#include "led_strip_sys.h"
#include "board_ac632n_demo_cfg.h"
#include "led_strand_effect.h"
#include "one_wire.h"
#include "btstack/btstack_typedef.h"
#include "ble_multi_profile.h"
#include "att.h"
#include "asm/mcpwm.h"
#include "ir_key_app.h"
#include "led_strand_effect.h"

#include "rf24g_parse.h"

u8 off_long_cnt = 0;
extern void parse_zd_data(unsigned char *LedCommand);
extern void set_IS_light_scene_state(void);
u8 all_mode[3] = {0x04, 0x02, 0x07};           //j模式集合
u8 sevrn_color_breath[3] = {0x04, 0x02, 0x0b}; //七色呼吸
u8 stepmotpor_speed_cnt = 0;
u8 dynamic_speed = 0;
u8 meteor_flag;
u8 meteor_music_flag;
u8 meteor_speed[5] = {1, 25, 50, 75, 100}; //1, 25, 50, 75, 100
// u8 meteor_cycle[5] = {2, 8, 12, 16, 20};   //2s 8s 12s 16s 20s
u8 cycle_cntt = 0;
u8 meteor_tail = 0;
u8 meteor_direction = 0; //0：顺向  1：正向
u8 single_meteor = 0;
u8 fc_music_cnt = 0;
extern u8 Ble_Addr[6]; //蓝牙地址
extern hci_con_handle_t fd_handle;

// 根据按键键值和key_driver_scan得到的事件值，转换为对应的按键事件
u8 rf24g_convert_key_event(u8 key_value, u8 key_driver_event)
{
    // 将key_driver_scan得到的key_event转换成自定义的key_event对应的索引
    // 索引对应 rf24g_key_event_table[][] 中的索引
    u8 key_event_index = 0; // 默认为0，0对应无效索引
    if (KEY_EVENT_PRESS == key_driver_event) {
        key_event_index = 1;
    } else if (KEY_EVENT_CLICK == key_driver_event) {
        key_event_index = 2;
    } else if (KEY_EVENT_LONG == key_driver_event) {
        key_event_index = 3;
    } else if (KEY_EVENT_HOLD == key_driver_event) {
        key_event_index = 4;
    } else if (KEY_EVENT_UP == key_driver_event) {
        // 长按后松手
        key_event_index = 5;
    }

    if (0 == key_event_index || NO_KEY == key_value) {
        // 按键事件与上面的事件都不匹配
        // 得到的键值是无效键值
        return RF24G_KEY_EVENT_NONE;
    }

    // 遍历表格中的每一个按键：
    for (u8 i = 0; i < sizeof(rf24g_parse_table) / sizeof(rf24g_parse_table[0]);
         i++) {
        if (key_value == rf24g_parse_table[i][0]) {
            return rf24g_parse_table[i][key_event_index];
        }
    }

    // 如果运行到这里，都没有找到对应的按键，返回无效按键事件
    return RF24G_KEY_EVENT_NONE;
}

void rf24g_key_event_handle(void)
{
    u8 rf24g_key_event = 0;
    rf24g_key_event =
        rf24g_convert_key_event(rf24g_key_driver_value, rf24g_key_driver_event);
    rf24g_key_driver_value =
        NO_KEY; // 置为无效键值（由于扫描函数只更新，不会清除，在这里要清除）

    // u8 key_value = rf24g_key_driver_value;
    // u8 key_driver_event = rf24g_key_driver_event;
    // u8 rf24g_key_event = rf24g_convert_key_event(key_value, key_driver_event);

    // rf24g_key_driver_value = NO_KEY;
    // rf24g_key_driver_event = 0;

    if (rf24g_key_event == RF24G_KEY_EVENT_NONE) {
        return;
    }

    printf("key event == %u\n", (u16)rf24g_key_event);
    switch (rf24g_key_event) {
    case RF24G_KEY_EVENT_R1C3_LONG:
        // 长按按键 进行开关灯
        if (fc_effect.on_off_flag == DEVICE_ON) {
            soft_rurn_off_lights();
        } else {
            soft_turn_on_the_light();
        }
        break;

    case RF24G_KEY_EVENT_R1C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        if (fc_effect.Now_state == IS_STATIC) {
            extern void bright_sub(void);
            bright_sub();
            save_user_data_area3();
        } else if (fc_effect.Now_state == IS_light_scene) {
            // USER_TO_DO
            if (MODE_MIXED_WHITE_BREATH == fc_effect.dream_scene.change_type) {
                // 如果正处于混白色呼吸
                // fc_effect.dream_scene.mixed_white_breath_speed = (u16)6000;
                fc_effect.dream_scene.mixed_white_breath_speed = (u16)4000;
            } else {
                fc_effect.dream_scene.speed = 350;
            }

            set_fc_effect();
            save_user_data_area3();
        } else if (fc_effect.Now_state == IS_light_music) {
            extern void ls_sensitive_plus(void);
            ls_sensitive_plus();
            save_user_data_area3();
        }
        break;
    case RF24G_KEY_EVENT_R1C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        if (fc_effect.Now_state == IS_STATIC) {

            extern void bright_plus(void);
            bright_plus();
            save_user_data_area3();
        } else if (fc_effect.Now_state == IS_light_scene) {
            if (MODE_MIXED_WHITE_BREATH == fc_effect.dream_scene.change_type) {
                // 如果正处于混白色呼吸
                // fc_effect.dream_scene.mixed_white_breath_speed = (u16)10000;
                fc_effect.dream_scene.mixed_white_breath_speed = (u16)8000;
            } else {
                fc_effect.dream_scene.speed = 200;
            }

            set_fc_effect();
            save_user_data_area3();
        } else if (fc_effect.Now_state == IS_light_music) {
            extern void ls_sensitive_sub(void);
            ls_sensitive_sub();
            save_user_data_area3();
        }
        break;
    case RF24G_KEY_EVENT_R2C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        //红色
        fc_static_effect(0);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R2C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 绿色
        fc_static_effect(1);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R2C3_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 蓝色
        fc_static_effect(2);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R3C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 黄色
        fc_static_effect(4);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R3C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 改成了 CYAN ， 蓝色和绿色分量最亮：
        set_static_mode(0, 255, 255);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R3C3_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 改成 MAGENTA ， 红色和蓝色分量最亮
        set_static_mode(255, 0, 255);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R4C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 纯白色
        fc_static_effect(3);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R4C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 模暖白光
        set_static_mode(255, 180, 20);
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R4C3_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 模式集合区
        parse_zd_data(all_mode);
        all_mode[2] += 1;
        if (all_mode[2] > 0x1c) {
            all_mode[2] = 0;
        }
        break;
    case RF24G_KEY_EVENT_R5C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 7色渐变
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
        set_fc_effect();
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R5C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 七色呼吸
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_MUTIL_BRAETH;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R5C3_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 七色跳变
        u8 temp[3] = {0x04, 0x02, 0x08};
        parse_zd_data(temp);
        break;
    case RF24G_KEY_EVENT_R6C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        // 电机转速调节 5挡   8s 13s 18s 21s 26s 35s
        extern void Motor_Switch(void);
        Motor_Switch();
        break;
    case RF24G_KEY_EVENT_R6C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        if (fc_music_cnt > 0) {
            fc_music_cnt--;
        }
        set_music_mode(fc_music_cnt);
        break;
    case RF24G_KEY_EVENT_R6C3_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        if (fc_music_cnt < 3) {
            fc_music_cnt++;
        }
        set_music_mode(fc_music_cnt);
        break;
    case RF24G_KEY_EVENT_R7C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        one_wire_set_mode(4);
        one_wire_set_period(period[0]);
        enable_one_wire();
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R7C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        one_wire_set_mode(4);
        one_wire_set_period(period[1]);
        enable_one_wire();
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R7C3_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        one_wire_set_mode(4);
        one_wire_set_period(period[2]);
        enable_one_wire();
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R8C1_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        one_wire_set_mode(4);
        one_wire_set_period(period[3]);
        enable_one_wire();
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R8C2_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        one_wire_set_mode(4);
        one_wire_set_period(period[4]);
        enable_one_wire();
        save_user_data_area3();
        break;
    case RF24G_KEY_EVENT_R8C3_PRESS:
        if (0 == fc_effect.on_off_flag) {
            return;
        }

        one_wire_set_mode(4);
        one_wire_set_period(period[5]);
        enable_one_wire();
        save_user_data_area3();
        break;

    default:
        break;
    }
}
