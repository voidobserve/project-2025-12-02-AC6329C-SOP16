
---

> [!NOTE]
>
> 需要注意的宏定义
>
> USER_TO_DO
>
> REVIEW
>
> TEST ONLY


# 特殊说明

- 当前工程没有app控制的功能，因此它的广播功能可以关掉
- 动态速度的调节并不是均匀的，没有按照百分比来划分
- 客户的遥控在松手后还会再发送一段时间的数据包，这个时候不好判断松手。而且每帧数据的间隔又比较松散，中间容易丢失数据。在扫描键值的时候，程序里加了补偿时间(依靠调节得出)
- 当前项目只有两种动画速度值

## 手机连接后 2.4G 遥控器失效（已定位）

**现象**：手机通过 BLE 连接上之后，遥控器完全无反应；把手机断开立刻恢复正常。log 里 `[rf24g] adv cnt` 停止增长，但 host 侧扫描状态仍返回 `BLE_ST_SCAN`，也没有 `drop scan_report!!!` 打印 —— 说明是控制器**静默停掉了扫描**，host 侧无感知、不会自恢复。

**根因**：BLE 连接间隔(conn interval) 小于约 30ms 时，控制器会停止调度扫描。用 nRF Connect 逐个请求连接优先级实测（同一台手机）：

| 优先级 | 实际间隔 | 遥控器 |
| --- | --- | --- |
| HIGH (11.25~15ms) | 约 9~12 | 失效 |
| BALANCED (30~50ms) | 约 24~40 | 正常 |
| LOW POWER (100~125ms) | 约 80~100 | 正常 |

实测边界：interval = 24(30ms) 正常，21(26.25ms) 失效。

不同手机表现不一样的原因：iOS 默认给较宽松的间隔，而 Android / 部分 APP 会主动请求 HIGH priority。

**修复**（`apps/spp_and_le/examples/multi_conn/ble_multi_peripheral.c`）：
- 连接参数请求表 `multi_connection_param_table` 全部抬到 ≥24：`{24,40,10,600}` 等三组。原值 `{16,24,10,600}` 的下限只有 20ms 落在死区，手机会在区间内任选（实测会选到 21）。该请求在手机写 CCC 时由 `multi_send_connetion_update_deal()` 触发。
- `GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE` 里复查 interval，小于 24 就重新请求一次，防止 APP 事后又把间隔改小。

**以下手段已验证无效，不要再走**：
- 连接后 / 参数更新后重开扫描（`scan_enable(0)` + `(1)`）：命令返回成功但不起作用
- 2 秒看门狗检测到无广播后重开扫描
- 降低扫描窗口（24→8）、改成被动扫描
- 加大控制器状态机数 `config_btctler_le_hw_nums`
- `config_vendor_le_bb = VENDOR_BB_NEW_SCAN_STRATEGY`

**调试开关**（都在 `apps/user_app/rf24g/rf24g_parse.h`）：
- `RF24G_DEBUG_LOG`：置 1 打开 2.4G 遥控器广播的解析打印
  - `[rf24g] adv cnt=..`：每 50 次广播上报打印一条。连接手机后这条停了，就说明 BLE 扫描被停掉了
  - `[rf24g] remote hit: key=..`：每收到一个合法遥控器包打印一条，用来判断遥控器的数据有没有进来
- `RF24G_WATCH_DOG_DEBUG_ENABLE`：置 1 打开扫描看门狗（同时开启广播上报计数 `rf24g_adv_cnt`）
  - 每 2 秒检查一次，若这 2 秒内一条广播上报都没有，就打印 `no adv report(cnt=..)`，用来观察扫描失效的时间点、以及失效前计数停在多少
