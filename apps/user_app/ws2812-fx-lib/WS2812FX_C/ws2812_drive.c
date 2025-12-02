
#include "ws2812_bsp.h"
#include "debug.h"
#include "my_effect.h"
static unsigned long tick_ms;
void ws281x_init()
{

}
extern uint8_t           rOffset;    ///< Red index within each 3- or 4-byte pixel
extern uint8_t           gOffset;    ///< Index of green byte
extern uint8_t           bOffset;    ///< Index of blue byte
extern u8 send_rgbbuf_flag;
u8 buf[12*3];   //使用中断实现效果时，必须需要全局变量


/**
 * @brief 灯具的驱动集成，包括七彩的驱动，幻彩的驱动
 * 
 * @param pixels_pattern   颜色
 * @param pattern_size     灯具的长度
 */
void ws281x_show(unsigned char *pixels_pattern, unsigned short pattern_size)
{
   
    //printf_buf(pixels_pattern,pattern_size);
//七彩灯驱动函数
    extern void fc_driver(u8 r,u8 g ,u8 b);
    fc_driver(  *pixels_pattern,\      
                *(pixels_pattern + 1),\  
                *(pixels_pattern + 2)); 


  
//该数据处理是仅有白光的流星使用
    unsigned short i,j=0,k=0;    
    u8 r,g,b;
    for(i=0; i< pattern_size/3; i++)
    {
         *(buf+i) = *(pixels_pattern + 3 + i*3);  //sjf  +3是因为前面有一个灯 RGB，地址不是从零开始

    }
    for(i=0; i< pattern_size/3; i++)
    {
        if(j==0)
        {
            r = *(buf+i);
        }
        else if(j==1)
        {
            g = *(buf+i);
        }
        else if(j==2)
        {
            b = *(buf+i);
        }
        j++;
        if(j==3)
        {
            j=0;
            *(buf + rOffset + (i-2)) = r;
            *(buf + gOffset + (i-2)) = g;
            *(buf + bOffset + (i-2)) = b;
        }
    }
    //幻彩灯驱动函数
    // ledc_send_rgbbuf(0, buf, pattern_size/3, 0); 
    extern void ledc_send_rgbbuf_isr(u8 index, u8 *rgbbuf, u32 buf_len, u16 again_cnt);
    ledc_send_rgbbuf_isr(0, buf, pattern_size/3, 0);
     
}

// 周期10ms
unsigned long HAL_GetTick(void)
{
    return tick_ms;
}

// 每10ms调用一次
void run_tick_per_10ms(void)
{
    tick_ms+=10;
}




