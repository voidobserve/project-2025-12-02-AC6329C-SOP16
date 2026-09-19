#include "user_ble_debug.h"
#include "btstack_typedef.h"   // hci_con_handle_t
#include "le_gatt_common.h"    // ble_comm_att_send_data()
#include "ble_trans_profile.h" // ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLEATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE

#if USER_BLE_DEBUG_ENABLE
static volatile u16 connection_handle; // 存放蓝牙连接后的句柄

void user_ble_debug_connection_handle_update(u16 handle)
{
    connection_handle = handle;
}

void user_ble_debug_notify(u8 *buf, u16 len)
{
    // 使用预留的 fff1 通知：
    ble_comm_att_send_data(connection_handle,
                           ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, buf, len,
                           ATT_OP_AUTO_READ_CCC);
}

#endif