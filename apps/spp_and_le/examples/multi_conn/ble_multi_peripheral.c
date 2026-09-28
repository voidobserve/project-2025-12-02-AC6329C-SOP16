/*********************************************************************************************
    *   Filename        : le_server_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/
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
#include "gatt_common/le_gatt_common.h"
#include "ble_multi.h"
#include "ble_multi_profile.h"

#include "user_ble_debug.h"
#include "dp_data_tran.h"
#include "user_ble_notify.h"
#include "rf24g_parse.h"
#include "le/ble_api.h"

#if CONFIG_APP_MULTI && CONFIG_BT_GATT_SERVER_NUM

#if LE_DEBUG_PRINT_EN
//#define log_info            y_printf
#define log_info(x, ...) printf("[MUL-PER]" x " ", ##__VA_ARGS__)
#define log_info_hexdump put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN (160 * 5) //

//---------------
//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static uint8_t multi_connection_update_enable = 1; ///0--disable, 1--enable
//当前请求的参数表index
//参数表
/* 注意: 连接间隔至少要到 24(即30ms)!
 * 实测(用 nRF Connect 逐个请求各优先级验证): 间隔 < 30ms 时控制器会停止
 * 调度BLE扫描, 表现为手机连接后2.4G遥控器彻底失效(断开连接立即恢复);
 * 30~50ms(BALANCED)及以上则一切正常。
 * 原值 {16,24,...} 下限只有20ms, 手机会在区间内任选(实测 21 死 / 24 活)。 */
static const struct conn_update_param_t multi_connection_param_table[] = {
    {24, 40, 10, 600}, //30~50ms 推荐值
    {24, 32, 10, 600}, //30~40ms
    {32, 48, 10, 600}, //40~60ms
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT                                                   \
    (sizeof(multi_connection_param_table) / sizeof(struct conn_update_param_t))

#define EIR_TAG_STRING 0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D', 'K'
static const char user_tag_string[] = {EIR_TAG_STRING};

static u8 multi_adv_data[ADV_RSP_PACKET_MAX];      //max is 31
static u8 multi_scan_rsp_data[ADV_RSP_PACKET_MAX]; //max is 31
static u8 multi_test_read_write_buf[4];

static adv_cfg_t multi_server_adv_config;
static u8 pair_bond_enalbe;

//配对信息表
#define PER_PAIR_BOND_ENABLE CONFIG_BT_SM_SUPPORT_ENABLE
#define PER_PAIR_BOND_TAG    0x53
#define DIRECT_ADV_MAX_CNT   2
static u8 pair_bond_info[8];     //tag + addr_type + address
static u8 direct_adv_count;      //定向广播次数
static u8 cur_peer_addr_info[7]; //当前连接对方地址信息

//------------------------------------------------------
//广播参数设置
extern const char *bt_get_local_name();
extern void clr_wdt(void);
static void multi_adv_config_set(void);

/* 重开2.4G遥控器所依赖的BLE扫描通道(先关后开)。
 * 实测结论: 对"特定手机连接后底层扫描被控制器停掉"的情况无效,
 * 重开命令返回成功但不起作用, 故整体屏蔽保留代码备用。
 * 注意: 若恢复使用, filter policy 必须在 ble_op_set_scan_param 之前下发。 */
#if 0
static void multi_rf24g_scan_reopen(const char *tag)
{
    if (BLE_ST_SCAN == ble_gatt_client_get_work_state()) {
        ble_gatt_client_scan_enable(0);
    }
    ble_op_set_scan_filter_policy(0);
    log_info("scan reopen(%s) ret = %d", tag,
             ble_gatt_client_scan_enable(1));
}
#endif

//------------------------------------------------------
//for ANCS
static uint16_t multi_att_read_callback(hci_con_handle_t connection_handle,
                                        uint16_t att_handle, uint16_t offset,
                                        uint8_t *buffer, uint16_t buffer_size);
static int multi_att_write_callback(hci_con_handle_t connection_handle,
                                    uint16_t att_handle,
                                    uint16_t transaction_mode, uint16_t offset,
                                    uint8_t *buffer, uint16_t buffer_size);
static int multi_event_packet_handler(int event, u8 *packet, u16 size,
                                      u8 *ext_param);

const gatt_server_cfg_t mul_server_init_cfg = {
    .att_read_cb = &multi_att_read_callback,
    .att_write_cb = &multi_att_write_callback,
    .event_packet_handler = &multi_event_packet_handler,
};

//-------------------------------------------------------------------------------------
//vm 绑定对方信息读写
static int multi_pair_vm_do(u8 *info, u8 info_len, u8 rw_flag)
{
#if PER_PAIR_BOND_ENABLE
    int ret;
    int vm_len = info_len;

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);
    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
        if (!ret) {
            log_info("-null--\n");
            memset(info, 0xff, info_len);
        }
        if (info[0] != PER_PAIR_BOND_TAG) {
            return -1;
        }
    } else {
        syscfg_write(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
    }
#endif
    return 0;
}

