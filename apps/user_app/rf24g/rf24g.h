#ifndef rf24g_h
#define rf24g_h





// 硬件按键值定义
//天奕光纤灯
#define RF24_K1 0x08	
#define RF24_K2 0x0c	
#define RF24_K3 0x07

#define RF24_K4 0x10
#define RF24_K5 0x14	
#define RF24_K6 0x05

#define RF24_K7 0x18	
#define RF24_K8 0x1c	
#define RF24_K9 0x06

#define RF24_K10 0x09	
#define RF24_K11 0x0a	
#define RF24_K12 0x0b

#define RF24_K13 0x0d	
#define RF24_K14 0x0e	
#define RF24_K15 0x0f

#define RF24_K16 0x11	
#define RF24_K17 0x12
#define RF24_K18 0x13

#define RF24_K19 0x15	
#define RF24_K20 0x16
#define RF24_K21 0x17

#define RF24_K22 0x19
#define RF24_K23 0x1a	
#define RF24_K24 0x1b	

// 用户键值定制
//天奕光纤灯
#define RF24_SPEED_BRIGHT_SUB       RF24_K1
#define RF24_SPEED_BRIGHT_PLUS      RF24_K2
#define RF24_ON_OFF                 RF24_K3
#define RF24_RED                    RF24_K4
#define RF24_GREEN                  RF24_K5
#define RF24_BLUE                   RF24_K6
#define RF24_YELLOW                 RF24_K7
#define RF24_AZURE                  RF24_K8
#define RF24_ROSE_RED               RF24_K9
#define RF24_WHITE                  RF24_K10
#define RF24_WARM_WHITE             RF24_K11
#define RF24_ALL_MODE               RF24_K12
#define RF24_SEVEN_COLOR_GRADUAL    RF24_K13
#define RF24_SEVEN_COLOR_BREATHE    RF24_K14
#define RF24_SEVEN_COLOR_JUMP       RF24_K15
#define RF24_STEMPMOTOR_SPEED       RF24_K16
#define RF24_SOUND_ONE              RF24_K17  //七彩？流星？声控
#define RF24_SOUND_TWO              RF24_K18 //七彩？流星？声控
#define RF24_ONE_TOW_METEOR         RF24_K19
#define RF24_METEOR_SOUND_ONE_TWO   RF24_K20
#define RF24_DIRECTION              RF24_K21
#define RF24_METEOR_SPEED           RF24_K22
#define RF24_METEOR_FREQUENCY       RF24_K23
#define RF24_METEOR_TAIL            RF24_K24


// ----------------------------------------------超薄24键遥控
#define RFKEY_AUTO 0x3B
#define RFKEY_PAUSE 0x3A
#define RFKEY_ON_OFF 0x40
#define RFKEY_LIGHT_PLUS 0x3C
#define RFKEY_LIGHT_SUB 0x3E
#define RFKEY_SPEED_PLUS 0x3D
#define RFKEY_SPEED_SUB 0x39
#define RFKEY_R 0x4D
#define RFKEY_G 0x47
#define RFKEY_B 0x41
#define RFKEY_YELLOW 0x4E
#define RFKEY_CYAN 0x48
#define RFKEY_PURPLE 0x42
#define RFKEY_W 0x49
#define RFKEY_MODE_ADD 0x4C
#define RFKEY_MODE_DEC 0x46
#define RFKEY_7COL_JUMP 0x3F
#define RFKEY_W_BREATH 0x4B
#define RFKEY_7COL_GRADUAL 0x45
#define RFKEY_MUSIC1 0x50
#define RFKEY_MUSIC2 0x4A
#define RFKEY_MUSIC3 0x44
#define RFKEY_2H 0x43
#define RFKEY_1H 0x4F

//天奕光纤灯2.4G遥控 不同的遥控，该结构体不一样
#pragma pack (1)
// typedef struct
// {
//     u8 header1;
//     u8 header2;
//     u8 sum_h;
//     u8 sum_l;
//     u16 vid;  //
//     u8 dynamic_code;    //  动态码 包号
//     u8 type;
//     u16 signatures;
//     u8 key_v;
//     u8 remoter_id;
//     // u8 pair[3];         //客户码
 

// }rf24g_ins_t;   //指令数据

typedef struct
{
    u8 header1;
    u8 header2;
    u8 key_v;
    u8 remoter_id;
    u8 dynamic_code;    //  动态码 包号
    u8 remoter_id3;
    u8 remoter_id2;
    u8 remoter_id1;
 
}rf24g_ins_t;   //指令数据



#pragma pack ()

void rf24g_scan(unsigned char *pBuf);
void rf24g_long_timer(void);
u8 get_rf24g_long_state(void);


#endif

