

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

