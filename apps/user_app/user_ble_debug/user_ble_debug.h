#ifndef __USER_BLE_DEBUG_H__
#define __USER_BLE_DEBUG_H__

#define USER_BLE_DEBUG_ENABLE 0
#if USER_BLE_DEBUG_ENABLE
#include "typedef.h"

void user_ble_debug_connection_handle_update(u16 handle);
void user_ble_debug_notify(u8 *buf, u16 len);


#endif
#endif

