

### communication process

```sequence
Title:singal communication process
mesh/host->host/mesh: data 
host/mesh->mesh/host: ack data

```

​	this way make sure that if recving ack, the data must be sended success, but not be responsible for fail 

return result

#### 

### UDP FOUND

​	mesh send UDP broadcast packet to specific port, host listen the port will return ack packet. packet have such information: MeshID

##### format

```json
{
    "MeshID":"00:00:00:00:00:00"
}
```



### raw data format

#### host to mesh

| 0:5     | 6    | 7:   |
| ------- | ---- | ---- |
| reserve | type | data |

#### mesh to host

| 0:5     | 6    | 7:   |
| ------- | ---- | ---- |
| mesh_id | type | data |

#### type

| type | size | value |
| ---- | ---- | ----- |
| BIN  | 1    | 0     |
| STR  | 1    | 1     |



### host to mesh

##### format(str)

```json
{
    "Typ":"ota",
    "Devcies":["ff:ff:ff:ff:ff:ff"],
    "CusData":"fasd"
}
```

> `Typ` cmd name
>
> `Devcies` can not exist, target devices
>
> `CusData` can not exist, additional data for some cmd



##### Typ List

| cmd          | additional | execer |
| :----------- | ---------- | ------ |
| Ota          |            | root   |
| ReportGather |            | root   |
| ReportNum    |            | root   |
| SendCustom   |            | root   |



### mesh to host

#### format

| type   | type | func                                                    |
| ------ | ---- | ------------------------------------------------------- |
| gather | bin  | report normal device info                               |
| brust  | str  | report urgent device info                               |
| num    | str  | when mesh is not initialization, mesh send this to host |

#### gather

| 0:5  | 6        | 7:          |
| ---- | -------- | ----------- |
| mac  | data len | device_info |

#### brust

```json
{
    "Typ":"brust",
    "Mac":"11:11:11:11:11:11",
    "ParentMac":"11:11:11:11:11:11",
    "Layer": 1,
    "DeviceInfo":{}
}
```

#### num

```json
{
    "Typ":"num",
	"Num":12
}
```





#### MESH_OTA指令格式

##### start ota

```json
{
    "cmd":"ota",
    "type":1,
    "version":"0.0.1",
    "bin_len":1234,
}
```



##### ota status check

```json
{
	"cmd":"ota",
    "type":3
}
```

##### ota status check[ack]

```c
bool bin_status[MAX_CHEC_BIT]
```





##### ota end

```json
{
	"cmd":"ota",
    "type":5
}
```



##### ota切片包数据格式

| offset | type     | data               |
| ------ | -------- | ------------------ |
| 0-1    | uint16_t | ota packet seq     |
| 2-1001 | byte     | ota packet payload |
|        |          |                    |



#### serial ota指令格式

##### start ota

```json
{
    "cmd":"ota",
    "type":1,
    "version":"0.0.1",
    "bin_len":1234,
}
```



##### ota status check

```json
{
	"cmd":"ota",
    "type":3
}
```

##### ota status check[ack]

```c
bool bin_status[MAX_CHEC_BIT]
```





##### ota end

```json
{
	"cmd":"ota",
    "type":5
}
```



##### ota切片包数据格式

| offset | type     | data               |
| ------ | -------- | ------------------ |
| 0-1    | uint16_t | ota packet seq     |
| 2-1001 | byte     | ota packet payload |
|        |          |                    |



### 蓝牙配网协定

#### 版本：V1

#### 改动：初始版本

#### 框架：乐鑫BLUFI通讯协定，<https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/blufi.html

#### app参考示例：https://github.com/EspressifApp/EspBlufiForAndroid

#### 设备端参考示例：https://github.com/espressif/esp-idf/tree/8b885fb93530c2fb52d7b63d29b7c89defbc8940/examples/bluetooth/blufi

#### 通讯对象：单个设备

#### 通讯方式：蓝牙

#### 通讯流程：勾选方式

- APP扫描设备，发现所有设备名称为MCB_LIGHT的设备并储存，在界面上以列表形式显示名称
- 把用户勾选的设备的蓝牙mac地址存入设备列表
- 选择勾选设备中的一个信号最强的设备进行通讯
- APP与此设备建立连接
- APP请求加密通讯
- APP发送CONFIG数据（CUSTOM）设置信息
- APP等待接收设备的配网状态（CUSTOM）来判断设备是否配网成功
- APP结束连接

#### 通讯流程：一键配网

- APP扫描设备，发现所有设备名称带有MCB_LIGHT前缀的设备并储存，在界面上以列表形式显示名称
- 用户点击使用一键配网后，从发现的设备中选择一个信号最强的的设备进行通讯
- APP与此设备建立连接
- APP请求加密通讯
- APP发送一键CONFIG数据（CUSTOM）设置信息
- APP等待接收设备的配网状态（CUSTOM）来判断设备是否配网成功
- APP结束连接

#### 错误处理:

- app与设备建立连接后，发送数据是否成功根据BLUFI类方法的返回值来判 断，如果不成功需要立即断开连接，并提示用户配网失败
- app等待配网是否成功的超时时间为10s，如果超时无相应，则进行一次配网状态查询（CUSTOM），如果在3s内还是没有接收到配网状态信息或者类返回失败，则关闭连接，通知失败

##### CONFIG数据（CUSTOM）格式：

```json
{
    "type": "config",
    "ssid": "MESH_ROUTER",
    "password": "12345678",
    "mesh_id": "aa:aa:aa:aa:aa:aa",
    ble_macs: ["aa:aa:aa:aa:aa:aa", "bb:bb:aa:aa:aa:aa"],
    "broadcast":0
}
```

`type`：固定为“config”
`ssid`：wifi名，限制为31个字节，如果为“NOROUTER”表示使用无router mesh
`password`：密码，限制为63个字节
`mesh_id`：配置的mesh_id
`broadcast`: 固定为0
`ble_macs`: 设备列表

##### 一键CONFIG数据（CUSTOM）格式：

```json
{
    "type": "config",
    "ssid": "MESH_ROUTER",
    "password": "12345678",
    "mesh_id": "aa:aa:aa:aa:aa:aa",
    "broadcast":1
}
```

`type`：固定为“config”
`ssid`：wifi名，限制为31个字节，如果为“NOROUTER”表示使用无router mesh
`password`：密码，限制为63个字节
`broadcast`: 固定为1
`mesh_id`：配置的mesh_id

##### 配网状态查询（CUSTOM）格式：

```json
{
    "type": "wifi_status"
}
```

`type`：固定为”wifi_status”

##### 配网状态（CUSTOM）格式：

```json
{
    "status": 0
}
```

`status`: 0成功,-1失败