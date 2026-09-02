# F28388D CPU1 → IPC → CPU2 → SCIA 迁移说明

## 0. 工程版本与最终结果

- CCS：20.5.0
- SysConfig：1.27.0
- C2000 编译器：TI C2000 25.11.0.LTS
- 实际使用的 C2000Ware：`C:\ti\c2000\C2000Ware_26_00_00_00`
- 芯片：TMS320F28388D
- 串口：SCIA，GPIO28=RX，GPIO29=TX，115200 bit/s，8 数据位，无校验，1 停止位（8N1）
- CPU1_FLASH、CPU2_FLASH、CPU2_RAM 均已完成实际编译和链接，结果为 0 error。

最终实现的数据流为：

```text
CPU1 current_count/current_speed
        ↓  100 Hz、NONBLOCKING
DriverLib IPC Message Queue（FLAG0 / INT0）
        ↓
CPU2 IPC_0_ISR（只解包、保存、置 pending）
        ↓
CPU2 main（格式化字符串）
        ↓
SCIA / GPIO29 TX
        ↓
上位机
```

上位机接收格式保持为：

```text
enc_count:12345,current_speed:125.632\r\n
```

## 1. 修改了哪些文件

### CPU1

- `cpu1/empty_c28x_dual_driverlib_main_cpu1.c`
  - 删除 CPU1 的 SCIA 初始化、接收中断和直接串口发送调用。
  - 加入 SCIA/GPIO 所有权移交、IPC 初始化、CPU2 启动和 FLAG31 同步。
  - ePWM ISR 中仅调用 `IPC_CPU1_TickFromISR()` 产生 10 ms 标志。
  - 主循环调用 `IPC_CPU1_PollSend(current_count, current_speed)` 非阻塞发送。
- `cpu1/c2000_cpu1.syscfg`
  - 删除 SCIA、GPIO28、GPIO29 配置。
  - 保留 CPU1 的 IPC1 接收中断，以及 FLAG0、FLAG31 配置。
- `cpu1/.cproject`
  - 将原 CPU1 `APP/pc_serial/pc_serial.c` 排除出当前 CPU1_FLASH 构建，避免 CPU1 继续占用 SCIA。
  - 保持 SysConfig 文件与当前构建工具的关联。

### CPU2

- `cpu2/empty_c28x_dual_driverlib_main_cpu2.c`
  - 增加 CPU2 基础初始化、Message Queue 初始化、`Board_init()`、FLAG31 同步。
  - 在主循环中获取 pending 遥测并通过 SCIA 发送。
- `cpu2/c2000_cpu2.syscfg`
  - 增加 SCIA 配置，并固定 GPIO28/GPIO29、115200、8N1。
  - 保留 IPC0 接收中断，以及 FLAG1、FLAG31 配置。
- `cpu2/.cproject`
  - 为 CPU2_RAM 和 CPU2_FLASH 增加 `.syscfg` 构建关联。
  - 恢复编译 CPU2 本地 `device/device.c`，解决设备初始化函数链接失败。

原有 PMSM、FOC、SVPWM、速度 PI、ePWM、eQEP、编码器测速和按键算法文件没有修改。

## 2. 新增了哪些文件

- `ipc_protocol.h`：CPU1/CPU2 公共 IPC 命令及遥测周期定义。
- `cpu1/APP/ipc/ipc_cpu1.h`
- `cpu1/APP/ipc/ipc_cpu1.c`
- `cpu2/APP/ipc/ipc_cpu2.h`
- `cpu2/APP/ipc/ipc_cpu2.c`
- `cpu2/APP/pc_serial/pc_serial_cpu2.h`
- `cpu2/APP/pc_serial/pc_serial_cpu2.c`
- `IPC_SCIA迁移说明.md`：本文档。

CPU1 原有 `APP/pc_serial` 文件没有删除，只是在 CPU1_FLASH 中排除，便于后续参考旧命令协议。当前阶段没有实现“上位机 → CPU2 → IPC → CPU1”业务命令，但反向 IPC 队列和 CPU1 IPC1 ISR 已保留。

## 3. CPU1 的 IPC 初始化位置

CPU1 主程序在 `Board_init()` 之后依次执行：

```c
IPC_CPU1_AssignSCIAtoCPU2();
IPC_CPU1_Init();
IPC_CPU1_SyncWithCPU2();
```

对应实现位于 `cpu1/APP/ipc/ipc_cpu1.c`：

```c
IPC_initMessageQueue(IPC_CPU1_L_CPU2_R,
                     &g_ipcQueue,
                     IPC_INT1,
                     IPC_INT0);
```

含义如下：