//清配对信息
int multi_server_clear_pair(void)
{
#if PER_PAIR_BOND_ENABLE
    ble_gatt_server_disconnect_all();
    memset(pair_bond_info, 0, sizeof(pair_bond_info));
    multi_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
    if (BLE_ST_ADV == ble_gatt_server_get_work_state()) {
        ble_gatt_server_adv_enable(0);
        multi_adv_config_set();
        ble_gatt_server_adv_enable(1);
    }
#endif
    return 0;
}

//配置连接参数更新
static void multi_send_connetion_update_deal(u16 conn_handle)
{
    if (multi_connection_update_enable) {
        if (0 == ble_gatt_server_connetion_update_request(
                     conn_handle, multi_connection_param_table,
                     CONN_PARAM_TABLE_CNT)) {
            multi_connection_update_enable = 0;
        }
    }
}

/**
 * @brief 回连状态，主动使能通知
 * 
 * @param conn_handle 
 * @param update_request 
 * 
 * @note  注册服务
 */
static void multi_resume_all_ccc_enable(u16 conn_handle, u8 update_request)
{
    log_info("resume_all_ccc_enable\n");

#if RCSP_BTMATE_EN
    ble_gatt_server_characteristic_ccc_set(
        conn_handle, ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE,
        ATT_OP_NOTIFY);
#endif
#if 0
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_INDICATE);
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
#endif
    //从仅从机服务搬过来
    ble_gatt_server_characteristic_ccc_set(
        conn_handle, ATT_CHARACTERISTIC_fff1_01_CLIENT_CONFIGURATION_HANDLE,
        ATT_OP_NOTIFY);

    if (update_request) {
        multi_send_connetion_update_deal(conn_handle);
    }
}

hci_con_handle_t fd_handle;

hci_con_handle_t get_handle(hci_con_handle_t p)
{
    fd_handle = p;
    return fd_handle;
}

