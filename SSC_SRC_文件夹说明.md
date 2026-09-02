# SSC Tool 生成的 `C:\Users\yangshilin\Desktop\Src` 文件夹说明

## 1. 这个文件夹是怎样生成的

你在 Beckhoff **EtherCAT Slave Stack Code Tool（SSC Tool）V5.12** 中创建了 `TI F2838x CM Sample` 从站工程，并执行了：

```text
Project → Create new Slave Files
```

随后 SSC Tool 按项目配置生成了：

```text
C:\Users\yangshilin\Desktop\Src
```

它包含的是 **EtherCAT 从站协议栈 C 源码、对象字典、PDO 映射接口以及 F28388D CM EchoBack 示例**。

它和 CCS 的 SysConfig 生成文件不同：

- SysConfig：配置时钟、引脚、外设、中断。
- `Src`：实现 EtherCAT 从站协议，例如状态机、Mailbox、CoE、SDO、PDO。

这两个部分需要同时存在，CM 才能成为一个完整的 EtherCAT 从站。

## 2. 同时生成的另外两个文件

| 文件 | 作用 |
| --- | --- |
| `C:\Users\yangshilin\Desktop\F2838x CM EtherCAT Slave.esp` | SSC Tool 工程文件。以后双击它即可重新打开当前从站配置。 |
| `C:\Users\yangshilin\Desktop\F2838x CM EtherCAT Slave.xml` | ESI（EtherCAT Slave Information）文件。导入 TwinCAT 后，主站据此识别设备、对象字典和 PDO。 |

> 每次在 SSC Tool 修改对象字典或 PDO 后，都应再次执行 `Create new Slave Files`，并重新导入或更新 ESI XML。

## 3. `Src` 文件夹总览

该目录当前包含四类文件：

```text
1. SSC 协议栈核心          ecatslv / ecatappl / mailbox
2. CoE 与 SDO 服务          ecatcoe / coeappl / sdoserv / objdef
3. 协议与应用接口定义       ecat_def / esc / applInterface
4. TI F28388D CM 示例适配   f2838x_cm_echoback / f2838x_cm_hw / f2838x_cm_system
```

通常 `.c` 是需要编译的实现文件，`.h` 是对应的类型、宏、函数声明文件。

## 4. SSC 协议栈核心文件

| 文件 | 作用 | 是否建议修改 |
| --- | --- | --- |
| `ecatslv.c` / `ecatslv.h` | EtherCAT 从站状态机（ESM）。处理 `INIT → PREOP → SAFEOP → OP`、错误状态、状态切换、同步管理器等。 | 不建议修改。 |
| `ecatappl.c` / `ecatappl.h` | EtherCAT 应用与协议栈之间的过程数据接口。协议栈在这里调用应用层的 PDO 映射函数。 | 通常不改。 |
| `mailbox.c` / `mailbox.h` | Mailbox 缓冲区和 Mailbox 服务调度。CoE、FoE、EoE 等非实时服务依赖它。 | 不建议修改。 |
| `ecat_def.h` | SSC 栈的公共基础定义、数据类型、功能开关和协议相关宏。 | 不建议手工修改。 |
| `esc.h` | EtherCAT Slave Controller（ESC）寄存器、事件和底层访问相关定义。 | 不建议修改。 |
| `applInterface.h` | 协议栈与具体应用层之间的通用接口声明。 | 一般不改。 |

其中最重要的运行关系是：

```text
EtherCAT 主站
    ↓
ESC 硬件 + CM HAL
    ↓
ecatslv.c                 从站状态机
    ↓
ecatappl.c                过程数据调度
    ↓
APPL_InputMapping() / APPL_OutputMapping() / APPL_Application()
    ↓
你的 CM 电机应用代码
```

## 5. CoE、对象字典和 SDO 文件

| 文件 | 作用 | 是否建议修改 |
| --- | --- | --- |
| `ecatcoe.c` / `ecatcoe.h` | CoE（CANopen over EtherCAT）Mailbox 接口。负责 CoE 请求的协议处理。 | 不建议修改。 |
| `coeappl.c` / `coeappl.h` | CoE 应用支持，包括对象字典初始化与相关服务。 | 一般不改。 |
| `sdoserv.c` / `sdoserv.h` | SDO Server。TwinCAT 可以通过 SDO 读取或写入对象字典中的对象。 | 不建议修改。 |
| `objdef.c` / `objdef.h` | 对象字典查找、读取、写入、访问权限检查等通用函数。 | 不建议修改。 |
| `aoeappl.c` / `aoeappl.h` | AoE（ADS over EtherCAT）支持代码。是否真正使用取决于 SSC 工程是否启用了 AoE。 | 当前电机项目通常不用改。 |

