#### 指令格式

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