//-------------------------------------------------------------------------------------
//处理gatt_common 模块返回的事件，hci & gatt
static int multi_event_packet_handler(int event, u8 *packet, u16 size,
                                      u8 *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */

    switch (event) {

    case GATT_COMM_EVENT_CAN_SEND_NOW:
        break;

    case GATT_COMM_EVENT_SERVER_INDICATION_COMPLETE:
        log_info("INDICATION_COMPLETE:con_handle= %04x,att_handle= %04x\n",
                 little_endian_read_16(packet, 0),
                 little_endian_read_16(packet, 2));
        break;

    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("peer_address_info:");
        hci_con_handle_t temp = little_endian_read_16(packet, 0);
        get_handle(temp);
        put_buf(&ext_param[7], 7);
        memcpy(cur_peer_addr_info, &ext_param[7], 7);
        multi_connection_update_enable = 1;
        pair_bond_enalbe = 0;

#if USER_BLE_DEBUG_ENABLE
        // 连接成功后，更新连接句柄
        user_ble_debug_connection_handle_update(
            little_endian_read_16(packet, 0));
#endif

        // 更新连接句柄
        user_ble_notify_connection_handle_update(
            little_endian_read_16(packet, 0));

        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n",
                 little_endian_read_16(packet, 0), packet[2]);
        if (packet[2] == 8) {
            if (pair_bond_info[0] == PER_PAIR_BOND_TAG) {
                direct_adv_count = DIRECT_ADV_MAX_CNT;
                multi_adv_config_set();
            }
        }
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%02x",
                 little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            log_info("reconnect...\n");
            multi_resume_all_ccc_enable(little_endian_read_16(packet, 0), 1);
            if (memcmp(&pair_bond_info[1], cur_peer_addr_info, 7)) {
                log_info("update peer\n");
                memcpy(&pair_bond_info[1], cur_peer_addr_info, 7);
                pair_bond_info[0] = PER_PAIR_BOND_TAG;
                multi_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
            }
        } else {
            log_info("first pair...\n");
#if PER_PAIR_BOND_ENABLE
            memcpy(&pair_bond_info[1], cur_peer_addr_info, 7);
            pair_bond_info[0] = PER_PAIR_BOND_TAG;
            multi_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
            log_info("bonding remote");
            put_buf(pair_bond_info, sizeof(pair_bond_info));
#endif
        }
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE: {
        u16 conn_interval =
            hci_subevent_le_connection_update_complete_get_conn_interval(
                ext_param);
        log_info("conn_param update_complete:%04x, interval=%d(latency=%d)\n",
                 little_endian_read_16(packet, 0), conn_interval,
                 hci_subevent_le_connection_update_complete_get_conn_latency(
                     ext_param));
        /* 手机(尤其APP请求HIGH priority时)可能把间隔改小到30ms以下,
         * 那会导致底层扫描被停掉, 这里复查并重新请求宽松参数 */
        if (conn_interval && conn_interval < 24) {
            log_info("interval too small(%d), re-request wide param\n",
                     conn_interval);
            multi_connection_update_enable = 1;
            multi_send_connetion_update_deal(little_endian_read_16(packet, 0));
        }
        break;
    }

    case GATT_COMM_EVENT_DIRECT_ADV_TIMEOUT:
        log_info("DIRECT_ADV_TIMEOUT:%d", direct_adv_count);
        if (direct_adv_count) {
            direct_adv_count--;
        } else {
            multi_adv_config_set();
        }
        break;

    case GATT_COMM_EVENT_SERVER_STATE:
        log_info("server_state: handle=%02x,%02x\n",
                 little_endian_read_16(packet, 1), packet[0]);
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT:
    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        break;

    default:
        break;
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      处理client 读操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      profile的读属性uuid 有配置 DYNAMIC 关键字，就有read_callback 回调
 */
/*************************************************************************************************/

