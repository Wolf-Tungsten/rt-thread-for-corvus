# 卫星站驱动 (Satellite Station Driver) API 文档

本文档介绍了用于与硬件卫星站（Satellite Station）进行通信的驱动程序 API。

这些 API 提供了一组接口，用于初始化卫星站、发送和接收消息、管理接收缓冲区、以及配置同步标志和中断。

当前硬件把 StateBus 分成两类：

- `MBus`：会经过 master station 和全部 satellite station。
- `SBus`：只会经过全部 satellite station，不经过 master station。

默认配置下，`nMBus = 2`、`nSBus = 2`、`nStateBus = 4`。对 satellite hart，`0 ..< nMBus` 是 MBus 通道，`nMBus ..< nStateBus` 是 SBus 通道；对 master hart，只能看到本地 `nMBus` 条 MBus 通道。

头文件还提供：

- `sat_BusCount`：当前 hart 可见的总 StateBus 数量。
- `sat_MBusCount`: 当前 hart 可见的 MBus 数量
- `sat_SBusCount`: 当前 hart 可见的 SBus 数量
- `SAT_MBUS_CH(ch)`：把第 `ch` 条 MBus 映射到本地通道号。会过滤不合法的 `ch` 值。
- `SAT_SBUS_CH(ch)`：把第 `ch` 条 SBus 映射到本地通道号。会过滤不合法的 `ch` 值。

## 头文件
请在使用前包含相应的头文件：
```c
#include "drv_satellitestation.h"
```

## API 概览

### 1. `sat_init`
**函数原型:**
```c
void sat_init(void);
```

**功能描述:**
初始化卫星站驱动。 这个函数 **不会** 开启接收缓冲区中断。
在使用任何卫星站的功能前，需要执行本函数。

---

### 2. `sat_send`
**函数原型:**
```c
void sat_send(int ch, uint16_t target, uint64_t payload);
```

**功能描述:**
通过指定通道发送消息给目标节点。

**参数:**
- `ch`: 发送通道编号（必须小于当前 hart 本地可见的状态总线数目 `sat_BusCount`）。
- `target`: 目标节点的 ID。（若发往主控站（站点0），则ch不能是SBus）
- `payload`: 要发送的数据，低 48 bit 有效。

---

### 3. `sat_recv`
**函数原型:**
```c
uint64_t sat_recv(int ch);
```

**功能描述:**
从指定通道接收一条消息的有效载荷。

**参数:**
- `ch`: 接收通道编号（必须小于当前 hart 本地可见的状态总线数目 `sat_BusCount`）。

**返回值:**
返回接收到的 48 bit 数据，右对其。读取后即出队。

---

### 4. `sat_receiveBufferCnt`
**函数原型:**
```c
uint64_t sat_receiveBufferCnt(int ch);
```

**功能描述:**
获取指定接收通道的缓冲区内现存未读的消息数量。

**参数:**
- `ch`: 通道编号（必须小于当前 hart 本地可见的状态总线数目 `sat_BusCount`）。

**返回值:**
返回该通道接收缓冲区中的消息计数。

---

### 5. `sat_sendBufferCnt`
**函数原型:**
```c
uint64_t sat_sendBufferCnt(int ch);
```

**功能描述:**
获取指定发送通道当前已经排队、尚未被 corvus 侧取走的消息数量。

**参数:**
- `ch`: 通道编号（必须小于当前 hart 本地可见的状态总线数目 `sat_BusCount`）。

**返回值:**
返回该通道发送缓冲区中的消息计数。

---

### 6. `sat_clearBuffer`
**函数原型:**
```c
void sat_clearBuffer(int ch);
```

**功能描述:**
清空指定接收通道的缓冲区。该函数会循环拉取所有缓冲中的消息直到队列为空。

**参数:**
- `ch`: 通道编号（必须小于当前 hart 本地可见的状态总线数目 `sat_BusCount`）。

---

### 7. `sat_set_outSyncFlag`
**函数原型:**
```c
void sat_set_outSyncFlag(uint64_t flag);
```

**功能描述:**
设置向外发送的同步标志位 (`outSyncFlag`)。

**参数:**
- `flag`: 同步标志位的值。

---

### 8. `sat_get_inSyncFlag`
**函数原型:**
```c
uint64_t sat_get_inSyncFlag(void);
```

**功能描述:**
读取外部输入的同步标志位 (`inSyncFlag`)。

**返回值:**
返回当前的同步标志位。

---

### 9. `sat_interrupt_install`
**函数原型:**
```c
rt_isr_handler_t sat_interrupt_install(rt_isr_handler_t handler, void* param);
```

**功能描述:**
安装并使能卫星站在接收缓冲区满的中断处理函数。

中断服务函数的类型为`void (int vector, void* param)`。其中，`vector`固定为`satelliteIRQNum`,
`param`与注册时传入的`param`相同。

**参数:**
- `handler`: 指向中断服务程序的函数指针。
- `param`: 传递给中断服务程序的参数指针。当非空时，其生命周期必须延续至中断处理时。

**返回值:**
如果在重新安装时有旧版本中断回调，返回旧的中断处理函数指针。

---

### 10. `sat_dump_regs`
**函数原型:**
```c
void sat_dump_regs(void);
```

**功能描述：**
调试用，打印当前站点的卫星站的寄存器布局。

## 地址映射说明

- 只读状态区大小为 `pow2ceil(1 + 2 + 2 * sat_BusCount)` 个 64-bit 寄存器。
- 偏移 0 为 `inSyncFlag`。
- 偏移 1 为当前站点可用的`MBus`数量。
- 偏移 2 为当前站点可用的`SBus`数量
- 偏移 3 .. `3+sat_BusCount-1` 为接收方向 `toCoreStateBusBuffer[i].count`。
- 偏移 `3+sat_BusCount` .. `3+2 * sat_BusCount-1` 为发送方向 `fromCoreStateBusBuffer[i].count`。