- CPU1 本地接收：IPC1 / FLAG1，用于以后 CPU2 → CPU1。
- CPU1 远端发送：IPC0 / FLAG0，用于当前 CPU1 → CPU2 遥测。
- `IPC_CPU1_SyncWithCPU2()` 先调用 `Device_bootCPU2(BOOT_MODE_CPU2)`，再使用 FLAG31 调用 `IPC_sync()`。
- CPU1_FLASH 下 `BOOT_MODE_CPU2` 由 SysConfig 映射到 CPU2 Flash Sector 0。

## 4. CPU2 的 IPC 初始化位置

CPU2 主程序初始化顺序为：

```c
Device_init();
Device_initGPIO();
Interrupt_initModule();
Interrupt_initVectorTable();
IPC_CPU2_Init();
Board_init();                 // 注册 IPC_0_ISR，并初始化 SCIA
IPC_CPU2_SyncWithCPU1();
EINT;
ERTM;
```

`IPC_CPU2_Init()` 位于 `cpu2/APP/ipc/ipc_cpu2.c`：

```c
IPC_initMessageQueue(IPC_CPU2_L_CPU1_R,
                     &g_ipcQueue,
                     IPC_INT0,
                     IPC_INT1);
```

含义如下：

- CPU2 本地接收：IPC0 / FLAG0。
- CPU2 远端发送：IPC1 / FLAG1，保留给未来的反向命令。
- CPU2 与 CPU1 同样使用 FLAG31 同步。

## 5. SCIA 是如何从 CPU1 迁移到 CPU2 的

1. 从 `cpu1/c2000_cpu1.syscfg` 删除 SCIA 实例，因此 CPU1 生成的 `board.c/board.h` 不再初始化或声明 `mySCI1`。
2. CPU1 在启动 CPU2 前调用当前 C2000Ware 26.00 中真实存在的 DriverLib API：

```c
SysCtl_selectCPUForPeripheralInstance(SYSCTL_CPUSEL_SCIA,
                                      SYSCTL_CPUSEL_CPU2);
GPIO_setControllerCore(28U, GPIO_CORE_CPU2);
GPIO_setControllerCore(29U, GPIO_CORE_CPU2);
```

3. 在 `cpu2/c2000_cpu2.syscfg` 增加 SCIA：GPIO28 RX、GPIO29 TX、115200、8N1。
4. CPU2 获得所有权后通过 `Board_init()` 初始化 SCIA。
5. CPU1 原有 SCIA ISR 和直接发送路径不再参与构建；CPU2 主循环负责最终串口发送。

串口助手应设置为 115200、8 数据位、无校验、1 停止位。CPU2 GPIO29（TX）接串口模块 RX；如以后接收命令，CPU2 GPIO28（RX）接串口模块 TX；两者必须共地。

## 6. CPU1 → CPU2 的 IPC Message 数据格式

公共命令定义：

```c
#define IPC_CMD_MOTOR_TELEMETRY 0x1001U
```

`IPC_Message_t` 字段使用如下：

| 字段 | 内容 |
| --- | --- |
| `command` | `IPC_CMD_MOTOR_TELEMETRY`（0x1001） |
| `address` | `0U`，未使用 |
| `dataw1` | `current_count` 的 32 位值 |
| `dataw2` | `current_speed` 的 IEEE-754 原始 32 位 bit 数据 |

CPU1 使用 `memcpy()` 将 float 原始位复制到 `dataw2`，CPU2 再用 `memcpy()` 恢复，未使用会丢失小数的 `(uint32_t)current_speed` 数值转换。地址修正为 `IPC_ADDR_CORRECTION_DISABLE`。

CPU1 每约 10 ms 尝试发送一次：

```c
IPC_sendMessageToQueue(..., IPC_NONBLOCKING_CALL);
```

队列满时直接丢弃本次旧遥测，不阻塞控制流程，并递增：

- `g_ipc_tx_count`：成功入队数量。
- `g_ipc_tx_drop_count`：队列满导致的丢弃数量。

## 7. CPU2 `IPC_0_ISR` 的处理流程

`IPC_0_ISR` 位于 `cpu2/APP/ipc/ipc_cpu2.c`，处理步骤为：

1. 使用 `IPC_readMessageFromQueue(..., IPC_NONBLOCKING_CALL)` 读取并排空当前消息。
2. 将命令保存到 `g_ipc_last_command`，递增 `g_ipc_rx_count`。
3. 判断 `message.command == IPC_CMD_MOTOR_TELEMETRY`。
4. 从 `dataw1` 恢复 `g_received_encoder_count`。
5. 使用 `memcpy()` 从 `dataw2` 恢复 `g_received_current_speed`。
6. 设置 `g_ipc_telemetry_pending = true`。
7. 调用 `IPC_ackFlagRtoL(..., IPC_FLAG0)` 应答 FLAG0。
8. 调用 `Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1)` 清 PIE ACK。