### CoE、SDO、PDO 的区别

| 名称 | 用途 | 特点 |
| --- | --- | --- |
| CoE | 在 EtherCAT 上使用 CANopen 风格对象字典。 | 是协议框架。 |
| SDO | 读取/写入对象字典中的参数，例如设备名称、PID 参数。 | 非周期、配置型通信。 |
| PDO | 周期传输控制命令与实时状态，例如启停、目标速度、实际速度。 | 实时、周期性通信。 |

对你的电机项目，建议的划分是：

```text
RxPDO：主站 → 从站
    电机启动、停止、急停、目标速度、PID 参数、命令序号

TxPDO：从站 → 主站
    编码器计数、当前速度、运行状态、急停状态、命令应答、故障状态

SDO：主站配置从站
    设备参数、默认速度、控制器参数、版本信息等
```

## 6. 本次生成的 F28388D CM EchoBack 示例

| 文件 | 作用 | 与当前电机项目的关系 |
| --- | --- | --- |
| `f2838x_cm_echoback.c` | TI 的 EchoBack 从站示例应用。接收主站数据后再回传，内部实现 `APPL_*` 回调，并且**包含自己的 `main()`**。 | 只能作为参考，不能原样加入当前 CM 工程。 |
| `f2838x_cm_echoback.h` | EchoBack 示例的函数、`APPL_*` 回调声明。 | 可参考函数接口。 |
| `f2838x_cm_echobackobjects.h` | 本次 SSC 配置自动生成的对象字典和 PDO 结构体定义。 | 需要根据电机对象字典重新生成或修改配置后再生成。 |
| `f2838x_cm_hw.c` / `.h` | F28388D CM 与 SSC 栈之间的硬件适配接口。 | 需要结合 C2000Ware 的 EtherCAT CM HAL 使用。 |
| `f2838x_cm_system.c` / `.h` | CM 工具链/系统兼容层，例如字符串函数包装。 | 通常直接保留。 |

### 为什么不能直接编译 `f2838x_cm_echoback.c`

当前工程已经有自己的 CM 入口：

```text
CM/empty_driverlib_main_cm.c
```

而 `f2838x_cm_echoback.c` 也定义了：

```c
int main(void)
```

如果两个文件同时参与编译链接，会出现 `main` 重复定义错误。

正确方式是：

1. 保留当前工程的 `CM/empty_driverlib_main_cm.c` 作为唯一 `main()`。
2. 从 EchoBack 示例中提取或重写 `APPL_*` 回调函数。
3. 在这些回调中连接当前已有的 `ECAT_MotorCommand_*()` 与 CPU1↔CM IPC 逻辑。

## 7. EchoBack 示例中的关键应用回调

`f2838x_cm_echoback.c` 已经提供了 SSC 所需的典型应用函数：

| 函数 | 被谁调用 | 你在电机项目中应该做什么 |
| --- | --- | --- |
| `APPL_StartMailboxHandler()` | 从站进入 PREOP 时。 | 做 Mailbox 相关应用初始化。 |
| `APPL_StartInputHandler()` | 从站允许发送输入 PDO 时。 | 准备 TxPDO 状态。 |
| `APPL_StartOutputHandler()` | 从站允许接收输出 PDO 时。 | 准备接收主站命令。 |
| `APPL_StopOutputHandler()` | 从站离开 OP 或停止输出 PDO 时。 | 可执行安全停止或清零命令。 |
| `APPL_GenerateMapping()` | 协议栈初始化 PDO 映射时。 | 一般使用 SSC 自动生成逻辑。 |
| `APPL_InputMapping()` | 从站向主站发送 TxPDO 前。 | 将 CPU1 遥测/状态复制到 TxPDO。 |
| `APPL_OutputMapping()` | 从站收到主站 RxPDO 后。 | 取出命令、速度等数据，写入 `g_ecat_motor_command_rxpdo`。 |
| `APPL_Application()` | SSC 主循环中周期调用。 | 调用 `ECAT_MotorCommand_ApplyRxPDO()`，将新命令经 IPC 发给 CPU1。 |
| `APPL_AckErrorInd()` | 主站确认从站错误时。 | 可清除应用级故障标志。 |
| `PDO_ResetOutputs()` | 输出 PDO 失效或停止时。 | 将 RxPDO 命令清零，防止旧命令重复执行。 |

