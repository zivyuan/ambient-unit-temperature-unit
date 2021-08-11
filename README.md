# 迷你气象站

## ADS1115 地址选择
| PIN | Address |
| - | - |
| GND | 0x48 |
| VCC | 0x49 |
| SDA | 0x4A |
| SCL | 0x4B |


## 设备上代码上传

使用FT232 USB转TTL进行数据上传

1. 根据丝印连接好线
2. 运行代码下载程序, 当界面提示正在连接时, 按下 Flash 键直接到上传开始后即可松开
  2.1. 如果按下 Flash 键后还在等待连接, 这时可以按一下 Restart 键
  2.2. 看到上传进度后可松开 Flash 键
  ```
  Serial port /dev/cu.usbserial-AH03AC2W
  Connecting........
  ```
3. 代码上传结束, 如果需要使用串口调试的话, 需要把 RST 连线断开


## MQTT 资源
1. [ESP8366 接入阿里云IoT](https://developer.aliyun.com/article/761838)
2. [MQTT TCP连接](https://help.aliyun.com/document_detail/73742.html)
3. [基于MQTT通道的设备动态注册](https://help.aliyun.com/document_detail/132111.html)

# ESP8266 睡眠模式

| 项目 | 调制解调器睡眠 | 浅睡眠 | 沉睡 |
| - | - | - | - |
| 无线上网 | 关 | 关 | 关 |
| 系统时钟 | 上 | 关 | 关 |
| 实时时钟 | 上 | 上 | 上 |
| 中央处理器 | 上 | 待定 | 关 |
| 基板电流 | 15毫安 | 0.4毫安 | 约20 uA |
| 平均电流（DTIM = 1）| 16.2毫安 | 1.8毫安 | – |
| 平均电流（DTIM = 3）| 15.4毫安 | 0.9毫安 | – |
| 平均电流（DTIM = 10）| 15.2毫安 | 0.55毫安 | –  |

### 资料

1. [ESP8266深度睡眠](https://www.bilibili.com/read/cv5988390)


# Error Code

## WiFi Error Code

| Code | Error | Comment |
| :-: | - | - |
| 255 | WL_NO_SHIELD | For compatibility with WiFi Shield library |
| 0 | WL_IDLE_STATUS | |
| 1 | WL_NO_SSID_AVAIL | |
| 2 | WL_SCAN_COMPLETED | |
| 3 | WL_CONNECTED | |
| 4 | WL_CONNECT_FAILED | |
| 5 | WL_CONNECTION_LOST | |
| 6 | WL_WRONG_PASSWORD | |
| 7 | WL_DISCONNECTED | |


## MQTT Error Code

[物联网平台错误代码](https://help.aliyun.com/document_detail/148610.html)

| Code | Error | Comment |
| :-: | - | - |
| 0 | 0x00 Connection Accepted | 连接成功。 | |
| 1 | 0x01 Connection Refused, unacceptable protocol version | 服务器不支持设备端请求的MQTT协议版本。 | |
| 2 | 0x02 Connection Refused, identifier rejected | clientId参数格式错误，不符合物联网平台规定的格式。例如参数值超出长度限制、扩展参数格式错误等。 | |
| 3 | 0x03 Connection Refused, Server unavailable | 网络连接已建立成功，但MQTT服务不可用。 | |
| 4 | 0x04 Connection Refused, bad user name or password | username或password格式错误。 | |
| 5 | 0x05 Connection Refused, not authorized | 设备未经授权。 | |