ISR 中没有 `snprintf()`、字符串处理或 SCI 阻塞发送。CPU2 主循环通过 `IPC_CPU2_TakeTelemetry()` 做受保护的数据快照并清除 pending，然后调用串口模块。

可在 CCS Expressions 中观察：

- CPU1：`g_ipc_tx_count`、`g_ipc_tx_drop_count`
- CPU2：`g_ipc_rx_count`、`g_ipc_last_command`、`g_received_encoder_count`、`g_received_current_speed`、`g_ipc_telemetry_pending`

## 8. 最终数据流

1. CPU1 的 ePWM1 ISR 保持原有编码器测速和电机控制计算，更新 `current_count/current_speed`。
2. ePWM1 ISR 每 100 个 10 kHz tick 只设置一次遥测 pending，即约 10 ms/100 Hz。
3. CPU1 主循环将最新计数和速度打包为 `IPC_Message_t`，以 NONBLOCKING 模式发送。
4. Message Queue 设置 FLAG0，触发 CPU2 IPC0 中断。
5. CPU2 `IPC_0_ISR` 解包并保存最新遥测，不打印。
6. CPU2 主循环看到 pending 后格式化：

```text
enc_count:xxx,current_speed:xxx.xxx\r\n
```

7. CPU2 通过 SCIA/GPIO29 TX 发送到上位机。

CPU2 → CPU1 的 FLAG1/IPC1 配置、双向 Message Queue 和 CPU1 `IPC_1_ISR` 均保留；该 ISR 当前只排空并应答预留消息，不执行控制业务。

## 9. 编译过程中发现和修复的问题

### 9.1 C2000Ware 产品解析

两个 `.syscfg` 的产品参数固定为：

```text
C:/ti/c2000/C2000Ware_26_00_00_00/.metadata/sdk.json
```

避免再次出现 `No product with name C2000WARE and version 26.00.00.00 found`。

### 9.2 CPU2 SysConfig 未与两个构建配置完整关联

CPU2 原 `.cproject` 没有针对 CPU2_RAM/CPU2_FLASH 的 SysConfig `fileInfo`，清理后可能不再生成 `syscfg/board.opt`、`board.c` 等文件。现已为两个配置分别增加构建关联。

### 9.3 CPU2 排除了 `device/device.c`

首次 CPU2_FLASH 链接出现：

```text
unresolved symbol: Device_init
unresolved symbol: Device_initGPIO
unresolved symbol: __error__
```

根因是两个 CPU2 配置都排除了本地 `device/device.c`。现已恢复该文件参与 CPU2_RAM/CPU2_FLASH 编译，同时继续由 SysConfig 生成 `board.c/board.h`。

### 9.4 CPU1 原串口文件与迁移后的 SysConfig 冲突

移除 CPU1 SCIA 后，旧 `pc_serial.c` 仍依赖 `mySCI1_BASE`。该文件现已从 CPU1_FLASH 排除，CPU1 控制代码改为调用 IPC 模块，避免 CPU1 再次访问 SCIA。

### 9.5 最终编译结果

- CPU1_FLASH：0 error；2 个既有/生成代码警告。
- CPU2_FLASH：0 error；1 个 SysConfig IPC 双核校验提示。
- CPU2_RAM：0 error；1 个 SysConfig IPC 双核校验提示。

IPC 警告内容是 SysConfig 在单个 CPU 上下文中无法完整检查另一核；两侧的 FLAG/INT 对应关系已按要求手工核对，不是编译错误。

## 烧录与首次验证建议

1. 分别烧录 CPU2_FLASH 与 CPU1_FLASH 的 `.out` 文件。
2. 复位后由 CPU1 从 CPU2 Flash Sector 0 启动 CPU2，双方在 FLAG31 同步点会合。
3. 串口助手设置为 115200/8N1，停止位必须为 1。
4. 若无输出，先观察 CPU1 `g_ipc_tx_count`，再观察 CPU2 `g_ipc_rx_count`：
   - CPU1 计数不增：CPU1 尚未进入正常主循环或卡在 CPU2 同步。
   - CPU1 增、CPU2 不增：检查 CPU2 是否已烧录/启动，以及 FLAG0/IPC0 中断。
   - 两者都增但串口无输出：检查 SCIA TX 的 GPIO29、串口模块 RX 和公共 GND。

