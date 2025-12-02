#ifndef led_strand_effect_h
#define led_strand_effect_h
#include "cpu.h"
#include "led_strip_sys.h"
#include "WS2812FX.H"
#include "led_strip_drive.h"
#include "one_wire.h"


#define MAX_SMEAR_LED_NUM 48  //最多48个灯/48段
#define ALARM_NUMBER  3   //闹钟个数
#define MAX_SOUND   10   //声音ADC采集个数


//麦克风
typedef enum
{
    OFF,    //mic关闭
    ON,     //mic打开
}MIC_OFFON;

extern MIC_OFFON MIC_ENABLE;

//声控
typedef struct
{
    int buf[MAX_SOUND];
    int v;          //平均值
    int c_v;        //当前值
    u8  valid;      //数据有效
    u8  sensitive;  //灵敏度 0~100
}sound_t;




//当前模式枚举
typedef enum
{
  ACT_TY_PAIR,          //配对效果
  ACT_CUSTOM,           //自定义效果
  IS_STATIC,            //静态模式
  IS_light_music = 27,    //音乐律动
  IS_light_scene = 56,   //炫彩情景
  IS_smear_adjust = 59  //涂抹功能


} Now_state_e;

//涂抹工具
typedef enum
{
  IS_drum = 1,  //油桶
  IS_pen = 2,   //画笔
  IS_eraser = 3 //橡皮檫
} smear_tool_e;

//方向
typedef enum
{
  IS_forward = 0,
  IS_back = 16
} direction_e;

//变化方式
typedef enum
{
  MODE_MUTIL_RAINBOW = 2,           //彩虹(多段颜色)
  MODE_MUTIL_JUMP = 10,             //跳变模式(多段颜色)
  MODE_MUTIL_BRAETH = 11,           //呼吸模式(多段颜色)
  MODE_MUTIL_TWIHKLE = 12,          //闪烁模式(多段颜色)
  MODE_MUTIL_FLOW_WATER = 13,       //流水模式(多段颜色)
  MODE_CHAS_LIGHT = 14,             //追光模式
  MODE_MUTIL_COLORFUL = 15,         //炫彩模式(多段颜色)
  MODE_MUTIL_SEG_GRADUAL = 16,      //渐变模式(多段颜色)
  MODE_JUMP,                        //标准跳变
  MODE_STROBE,                      //频闪，颜色之间插入黑mode
  MODE_MUTIL_C_GRADUAL,             //多种颜色切换整条渐变
  MODE_2_C_FIX_FLOW,                //两种颜色混合流水，渐变色流水
  MODE_SINGLE_FLASH_RANDOM = 21 ,   //单点，单色随机闪现
  MODE_SEG_FLASH_RANDOM = 22,       //从的颜色池抽取颜色闪现，以段为单位，闪现位置随机
  MODE_SINGLE_METEOR = 23,          //流星效果
  MODE_SINGLE_C_BREATH = 24,        //单色呼吸
  MODE_GRADUAL = 25,                //标准渐变，彩虹颜色
  MODE_BREATH_W = 26 ,               //W通道呼吸
  MODE_MUTIL_C_BREATH,

  MODE_MIXED_WHITE_BREATH,  // 混白色呼吸

} change_type_e;

#pragma pack (1)
/*----------------------------涂抹功能结构体----------------------------------*/
typedef struct
{
  smear_tool_e smear_tool;
  color_t rgb[MAX_SMEAR_LED_NUM];
} smear_adjust_t;

/*----------------------------静态模式结构体----------------------------------*/
  

/*----------------------------幻彩情景结构体----------------------------------*/
typedef struct
{

  change_type_e change_type;  //变化类型、模式
  direction_e direction;      //效果的方向
  unsigned char seg_size;     //段大小
  unsigned char c_n;          //颜色数量
  color_t rgb[MAX_NUM_COLORS];
  unsigned short speed;       //由档位决定 

  u16 mixed_white_breath_speed; // 混白色呼吸的速度 目前固定只有 6秒 和 10秒，数值对应 6000 和 10000 
} dream_scene_t;

