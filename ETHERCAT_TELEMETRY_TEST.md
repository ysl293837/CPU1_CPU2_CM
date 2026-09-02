# F28388D 三核遥测与 EtherCAT 验证说明

## 1. 当前三核职责

- **CPU1**：PMSM 实时控制、ePWM/EQEP/FOC/PI、产生 `current_count` 与 `current_speed`；同时配置 EtherCAT 时钟、GPIO 和外设归属。
- **CPU2**：保持原有 SCIA 上位机通信，发送 `enc_count` 与 `current_speed` 文本遥测，未改动 CPU1→CPU2 IPC 队列和串口协议。
- **CM**：接收 CPU1 遥测、运行 TI 官方 EtherCAT CM HAL，并准备 EtherCAT TxPDO 数据源。CM 不访问 CPU1 的 FOC/ePWM/EQEP 全局变量。

## 2. 两条遥测数据流

```text
CPU1 current_count/current_speed
      ├─ IPC_CPU1_L_CPU2_R → CPU2 IPC_0_ISR → SCIA → 串口助手
      └─ IPC_CPU1_L_CM_R   → CM IPC1 ISR   → TxPDO 数据影子 → SSC → EtherCAT → TwinCAT
```

CPU1 每 100 次 ePWM ISR 设置一次 pending 标志。以约 10 kHz ISR 频率计算，两个 IPC 队列都约每 10 ms 从 CPU1 主循环发送一帧；发送 API 都是 `IPC_NONBLOCKING_CALL`，CM 忙不会阻塞电机控制 ISR。

## 3. CPU1→CM IPC 实现

CPU1 代码在 `cpu1/APP/ipc/ipc_cpu1.c`：

- `IPC_CPU1_CM_Init()` 使用 `IPC_CPU1_L_CM_R` 建立独立 Message Queue。
- 队列使用 `IPC_INT1`；CPU2 现有队列仍保持原来的 `IPC_CPU1_L_CPU2_R`，没有替换或复用。
- `IPC_CPU1_CM_SyncWithCM()` 与 CM 在 `IPC_FLAG31` 完成一次启动会合；实际遥测使用 IPC1/`IPC_FLAG1`，不会与启动同步或 CM boot 的 FLAG0 混用。
- `IPC_CPU1_PollSend()` 仍先发送 CPU2，然后把同一个完整快照非阻塞发送给 CM。

IPC 消息格式保持 `ipc_protocol.h` 的 `IPC_CMD_MOTOR_TELEMETRY`：

| 字段 | 含义 | 类型/字节 |
|---|---|---|
| `command` | `IPC_CMD_MOTOR_TELEMETRY` (`0x1001`) | `uint32_t` |
| `dataw1` | `current_count` | `int32_t`，4 字节 |
| `dataw2` | `current_speed` 的原始 IEEE-754 位模式 | `float`，4 字节 |

CM 端通过 `memcpy()` 恢复 `dataw2`，没有使用不安全的指针强转。可在 CCS Expressions 查看：

- CPU1：`g_cm_ipc_tx_count`、`g_cm_ipc_tx_drop_count`
- CM：`g_cm_ipc_rx_count`、`g_cm_ipc_error_count`、`g_cm_last_encoder_count`、`g_cm_last_current_speed`

## 4. CM 端的当前实现

`CM/empty_driverlib_main_cm.c` 的启动顺序是：

1. `CM_init()` 建立 CM 向量表和外设时钟；
2. 初始化并注册 CPU1→CM 的 IPC1 NVIC ISR；
3. 与 CPU1 在 FLAG31 同步，避免 CPU1 向未初始化队列持续发送；
4. 调用官方 `ESC_initHW()`；
5. 主循环把最近一次完整 IPC 快照更新到 `g_ecat_motor_telemetry_txpdo`。

`g_ecat_motor_telemetry_txpdo` 的布局固定为 8 字节：

