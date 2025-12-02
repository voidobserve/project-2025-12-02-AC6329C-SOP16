
#ifndef led_strip_drive_h
#define led_strip_drive_h

#include "board_ac632n_demo_cfg.h"
#include "asm/ledc.h"
#include "asm/gpio.h"

#define MIC_PIN     IO_PORTA_08
#define LEDC_PIN    IO_PORTB_07 



void led_state_init(void);
void mic_gpio_init();
void led_gpio_init(void);
void led_pwm_init(void);
void ledc_init(const struct ledc_platform_data *arg);
void fc_driver(u8 r,u8 g ,u8 b);
#endif







