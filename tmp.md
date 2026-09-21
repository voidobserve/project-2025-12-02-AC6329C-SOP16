 

当前工程使用的是 **BLE GATT Server（从机）**，手机 APP 作为 GATT Client 连接设备后，通过自定义服务 `FFF0` 下的三个特征进行数据交互。

**服务和特征**

在 `ble_multi_profile.h` 中定义：

| 特征   |     句柄 | 属性                   | 用途                             |
| ------ | -------: | ---------------------- | -------------------------------- |
| `FFF1` | `0x0006` | Read / Write / Notify  | APP 与设备双向通信、设备主动通知 |
| `FFF2` | `0x0009` | Write Without Response | APP 向设备发送控制指令           |
| `FFF3` | `0x000B` | Read                   | APP 读取设备状态                 |

**一、手机发送数据到设备**

BLE 协议栈收到手机写数据后，进入 `ble_multi_peripheral.c` 的：

```c
multi_att_write_callback(...)
```

主要流程是：

```text
手机 APP
  -> 写 FFF2
  -> multi_att_write_callback()
  -> parse_zd_data(buffer)
  -> parse_led_strip_data(buffer, buffer_size)
  -> 控制灯带、电机、模式、亮度等
```

对应代码：

```c
case ATT_CHARACTERISTIC_fff2_01_VALUE_HANDLE:
    parse_zd_data(buffer);
    parse_led_strip_data(buffer, buffer_size);
    break;
```

其中：

- `parse_zd_data()`：解析中道协议指令；
- `parse_led_strip_data()`：解析灯带/涂鸦 DP 协议；
- 具体实现位于 `dp_data_tran.c`。

`FFF1` 也支持写入，但当前代码只打印数据，没有实际解析：

```c
case ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE:
    printf_buf(buffer, buffer_size);
    break;
```

因此，当前 APP 的主要控制数据入口是 **FFF2**。

**二、设备发送数据给手机**

设备到 APP 有两种方式。

### 1. 通过 FFF1 主动通知

函数 zd_fb_2_app() 将设备地址和反馈数据组合后，通过 `FFF1` 发送：

```c
ble_comm_att_send_data(
    fd_handle,
    ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE,
    Send_buffer,
    len + 6,
    ATT_OP_AUTO_READ_CCC
);
```

流程为：

```text
设备状态变化
  -> zd_fb_2_app()
  -> ble_comm_att_send_data()
  -> FFF1 Notify
  -> 手机 APP 收到通知
```

APP 必须先打开 `FFF1` 的 CCC/Notify，否则设备通知不会被 APP 接收。

另外，`user_ble_debug.c` 中的 `user_ble_debug_notify()` 也使用 `FFF1` 发送调试数据。

### 2. APP 读取 FFF3 获取状态

`FFF3` 不是 Notify，而是 APP 主动读取：

```text
手机 APP
  -> Read FFF3
  -> multi_att_read_callback()
  -> fff3_fb_state()
  -> 填充 Send_buffer
  -> 返回当前设备状态
```

在 `multi_att_read_callback()` 中：

```c
case ATT_CHARACTERISTIC_fff3_01_VALUE_HANDLE:
    fff3_fb_state();
    memcpy(buffer, &Send_buffer, fff3_buf_len);
    att_value_len = fff3_buf_len;
    break;
```

`fff3_fb_state()` 会按顺序生成：

- 蓝牙地址和灯具类型；
- 开关状态；
- 流星灯开关；
- 速度、周期；
- 亮度；
- 灯带长度；
- 声音灵敏度；
- RGB 颜色；
- 闹钟；
- 电机转速等。

**总结**

当前工程的实际通信关系可以概括为：

```text
APP 写 FFF2
    -> multi_att_write_callback
    -> parse_zd_data / parse_led_strip_data
    -> 执行灯具控制

设备主动反馈 (只在测试时使用)
    -> zd_fb_2_app 
    -> FFF1 Notify 
    -> APP 接收 

APP 查询状态
    -> Read FFF3
    -> multi_att_read_callback
    -> fff3_fb_state
    -> 返回状态数据
```

需要特别注意：当前 `fff3_fb_state()` 内部大量 `ble_comm_att_send_data()` 代码是注释状态，所以它目前主要通过 **APP 读取 FFF3 返回状态**，而不是主动通知。


```text
最后需要改成app写入完成后，主动通过通知来返回数据
```