/*----------------------------倒计时结构体----------------------------------*/
typedef struct
{
  unsigned char set_on_off;
  unsigned long time;
} countdown_t;

typedef struct 
{

  u8 hour;
  u8 minute;
  u8 second;
  u8 week;

}TIME_CLOCK;



typedef struct 
{
    u8 week;
    u8 hour;
    u8 minute;
    u8 on_off;
    u8 mode;

}ALARM_CLOCK;


typedef struct
{
  unsigned char m;  //效果模式
  unsigned char s;  //灵敏度
  unsigned char m_type;  //区分音乐的模式，手机麦或者外麦
}music_t;

/*----------------------------幻彩灯串效果大结构体----------------------------------*/
typedef struct
{
  unsigned char on_off_flag;    //开关状态
  unsigned char led_num;        //灯点数
  color_t rgb;                  //静态模式颜色
  unsigned char b;              //亮度
  unsigned short speed;         //8档  速度
  unsigned char meteor_period;  //周期值，单位秒
  unsigned char mode_cycle;     //1:模式完成一个循环。0：正在跑，和meteor_period搭配用
  u16 period_cnt;               //ms,运行时的计数器
  Now_state_e Now_state;        //当前运行模式
  // smear_adjust_t smear_adjust;  //涂抹功能
  dream_scene_t dream_scene;    //幻彩情景
  // countdown_t countdown;        //涂鸦倒计时
  u8 w;                         //w通道灰度，RGB模式W必须为0，w模式RGB为0
  u8 breath_mode;               //呼吸模式，0：红色，1：绿，2：蓝，3：W，4：七彩呼吸
  u8 music_mode;
  music_t music;                //音乐
  sound_t sound;                //声控
  u8 metemor_on_off;           //流星开关
  u8 metemor_effect_index;
  base_ins_t base_ins;           //电机
  unsigned char sequence;       //RGB通道顺序
  bool  auto_f;                 //自动跑效果
} fc_effect_t;

 countdown_t zd_countdown[ALARM_NUMBER];
 typedef struct _LED_STATE {
    u32 ledlight;                   //灯条的亮度，范围由Light_Max，Light_Min决定
    u32 ledlight_temp;              //亮度的百分比，取值范围0-100
    u8 running_task;                //设备处在动态模式还是静态模式
    u8 dynamic_state_flag;          //如果是动态,设备处在动态的哪种模式
    u8 static_state_flag;           //如果是静态,设备处在静态的哪种模式
    u8 OpenorCloseflag;             //亮灯还是灭灯标志,默认关灯
    u8 R_flag;                      //红色，取值范围0~0xff
    u8 G_flag;                      //绿色，取值范围0~0xff
    u8 B_flag;                      //蓝色，取值范围0~0xff
    u8 W_flag;
    u8 interface_mode;              //灯条RBG接口顺序模式
    u8 speed;                       //动态时用于调节速度
}LED_STATE;






#pragma pack ()

extern fc_effect_t fc_effect;
extern TIME_CLOCK time_clock;
extern ALARM_CLOCK alarm_clock[3];
void effect_smear_adjust_updata(smear_tool_e tool, hsv_t *colour,unsigned short *led_place);

void set_fc_effect(void);

void full_color_init(void);
void soft_rurn_off_lights(void); //软关灯处理
void soft_turn_on_the_light(void) ;  //软开灯处理
void ls_set_speed(uint8_t s);
void set_bright(u8 b);
void fc_static_effect(u8 n);
void ls_set_color(uint8_t n, uint32_t c);
void music_static_sound(void);

void set_static_mode(u8 r, u8 g, u8 b);
void fc_dynamic_effect(u8 n);

#endif