| 计划对象 | 成员 | EtherCAT 数据类型 | 位宽 |
|---|---|---:|---:|
| `0x6000:01` | `encoderCount` | DINT / INTEGER32 | 32 |
| `0x6000:02` | `currentSpeed` | REAL32 | 32 |
| `0x1A00` | 上述两个成员的 TxPDO 映射 | 总计 | 64 / 8 字节 |

`g_ecat_pdo_update_count` 仅在收到一份新的完整 IPC 快照并更新该 8 字节影子时递增。`g_ecat_al_state` 在 `ESC_initHW()` 成功后读取 ESC 的 AL Status 寄存器。

TI 官方 HAL 在 `ESC_initHW()` 内运行时注册 `INT_ECAT`、`INT_ECAT_SYNC0`、`INT_ECAT_SYNC1` 的处理函数，因此这三项不会继续使用 `startup_cm.c` 的默认处理函数。当前官方 HAL 没有为 `INT_ECAT_RST` 提供可猜测的通用 SSC 回调；待真实 SSC 生成后，必须以生成代码实际提供的 ISR/回调为准再接入，不能编造一个处理函数。

## 5. SSC 协议栈状态：尚缺一个人工步骤

本机已搜索 C2000Ware、桌面、文档和下载目录，未发现以下 SSC 生成文件：`ecat_def.h`、`ecatslv.c`、`ecatappl.c`、`ecatcoe.c`、`objdef.c`、`applInterface.c` 或 ESI XML。

因此，当前 CM 输出包含**官方 EtherCAT HAL**，也包含真实 CPU1→CM IPC 和 PDO 数据源；但它**不包含 Beckhoff SSC 协议栈**，不能声称已经能被 TwinCAT 扫描、进入 OP 或发送真实 PDO。这不是代码可安全补写的部分。

请你完成下面唯一的 SSC GUI 操作后，把生成目录保留在磁盘并告诉我路径；我会继续把栈、对象字典、PDO 映射和 ESI 接入当前工程：

1. 安装 Beckhoff **SSC Tool V5.12**。TI 的 F2838x 用户指南明确要求该版本。
2. 运行 TI 安装程序：`C:\ti\c2000\C2000Ware_26_00_00_00\libraries\communications\Ethercat\f2838x\ethercat_slave_ssc_and_demo_setup.exe`。它会生成 `ssc_configuration` 目录。
3. 打开 SSC Tool；在 New Project 对话框选择 **Import**，打开新生成的 `ssc_configuration\f2838x_ssc_config.xml`。
4. 在 **Custom** 下拉项选择 **TI F2838x CM Sample**（不带 sample application 的那个），然后确认外部文件提示并保存 SSC 工程。
5. 在 SSC 中配置对象字典：建立 `0x6000 Motor Telemetry`，含 `0x6000:01 EncoderCount (DINT, 32 bit)`、`0x6000:02 CurrentSpeed (REAL32, 32 bit)`；在 `0x1A00` 映射这两个成员，共 64 bit。
6. 执行 **Project → Create new Slave Files**。对于 CM Sample，TI 指南允许 Source Folder 和 ESI Directory 保持导入后的默认路径；点击 Start、OK。不要把空的 `CM/APP` 目录误当作 SSC 模板目录。
7. 生成后把完整 SSC C/H 源码与 ESI XML 的实际路径发给我。届时将由生成文件的真实变量名决定把 `g_ecat_motor_telemetry_txpdo` 复制到 SSC 的 `0x6000` 对象，以及在哪个 application callback 中执行，不猜测函数名。

生成后的 ESI XML 可复制到 `C:\TwinCAT\3.1\Config\Io\EtherCAT`；复制后关闭并重开 TwinCAT，令其重新加载 ESI。当前没有生成的 ESI XML，因此这里没有可复制的 XML 文件路径。

## 6. CPU1 SysConfig 与引脚冲突处理

`cpu1/c2000_cpu1.syscfg` 中已经明确设置：

```js
ECAT1.ESC_LATCH0.$used = false;
ECAT1.ESC_LATCH1.$used = false;
```

