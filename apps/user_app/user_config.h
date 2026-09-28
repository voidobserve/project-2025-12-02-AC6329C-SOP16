#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

/*
	最终生产的版本需要屏蔽该宏定义，否则功能会冲突
	IO_PORT_DP 在测试时 用于打印
	在最终样机对应的版本中，用于检测220V的50Hz信号
*/
#define USER_DEBUG_ENABLE 0

#endif
