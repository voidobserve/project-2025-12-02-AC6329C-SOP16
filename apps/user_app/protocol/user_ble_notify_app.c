#include "user_ble_notify_app.h"
#include "user_ble_notify.h"

#include "led_strip_drive.h"
#include "led_strand_effect.h"

// notify 设备信息
void user_ble_notify_dev_type(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x07;
    buf[buf_len++] = 0x01;
    buf[buf_len++] = 0x01;
    if (is_rgbw) {
        buf[buf_len++] = 0x02; // 灯具类型：RGBW
    } else {
        buf[buf_len++] = 0x01; // 灯具类型：RGB
    }

    user_ble_notify_param_put(buf, buf_len);
}

// notify 灯光电源状态
void user_ble_notify_light_pwr_sta(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x01;
    buf[buf_len++] = 0x01;
    buf[buf_len++] = fc_effect.on_off_flag;

    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_light_brightness(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x04;
    buf[buf_len++] = 0x03;
    buf[buf_len++] = fc_effect.b * 100 / 255;

    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_light_speed(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x04;
    buf[buf_len++] = 0x04;
    buf[buf_len++] = (500 - fc_effect.dream_scene.speed) / 5;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_light_len(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x04;
    buf[buf_len++] = 0x08;
    buf[buf_len++] = fc_effect.led_num >> 8;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_sensitive(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x2F;
    buf[buf_len++] = 0x05;
    buf[buf_len++] = 100 - fc_effect.sound.sensitive;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_light_color(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x04;
    buf[buf_len++] = 0x01;
    buf[buf_len++] = 0x1e;
    buf[buf_len++] = fc_effect.rgb.r;
    buf[buf_len++] = fc_effect.rgb.g;
    buf[buf_len++] = fc_effect.rgb.b;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_motor_speed(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x2F;
    buf[buf_len++] = 0x07;
    buf[buf_len++] = fc_effect.base_ins.period;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_motor_mode(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x2F;
    buf[buf_len++] = 0x06;
    buf[buf_len++] = fc_effect.base_ins.mode;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_rgb_sequence(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x04;
    buf[buf_len++] = 0x05;
    buf[buf_len++] = fc_effect.sequence;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_sound_ctl_type(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x06;
    buf[buf_len++] = 0x07;
    buf[buf_len++] = fc_effect.music.m_type;
    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_sound_mode(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x06;
    buf[buf_len++] = 0x06;
    buf[buf_len++] = fc_effect.music.m;
    user_ble_notify_param_put(buf, buf_len);
}

// notify 流星灯电源状态
void user_ble_notify_meteor_pwr_sta(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x2F;
    buf[buf_len++] = 0x02;
    buf[buf_len++] = fc_effect.metemor_on_off;

    user_ble_notify_param_put(buf, buf_len);
}

// 流星灯速度
void user_ble_notify_meteor_speed(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x2F;
    buf[buf_len++] = 0x01;
    buf[buf_len++] = 110 - (fc_effect.speed / 3);

    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_meteor_period(void)
{
    u8 buf[10] = {0};
    u8 buf_len = 0;
    buf[buf_len++] = 0x2F;
    buf[buf_len++] = 0x03;
    buf[buf_len++] = fc_effect.meteor_period;

    user_ble_notify_param_put(buf, buf_len);
}

void user_ble_notify_dev_all_info(void)
{
    user_ble_notify_dev_type();
    user_ble_notify_light_pwr_sta();
    user_ble_notify_light_brightness();
    user_ble_notify_light_speed();
    user_ble_notify_light_len();
    user_ble_notify_sensitive();
    user_ble_notify_light_color();
    user_ble_notify_motor_speed();
    user_ble_notify_motor_mode();
    user_ble_notify_rgb_sequence();
    user_ble_notify_sound_ctl_type();
	user_ble_notify_sound_mode();
	user_ble_notify_meteor_pwr_sta();
	user_ble_notify_meteor_speed();
	user_ble_notify_meteor_period();
}