## 8. 如何对应到当前 Dual_CPU 工程

当前 CM 工程已经有这些基础代码：

```text
CM/empty_driverlib_main_cm.c
CM/ecat_motor_app.h
CM/APP/ecat/ethercat_slave_cm_hal.c
```

其中已经准备好的数据流是：

```text
EtherCAT RxPDO
    ↓
g_ecat_motor_command_rxpdo
    ↓
ECAT_MotorCommand_ApplyRxPDO()
    ↓
CM → IPC → CPU1
    ↓
CPU1 执行启动 / 停止 / 急停 / 目标速度 / PID 命令

CPU1 遥测与状态
    ↓
CPU1 → IPC → CM
    ↓
g_ecat_motor_telemetry_txpdo
g_ecat_motor_status_txpdo
    ↓
EtherCAT TxPDO
```

因此，SSC 生成代码接入后，核心工作是把：

```text
APPL_OutputMapping() 连接到 g_ecat_motor_command_rxpdo
APPL_InputMapping() 连接到 g_ecat_motor_telemetry_txpdo 和 g_ecat_motor_status_txpdo
APPL_Application()   连接到 ECAT_MotorCommand_ApplyRxPDO()
```

## 9. 接入当前 CM 工程时的文件处理原则

### 9.1 应加入 CCS CM 工程的 SSC 核心文件

通常需要加入以下 SSC 源文件及全部对应头文件：

```text
ecatslv.c
ecatappl.c
mailbox.c
ecatcoe.c
coeappl.c
sdoserv.c
objdef.c
```

是否还需要 `aoeappl.c`，取决于 SSC Tool 中是否启用 AoE。若未启用，该文件常常会通过编译宏被排除相关功能，但保留也通常没有问题；以 SSC 生成的工程配置为准。

### 9.2 不应原样加入当前工程的文件

```text
f2838x_cm_echoback.c
```

原因：它包含独立的 `main()`，并实现的是 EchoBack 示例，不是电机控制逻辑。

正确做法：将其中的 `APPL_*` 回调实现迁移为新的电机 EtherCAT 应用文件，例如：

```text
CM/APP/ecat/ethercat_motor_appl.c
CM/APP/ecat/ethercat_motor_appl.h
```

### 9.3 必须保留的现有硬件层

当前工程已有 C2000Ware 的 CM EtherCAT HAL：

```text
CM/APP/ecat/ethercat_slave_cm_hal.c
CM/APP/ecat/ethercat_slave_cm_hal.h
```

它负责 CM 对 F28388D EtherCAT ESC 硬件的访问。SSC 栈负责协议，HAL 负责硬件，二者不能互相替代。

## 10. 重要注意事项

1. `Src` 中的文件由 SSC Tool 生成，重新生成可能覆盖你手工修改的内容。
2. 最好把“SSC 原始生成文件”和“自己编写的电机应用文件”分开保存。
3. 不要修改 `ecatslv.c`、`mailbox.c`、`sdoserv.c` 等协议栈核心逻辑。
4. 对象字典、PDO 和 ESI 应在 SSC Tool 中修改，然后重新生成。
5. 将 `f2838x_cm_echoback.c` 当作官方示例参考，不要与当前 CM 的 `main()` 同时编译。
6. 在 CCS 中添加 SSC 文件后，需要配置正确的头文件路径、CM 编译器宏以及 EtherCAT HAL 依赖。

## 11. 当前结论

`C:\Users\yangshilin\Desktop\Src` 已经证明：**SSC Tool 已成功为 F28388D CM 生成了 EtherCAT 从站协议栈源码。**

下一步不是重新配置 SysConfig，而是把 SSC 栈核心文件加入 CM 工程，并把 SSC 的 `APPL_*` 回调接到当前的 CPU1↔CM IPC 电机数据通道上。