extern u8 fff3_buf_len;
extern u8 _0103_f;
extern uint8_t Send_buffer[50];
static uint16_t multi_att_read_callback(hci_con_handle_t connection_handle,
                                        uint16_t att_handle, uint16_t offset,
                                        uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback,conn_handle =%04x, handle=%04x,buffer=%08x\n",
             connection_handle, handle, (u32)buffer);

    switch (handle) {
#if 0
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        char *gap_name = ble_comm_get_gap_name();
        att_value_len = strlen(gap_name);

        if ((offset >= att_value_len) ||
            (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s\n", gap_name);
        }
        break;
#endif

#if 0
    case ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE:
        break;
#endif
    case ATT_CHARACTERISTIC_fff1_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

#if 0
    case ATT_CHARACTERISTIC_fff3_01_VALUE_HANDLE:
        extern void fff3_fb_state(void);
        char *gap_name = ble_comm_get_gap_name();
        att_value_len = strlen(gap_name);

        if ((offset >= att_value_len) ||
            (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            if (_0103_f) {
                printf("_0103_f == 1");
                fff3_fb_state();
            }
            memcpy(buffer, &Send_buffer, fff3_buf_len);
            att_value_len = fff3_buf_len;
            printf_buf(Send_buffer, fff3_buf_len);
            // printf_buf(buffer,fff3_buf_len);
        }
        printf("ATT_CHARACTERISTIC_fff3_01_VALUE_HANDLE");

        break;
#endif

    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}

/*************************************************************************************************/
/*!
 *  \brief      处理client write操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      profile的写属性uuid 有配置 DYNAMIC 关键字，就有write_callback 回调
 */
/*************************************************************************************************/
#if 0
static int multi_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d\n", connection_handle, handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        multi_send_connetion_update_deal(connection_handle);
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        tmp16 = sizeof(multi_test_read_write_buf);
        if ((offset >= tmp16) || (offset + buffer_size) > tmp16) {
            break;
        }
        memcpy(&multi_test_read_write_buf[offset], buffer, buffer_size);
        break;

    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        log_info("\n-ae01_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (ble_comm_att_check_send(connection_handle, buffer_size)) {
            log_info("-loop send1\n");


            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;


    case ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE:
        log_info("\n-ae_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (ble_comm_att_check_send(connection_handle, buffer_size)) {
            log_info("-loop send2\n");
            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE:
        ble_op_latency_skip(connection_handle, 0xffff); //
        ble_gatt_server_set_update_send(connection_handle, ATT_CHARACTERISTIC_ae02_02_VALUE_HANDLE, ATT_OP_AUTO_READ_CCC);
#endif
        /* multi_send_connetion_update_deal(connection_handle); */
        log_info("------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_02_VALUE_HANDLE:
        log_info("rcsp_read:%x\n", buffer_size);
        ble_gatt_server_receive_update_data(NULL, buffer, buffer_size);
        break;
#endif

    case ATT_CHARACTERISTIC_ae3b_01_VALUE_HANDLE:
        log_info("\n-ae3b_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);
        break;


    default:
        break;
    }
    return 0;
}
#endif

//从从机代码移植过来
static int multi_att_write_callback(hci_con_handle_t connection_handle,
                                    uint16_t att_handle,
                                    uint16_t transaction_mode, uint16_t offset,
                                    uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

#if !TEST_TRANS_CHANNEL_DATA
    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d\n",
             connection_handle, handle, buffer_size);
#endif

    switch (handle) {
    case ATT_CHARACTERISTIC_fff1_01_CLIENT_CONFIGURATION_HANDLE:
        // set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        // trans_send_connetion_updata_deal(connection_handle);
        multi_send_connetion_update_deal(connection_handle); // 使用多从机+主机
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        // ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE:
        // 打印接收到的数据和数据长度
        log_info("\n-fff1_rx(%d):", buffer_size);
        printf_buf(buffer, buffer_size);
        parse_zd_data(buffer);
        // parse_led_strip_data(buffer, buffer_size);
        break;

#if 0   
    case ATT_CHARACTERISTIC_fff2_01_VALUE_HANDLE: // 只写
        log_info("\n-fff2_rx(%d):", buffer_size);
        // printf_buf(buffer, buffer_size);
        parse_zd_data(buffer);
        parse_led_strip_data(buffer, buffer_size);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE:
        ble_op_latency_skip(connection_handle, 0xffff); //
        ble_gatt_server_set_update_send(connection_handle,
                                        ATT_CHARACTERISTIC_ae02_02_VALUE_HANDLE,
                                        ATT_OP_AUTO_READ_CCC);
#endif
        /* trans_send_connetion_updata_deal(connection_handle); */
        log_info("------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle,
                                               buffer[0]);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_02_VALUE_HANDLE:
        log_info("rcsp_read:%x\n", buffer_size);
        ble_gatt_server_receive_update_data(NULL, buffer, buffer_size);
        break;
#endif

#if TEST_AUDIO_DATA_UPLOAD
        if (0 == memcmp(buffer, "start", 5)) {
            trans_test_send_audio_data(1);
        }
#endif
        break;
#endif

    default:
        break;
    }
    return 0;
}

//------------------------------------------------------
//
/**
 * @brief 生成adv包数据，写入buff
 * 
 * 
 * @note 可以设置从机的广播信息
 */
static u8 adv_name_ok = 0; //name 优先存放在ADV包
static int multi_make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = multi_adv_data;

    offset += make_eir_packet_val(
        &buf[offset], offset, HCI_EIR_DATATYPE_FLAGS,
        FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);
    offset += make_eir_packet_val(&buf[offset], offset,
                                  HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS,
                                  0xAF30, 2);
#if 0
    char *gap_name = ble_comm_get_gap_name();
    u8 name_len = strlen(gap_name);
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len < vaild_len) {
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name, name_len);
        adv_name_ok = 1;
    } else {
        adv_name_ok = 0;
    }
#endif

    // 广播头
    u8 info[20]; //客户机型数据
    u8 index = 0;

    info[index++] = 'Z';
    info[index++] = 'D';
    info[index++] = 0x00;
    info[index++] = 0xD9;
    info[index++] = 0x03;
    info[index++] = 0x86;
    info[index++] = 0x07;

    le_controller_get_mac(&info[index]); // 获取ble的蓝牙 public 地址

    offset += make_eir_packet_data(&buf[offset], offset,
                                   HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA,
                                   info, ARRAY_SIZE(info));

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***multi_adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("multi_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    multi_server_adv_config.adv_data_len = offset;
    multi_server_adv_config.adv_data = multi_adv_data;
    return 0;
}

//生成rsp包数据，写入buff
static int multi_make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = multi_scan_rsp_data;

#if RCSP_BTMATE_EN
    u8 tag_len = sizeof(user_tag_string);
    offset += make_eir_packet_data(&buf[offset], offset,
                                   HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA,
                                   (void *)user_tag_string, tag_len);
#endif

    if (!adv_name_ok) {
        char *gap_name = ble_comm_get_gap_name();
        u8 name_len = strlen(gap_name);
        u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
        if (name_len > vaild_len) {
            name_len = vaild_len;
        }
        offset += make_eir_packet_data(&buf[offset], offset,
                                       HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME,
                                       (void *)gap_name, name_len);
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    multi_server_adv_config.rsp_data_len = offset;
    multi_server_adv_config.rsp_data = multi_scan_rsp_data;
    return 0;
}

/**
 * @brief 广播参数设置
 * 
 */
static void multi_adv_config_set(void)
{
    int ret = 0;
    ret |= multi_make_set_adv_data();
    ret |= multi_make_set_rsp_data();

    multi_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
    multi_server_adv_config.adv_auto_do = 1;
    multi_server_adv_config.adv_channel = ADV_CHANNEL_ALL;
    memcpy(multi_server_adv_config.direct_address_info, &pair_bond_info[1],
           7); //仅从机的值== 0 ==  &pair_bond_info[1]

    if (direct_adv_count) {
        multi_server_adv_config.adv_type = ADV_DIRECT_IND;
        direct_adv_count--;
    } else {
        multi_server_adv_config.adv_type = ADV_IND; //仅从机执行的是这个句
    }

    if (ret) {
        log_info("adv_setup_init fail!!!\n");
        return;
    }
    ble_gatt_server_set_adv_config(&multi_server_adv_config);
}

#if RF24G_WATCH_DOG_DEBUG_ENABLE
/* 2.4G遥控器扫描看门狗:
 * 手机连接后, 底层扫描可能被连接事件抢占而停止(host 侧无感知),
 * 表现为不再有任何 adv report 上报 -> 2.4G遥控器彻底失效。
 * 这里只以 2s 为窗口做检测打印, 便于观察扫描失效的时间点;
 * 重开扫描的操作已屏蔽(实测无效)。 */
static void multi_rf24g_scan_watchdog(void *priv)
{
    static u32 last_cnt = 0;

    if (rf24g_adv_cnt == last_cnt) {
        /* cnt 每次相同 = 完全没有任何广播上报 */
        log_info("no adv report(cnt=%u)", rf24g_adv_cnt);
        /* multi_rf24g_scan_reopen("watchdog"); */
    }
    last_cnt = rf24g_adv_cnt;
}
#endif

//server init
void multi_server_init(void)
{
    log_info("%s", __FUNCTION__);

#if PER_PAIR_BOND_ENABLE
    if (0 == multi_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 0)) {
        log_info("server already bond dev");
        put_buf(pair_bond_info, sizeof(pair_bond_info));
        direct_adv_count = DIRECT_ADV_MAX_CNT;
    }
#endif

    ble_gatt_server_set_profile(multi_profile_data, sizeof(multi_profile_data));
    multi_adv_config_set();

#if RF24G_WATCH_DOG_DEBUG_ENABLE
    /* 扫描看门狗: 防止连接手机后底层扫描被停 */
    sys_timer_add(NULL, multi_rf24g_scan_watchdog, 2000);
#endif
}

//server exit
void multi_server_exit(void)
{
    log_info("%s", __FUNCTION__);
}

#endif
