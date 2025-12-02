#include "rf24g_server_driver.h"
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
// #include "ble_multi.h"
#include "le_client_demo.h"
#include "gatt_common/le_gatt_common.h"


#if 0

static scan_conn_cfg_t multi_client_scan_cfg;


//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   ADV_SCAN_MS(24) // unit: 0.625ms  24
//搜索 窗口大小
#define SET_SCAN_WINDOW     ADV_SCAN_MS(8)  // unit: 0.625ms, <= SET_SCAN_INTERVAL  8

//连接周期
#define BASE_INTERVAL_MIN   (6)//最小的interval
#define SET_CONN_INTERVAL   (BASE_INTERVAL_MIN*8) //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    400 //(unit:10ms)

//建立连接超时
#define SET_CREAT_CONN_TIMEOUT    8000 //(unit:ms)


struct ctl_pair_info_t {
    u8 head_tag;
    u8 match_dev_id;
    u8 pair_flag;
    u8 peer_address_info[7];
    u16 conn_handle;
    u16 conn_interval;
    u16 conn_latency;
    u16 conn_timeout;
};

//scan参数设置
static void multi_scan_conn_config_set(struct ctl_pair_info_t *pair_info)
{
    multi_client_scan_cfg.scan_auto_do = 1;
    multi_client_scan_cfg.creat_auto_do = 1;
    multi_client_scan_cfg.scan_type = SET_SCAN_TYPE;
    multi_client_scan_cfg.scan_filter = 1;
    multi_client_scan_cfg.scan_interval = SET_SCAN_INTERVAL;
    multi_client_scan_cfg.scan_window = SET_SCAN_WINDOW;

    if (pair_info) {
        log_info("pair_to_creat:%d,%d,%d\n", pair_info->conn_interval, pair_info->conn_latency, pair_info->conn_timeout);
        multi_client_scan_cfg.creat_conn_interval = pair_info->conn_interval;
        multi_client_scan_cfg.creat_conn_latency = pair_info->conn_latency;
        multi_client_scan_cfg.creat_conn_super_timeout = pair_info->conn_timeout;
    } else {
        multi_client_scan_cfg.creat_conn_interval = SET_CONN_INTERVAL;
        multi_client_scan_cfg.creat_conn_latency = SET_CONN_LATENCY;
        multi_client_scan_cfg.creat_conn_super_timeout = SET_CONN_TIMEOUT;
    }

    multi_client_scan_cfg.conn_update_accept = 1;
    multi_client_scan_cfg.creat_state_timeout_ms = SET_CREAT_CONN_TIMEOUT;

    ble_gatt_client_set_scan_config(&multi_client_scan_cfg);
}




//multi client 初始化
void multi_client_init(void)
{
    // log_info("%s", __FUNCTION__);
    int i;

#if CLIENT_PAIR_BOND_ENABLE
    if (!multi_bond_device_table) {
        int table_size = sizeof(multi_match_device_table) + sizeof(client_match_cfg_t) * SUPPORT_MAX_GATT_CLIENT;
        bond_device_table_cnt = multil_client_search_config.match_devices_count + SUPPORT_MAX_GATT_CLIENT;
        multi_bond_device_table = malloc(table_size);
        ASSERT(multi_bond_device_table != NULL, "%s,malloc fail!", __func__);
        memset(multi_bond_device_table, 0, table_size);
    }

    if (0 == multi_client_pair_vm_do(NULL, 0)) {
        log_info("client already bond dev");
    }

    if (multi_sm_master_pair_redo) {
        sm_set_master_pair_redo(multi_sm_master_pair_redo);
    }

#else
    // ble_gatt_client_set_search_config(&multil_client_search_config);
#endif

    multi_scan_conn_config_set(NULL);  //蓝牙主机配置重点是这个函数

#if MULTI_TEST_WRITE_SEND_DATA
    sys_timer_add(0, multi_client_test_write, 500);
#endif
}

#endif 