并删除了 LATCH0→GPIO29、LATCH1→GPIO30 的建议解。CPU1 的 `ECAT_GPIO_init()` 继续保留 LATCH0/LATCH1 不配置的策略；CPU2 的 SysConfig 保持 GPIO28=SCIA RX、GPIO29=SCIA TX，SCIA 归属仍在 CPU1 启动时移交 CPU2。

CPU1 当前 SysConfig 生成的时钟路径保持：CPU1/CPU2 200 MHz、CM 125 MHz、EtherCAT domain 100 MHz；CM 的 ECAT 外设归属仍由 CPU1 的 `SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_ETHERCAT, 1U)` 完成。

## 7. 编译与输出文件

已使用 CCS 的实际 `gmake` 工具链执行 Clean + Build；三项均为 0 error：

- `cpu1/CPU1_FLASH/Driverlib Empty Dual Example CCS Project.out`
- `cpu2/CPU2_FLASH/empty_c28x_dual_driverlib_project_cpu2.out`
- `CM/CM_FLASH/Driverlib Empty CM Example CCS Project.out`

CM map 文件已确认包含：`ethercat_slave_cm_hal.obj`、`ESC_initHW`、`IPC_CM_1_ISR` 与 `g_ecat_motor_telemetry_txpdo`。它尚不可能包含 SSC stack object，因为本机不存在 SSC 生成源码。

## 8. 烧录与硬件验证顺序

1. 烧录 CM Flash 输出；
2. 烧录 CPU2 Flash 输出；
3. 烧录 CPU1 Flash 输出；CPU1 会配置 ECAT 后 boot CM，并分别与 CM、CPU2 在 FLAG31 同步。
4. 复位整板。三核都必须存在正确固件；如果 CM 没有烧录或未启动，CPU1 会在 CM 启动同步处等待，这是故意避免向未初始化 CM 队列发送的保护机制。

在 CCS 观察：

- CPU1 的 `g_cm_ipc_tx_count` 应持续增长，`g_cm_ipc_tx_drop_count` 应为 0 或极低；
- CM 的 `g_cm_ipc_rx_count` 应持续增长，`g_cm_last_encoder_count`/`g_cm_last_current_speed` 应与 CPU2 串口输出一致；
- `g_ecat_hw_init_status == ESC_HW_INIT_SUCCESS` 表示 HAL 通过 ESCSS/EEPROM 初始化；
- `g_ecat_pdo_update_count` 应随 IPC 新遥测增长。

## 9. TwinCAT 验收（等待 SSC/ESI 接入后）

1. 在 TwinCAT 的 I/O 配置中选择连接 F28388D EtherCAT PHY 的物理网卡；这个网卡会被 TwinCAT 独占，不能选普通 Wi-Fi 或未接线网卡。
2. 在 EtherCAT Master 上执行 **Scan Devices**，接受扫描结果并导入/使用生成的 ESI。
3. 检查从站状态依次为 `INIT → PRE-OP → SAFE-OP → OP`。
4. 在从站的 Process Data / Online 变量中查看 `Motor Telemetry.EncoderCount` 和 `Motor Telemetry.CurrentSpeed`。
5. 手动转动编码器或运行电机，对照同一时刻串口的 `enc_count`、`current_speed`；两者应来自相同 CPU1 快照。

若 Scan Devices 找不到从站，按此顺序排查：PHY Link、ECAT 100 MHz 时钟、ECAT GPIO、CPU1 Peripheral Allocation、CM 是否启动、`ESC_initHW()` 状态、SSC 主循环、ECAT/Sync 中断、ESI、PDO 映射。

## 10. 软件验证边界

已验证的是三个工程的真实工具链编译、链接、官方 HAL 已进入 CM 输出、以及源码级 IPC/PDO 数据流。尚未连接 F28388D 和 TwinCAT 进行硬件验证，也未生成 SSC/ESI，所以不应把当前状态描述为“EtherCAT 已硬件验证通过”。
