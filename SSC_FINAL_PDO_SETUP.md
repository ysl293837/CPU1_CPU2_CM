# F28388D 三核工程：SSC、EtherCAT 与电机数据链路说明

## 0. 当前完成状态

当前工程已经完成以下软件集成：

- CPU1 继续负责 PMSM/FOC、电流/速度控制、eQEP、目标速度、PID，以及 CPU1↔CPU2、CPU1↔CM IPC。
- CPU2 继续负责 SCIA 串口、上位机命令解析，以及把 CPU1 的编码器和速度数据发送到串口助手。
- CM 使用 TI 官方 ethercat_slave_cm_hal.c，并实际编译 Beckhoff SSC 生成的协议栈源码。
- CM 的 main() 已经执行 ESC_initHW()、MainInit() 和 MainLoop()。
- CPU1 的 ECAT 时钟、ECAT 资源保留和开发板 EtherCAT 引脚仍由 CPU1 负责。
- CM 不编译 C28x 风格的 f2838x_pinmux.c、pinmux.c。
- 当前 SSC 对象字典仍是 SSC EchoBack 默认字典；本文件后面给出下一次 SSC Tool 重新生成时应使用的最终电机 PDO。
- 当前没有进行真实开发板或 TwinCAT 在线测试，因此硬件通信结果仍需要上板验证。

## 1. 工程文件修改情况

### 1.1 CM 主程序

文件：CM/empty_driverlib_main_cm.c

主要修改：

1. 引入 TI EtherCAT HAL、SSC 应用接口和电机 EtherCAT 影子数据定义。
2. 增加 CM↔CPU1 IPC 队列和 IPC1 接收中断。
3. 在 IPC_CM_SyncWithCPU1() 中使用 IPC_FLAG31 完成 CM 与 CPU1 的启动会合。
4. 接收 CPU1 发来的两类遥测帧：
   - IPC_CMD_MOTOR_TELEMETRY：编码器计数和当前速度。
   - IPC_CMD_MOTOR_STATUS：运行/急停状态和目标速度。
5. 处理 EtherCAT RxPDO 命令，并通过 CM→CPU1 IPC 转发给 CPU1。
6. 增加 ECAT_MotorApplication_Process()，由 SSC 的 APPL_Application() 周期调用。
7. 启动顺序为：

    CM_init()
        -> IPC_CM_Init()
        -> IPC_CM_SyncWithCPU1()
        -> CPU_clearPRIMASK()
        -> ESC_initHW()
        -> MainInit()
        -> bRunApplication = TRUE
        -> MainLoop()

### 1.2 CM 电机应用接口

文件：CM/ecat_motor_app.h

增加：

- ECAT_MotorTelemetryTxPDO_t：保存 CPU1 的 encoderCount 和 currentSpeed。
- ECAT_MotorCommandRxPDO_t：保存 EtherCAT 下发的 command/dataw1/dataw2/sequence。
- ECAT_MotorStatusTxPDO_t：保存电机状态、目标速度、最近命令、命令结果和统计计数。
- ECAT_MotorCommand_Request()：向 CPU1 提交电机命令。
- ECAT_MotorCommand_ApplyRxPDO()：过滤重复 RxPDO 并提交新命令。
- ECAT_MotorApplication_Process()：刷新 TxPDO 影子数据并处理 RxPDO。

### 1.3 SSC 应用层

文件：CM/ssc_stack/ecat_motor_appl.c

该文件由桌面目录中的 SSC EchoBack 应用文件复制到工程后形成。SSC 核心回调保持官方结构，包括：

- PDO_ResetOutputs()
- APPL_StartMailboxHandler()
- APPL_StopMailboxHandler()
- APPL_StartInputHandler()
- APPL_StopInputHandler()
- APPL_StartOutputHandler()
- APPL_StopOutputHandler()
- APPL_GenerateMapping()
- APPL_InputMapping()
- APPL_OutputMapping()
- APPL_Application()
- APPL_AckErrorInd()

当前阶段在 EchoBack 的 RxPDO 字段上做临时桥接：

- 0x7010 DatafromMaster 临时承载 IPC 命令号。
- 0x7014 TargetSpeedPosReq 临时承载 dataw1。
- 0x7012 TargetMode 临时承载 dataw2。
- 每次字段变化时递增 sequence，防止同一命令在每个 MainLoop 周期重复发送。

离开 OP 或输出复位时，临时缓存和命令影子区都清零；重新进入 OP 后，即使命令值与上一次相同，也可以再次触发。

原始的 CM/ssc_stack/f2838x_cm_echoback.c 保留作生成参考，但没有参与编译，因为它包含 SSC 自带的默认 main()；实际编译的是 ecat_motor_appl.c，CM 自己的 main() 负责统一启动流程。

### 1.4 CM 构建规则

文件：CM/makefile.defs

该文件是持久化的构建扩展，避免直接修改 CCS 自动生成的 CM_FLASH/makefile。它完成：

- 加入 TI 官方 EtherCAT CM HAL 源文件。
- 加入 SSC 核心和硬件适配源码。
- 加入 CM/ssc_stack 头文件搜索路径。
- 加入 C2000Ware EtherCAT include 路径。
- 加入 ETHERCAT_STACK。
- 加入 USE_DEFAULT_MAIN=0，关闭 SSC EchoBack 自带 main()。
- 为 ssc_stack/%.c 增加带目录的 ARM 编译规则。
- 将所有 SSC/HAL .obj 放入最终 CM 链接命令。

这样在 CCS 重新生成部分 makefile 后，手动接入的 SSC/HAL 仍会被加入构建。

### 1.5 CM 工程配置

文件：CM/.cproject

CM_RAM 和 CM_FLASH 两个配置都增加了：

- PROJECT_ROOT/ssc_stack
- C2000Ware EtherCAT include 路径
- ETHERCAT_STACK
- USE_DEFAULT_MAIN=0

CM 工程没有新增 c2000_cm.syscfg。CPU1 的 ECAT 配置继续保留在 cpu1/c2000_cpu1.syscfg；CM 采用 TI 官方 CM 工程方式，不依赖 C28x 风格 SysConfig 生成文件。

### 1.6 CM 链接命令文件

文件：

- CM/2838x_FLASH_lnk_cm.cmd
- CM/2838x_RAM_lnk_cm.cmd

两个文件均加入了官方 EtherCAT 工程需要的区域：

- CPU1TOCMMSGRAM0_ECAT
- CMTOCPU1MSGRAM0_ECAT
- MSGRAM_CPU1_TO_CM_ECAT
- MSGRAM_CM_TO_CPU1_ECAT

同时保留普通 IPC 区域，普通 CPU1↔CM IPC 与 EtherCAT 专用共享区不重叠。

CM/2838x_FLASH_lnk_ecat_cm.cmd 是工程内保留的官方布局参考副本。当前 CM_FLASH 实际链接的是 CM/2838x_FLASH_lnk_cm.cmd，不能同时把两个 Flash cmd 文件加入链接。

## 2. 新增和复制的文件

### 2.1 SSC 文件

SSC 生成文件已经从 C:\Users\yangshilin\Desktop\Src 复制到工程内的 CM/ssc_stack/，因此构建不再依赖桌面目录作为永久源码路径。

复制的主要文件：

    CM/ssc_stack/applInterface.h
    CM/ssc_stack/coeappl.c
    CM/ssc_stack/coeappl.h
    CM/ssc_stack/ecat_def.h
    CM/ssc_stack/ecatappl.c
    CM/ssc_stack/ecatappl.h
    CM/ssc_stack/ecatcoe.c
    CM/ssc_stack/ecatcoe.h
    CM/ssc_stack/ecatslv.c
    CM/ssc_stack/ecatslv.h
    CM/ssc_stack/esc.h
    CM/ssc_stack/f2838x_cm_echoback.c
    CM/ssc_stack/f2838x_cm_echoback.h
    CM/ssc_stack/f2838x_cm_echobackobjects.h
    CM/ssc_stack/f2838x_cm_hw.c
    CM/ssc_stack/f2838x_cm_hw.h
    CM/ssc_stack/f2838x_cm_system.c
    CM/ssc_stack/f2838x_cm_system.h
    CM/ssc_stack/mailbox.c
    CM/ssc_stack/mailbox.h
    CM/ssc_stack/objdef.c
    CM/ssc_stack/objdef.h
    CM/ssc_stack/sdoserv.c
    CM/ssc_stack/sdoserv.h
    CM/ssc_stack/ecat_motor_appl.c

### 2.2 实际参与编译的 SSC/HAL 文件

实际加入 CM 链接的对象：

    coeappl.obj
    ecatappl.obj
    ecatcoe.obj
    ecatslv.obj
    ecat_motor_appl.obj
    f2838x_cm_hw.obj
    f2838x_cm_system.obj
    mailbox.obj
    objdef.obj
    sdoserv.obj
    ethercat_slave_cm_hal.obj

未参与编译的源码：

- f2838x_cm_echoback.c：包含默认 main()，会和当前 CM 主程序重复。
- f2838x_cm_echobackobjects.h：作为对象字典头文件被多个源码引用，必须保留。
- SSC 目录中的 .h 文件：通过头文件依赖参与编译，不单独生成目标文件。

## 3. CPU1 的 ECAT 配置和初始化位置

CPU1 的 SysConfig 文件：cpu1/c2000_cpu1.syscfg。

其中保留：

- ECAT -> MyECAT1。
- 外部 25 MHz 晶振作为 AUXPLL 输入。
- AUXPLL 原始时钟 500 MHz。
- CMCLK 使用 AUXPLL/4 = 125 MHz。
- ECAT 使用 AUXPLL/5 = 100 MHz。
- PHY clock enable。

编译后，这些时钟设置生成在：

    cpu1/CPU1_FLASH/syscfg/device.c
    cpu1/CPU1_FLASH/syscfg/device.h
    cpu1/CPU1_FLASH/syscfg/clocktree.h

生成代码中可以看到：

    SysCtl_setAuxClock(...);
    SysCtl_setCMClk(SYSCTL_CMCLKOUT_DIV_4, SYSCTL_SOURCE_AUXPLL);
    SysCtl_setECatClk(SYSCTL_ECATCLKOUT_DIV_5,
                      SYSCTL_SOURCE_AUXPLL,
                      0x1U);

CPU1 的实际 EtherCAT 引脚初始化仍在：

    cpu1/APP/ecat/ecat_gpio.c

CPU1 主函数中的顺序是：

    Board_init();
    ECAT_GPIO_init();
    SysCtl_allocateSharedPeripheral(SYSCTL_PALLOCATE_ETHERCAT, 1U);

CPU1 先配置 ECAT 的 GPIO、PHY、MDIO、I2C、SYNC 和数据线，然后把 ECAT 外设控制权分配给 CM。CM 不重复配置这些 GPIO。

## 4. CM 的 IPC 初始化位置

CM↔CPU1 IPC 初始化在：

    CM/empty_driverlib_main_cm.c

主要函数：

    static void IPC_CM_Init(void);
    static void IPC_CM_SyncWithCPU1(void);
    static void IPC_CM_1_ISR(void);

IPC_CM_Init() 中：

- 使用 IPC_CM_L_CPU1_R 作为 CM↔CPU1 IPC 实例。
- CM 本地接收使用 IPC_INT1。
- CPU1 远端接收也使用 IPC1。
- CM 注册 IPC_CM_1_ISR 作为 CPU1→CM 接收中断。

IPC_CM_SyncWithCPU1() 中使用：

    IPC_sync(IPC_CM_L_CPU1_R, IPC_FLAG31);

该 FLAG31 是启动同步点，不是每一帧数据的通知标志。数据队列使用 IPC 消息队列和消息触发中断。

## 5. CPU1→CM 的遥测数据

CPU1 的发送位置：

    cpu1/APP/ipc/ipc_cpu1.c

CPU1 在 ePWM ISR 中只累计遥测节拍，在主循环中调用：

    IPC_CPU1_PollSend(current_count, current_speed);

它把相同的数据分别发送给 CPU2 和 CM：

    CPU1 current_count/current_speed
           ├── IPC → CPU2 → SCIA → 串口助手
           └── IPC → CM   → EtherCAT TxPDO → TwinCAT

CPU1→CM 的消息定义：

| 字段 | 内容 |
|---|---|
| command | IPC_CMD_MOTOR_TELEMETRY |
| dataw1 | current_count，编码器计数，按 uint32_t 传输 |
| dataw2 | current_speed 的 IEEE-754 32 位原始位模式 |
| address | 未使用 |
| source | CPU1 |

CM 的 IPC ISR 使用奇偶序号保护快照：写入前序号为奇数，写入完成后为偶数；CM 主循环读取前后比较序号，避免读到一半更新的数据。

## 6. CM→CPU1 的 IPC Message 格式

CM 通过当前工程已经存在的命令接口发送：

    EtherCAT RxPDO
       → g_ecat_motor_command_rxpdo
       → ECAT_MotorCommand_ApplyRxPDO()
       → ECAT_MotorCommand_Request()
       → CM→CPU1 IPC queue
       → CPU1 IPC_CPU1_CM_1_ISR()
       → CPU1 执行原有电机命令

命令号定义在 ipc_protocol.h：

| 命令 | 值 | 含义 |
|---|---:|---|
| IPC_CMD_MOTOR_START | 0x1101 | 启动电机闭环 |
| IPC_CMD_MOTOR_STOP | 0x1102 | 停止电机输出 |
| IPC_CMD_MOTOR_ESTOP | 0x1103 | 急停 |
| IPC_CMD_MOTOR_CLEAR_ESTOP | 0x1104 | 清除急停 |
| IPC_CMD_MOTOR_RESET_PID | 0x1105 | 复位速度 PI |
| IPC_CMD_MOTOR_SET_TARGET | 0x1106 | 设置目标速度 |
| IPC_CMD_MOTOR_SET_KP | 0x1107 | 设置 Kp |
| IPC_CMD_MOTOR_SET_KI | 0x1108 | 设置 Ki |
| IPC_CMD_MOTOR_SET_KD | 0x1109 | 设置 Kd |

IPC 消息格式：

| 字段 | 含义 |
|---|---|
| command | 上表中的命令号 |
| dataw1 | 第一个参数；浮点参数使用 IEEE-754 原始位模式 |
| dataw2 | 第二个参数或保留字段 |
| address | 当前未使用 |
| source | CM |

CPU1 执行完成后返回 IPC_CMD_MOTOR_ACK：

- dataw1：原始命令号。
- dataw2：执行结果。

执行结果包括 IPC_RESULT_OK、IPC_RESULT_ESTOP_ACTIVE、IPC_RESULT_BAD_COMMAND 和 IPC_RESULT_BAD_VALUE。

## 7. CPU1 端和 CM 端接收中断

### 7.1 CPU1 接收 CPU2

    __interrupt void IPC_1_ISR(void)

该函数处理 CPU2→CPU1 的串口命令和 CPU1 返回 CPU2 的应答。

### 7.2 CPU1 接收 CM

    __interrupt void IPC_CPU1_CM_1_ISR(void)

该函数处理 CM→CPU1 的 EtherCAT 命令，并调用原有的 IPC_CPU1_ExecuteCommand()，所以 EtherCAT 不会复制第二套电机控制逻辑。

### 7.3 CM 接收 CPU1

    static void IPC_CM_1_ISR(void)

处理 CPU1 的遥测、状态和命令应答：

1. 循环读取 IPC 消息队列。
2. 判断 message.command。
3. 将编码器计数和速度写入 CM 快照区。
4. 将状态和目标速度写入 CM 状态快照区。
5. 收到 IPC_CMD_MOTOR_ACK 时更新最后命令和执行结果。
6. 使用 IPC_ackFlagRtoL(..., IPC_FLAG1) 应答消息队列对应的通知标志。

## 8. SSC 主循环中的 PDO 处理流程

CM 的主程序调用 SSC 入口：

    (void)MainInit();
    bRunApplication = TRUE;
    while(bRunApplication == TRUE)
    {
        MainLoop();
    }

MainLoop() 会驱动 SSC 的状态机、邮箱、CoE/SDO、PDO 和应用层回调。

应用层流程：

    EtherCAT RxPDO
           ↓
    APPL_OutputMapping()
           ↓
    g_ecat_motor_command_rxpdo
           ↓
    APPL_Application()
           ↓
    ECAT_MotorCommand_ApplyRxPDO()
           ↓
    CM→CPU1 IPC

反向数据流：

    CPU1→CM IPC ISR
           ↓
    CM 遥测/状态快照区
           ↓
    APPL_Application()
           ↓
    TxPDO 影子对象
           ↓
    APPL_InputMapping()
           ↓
    EtherCAT ESC DPRAM
           ↓
    TwinCAT

## 9. 当前 EchoBack 对象字典的临时映射

当前 SSC 生成文件仍是 EchoBack 默认对象字典。

### TxPDO：从站发送给主站

| 索引 | 当前对象 | 当前承载数据 |
|---|---|---|
| 0x6000 | Switches | 状态位：运行、急停、ESC 初始化、OP、IPC 丢弃、IPC 错误 |
| 0x6010 | DataToMaster | CPU1 current_count |
| 0x6012 | TargetModeResponse | 电机状态低 16 位 |
| 0x6014 | TargetSpeedPosFeedback | CPU1 current_speed 的 IEEE-754 原始 32 位 |

### RxPDO：主站发送给从站

| 索引 | 当前对象 | 当前承载数据 |
|---|---|---|
| 0x7000 | LEDs | 仍保留 EchoBack 的 LED 字段 |
| 0x7010 | DatafromMaster | 临时承载 IPC 命令号 |
| 0x7012 | TargetMode | 临时承载 dataw2 |
| 0x7014 | TargetSpeedPosReq | 临时承载 dataw1 |

该映射用于第一阶段验证 SSC、HAL、ESC、PDO、IPC 和电机控制链路，不代表最终工程的正式对象字典。

## 10. 下一次 SSC Tool 应生成的最终 PDO

使用 SSC Tool V5.12 重新生成时，建议建立以下正式对象。

### 10.1 最终 TxPDO：0x6000 MotorTelemetry

| 子索引 | 名称 | 类型 | 位宽 | 方向 | 说明 |
|---:|---|---|---:|---|---|
| 01 | encoderCount | INT32 / DINT | 32 | 从站→主站 | CPU1 current_count |
| 02 | currentSpeed | REAL32 | 32 | 从站→主站 | CPU1 current_speed，单位 r/s |

### 10.2 最终 TxPDO：0x6001 MotorStatus

| 子索引 | 名称 | 类型 | 位宽 | 方向 | 说明 |
|---:|---|---|---:|---|---|
| 01 | motorStatus | UINT32 | 32 | 从站→主站 | bit0 运行，bit1 急停 |
| 02 | targetSpeed | REAL32 | 32 | 从站→主站 | 当前目标速度 |
| 03 | lastCommand | UINT32 | 32 | 从站→主站 | 最近执行命令号 |
| 04 | lastCommandResult | UINT32 | 32 | 从站→主站 | 最近执行结果 |
| 05 | commandTxCount | UINT32 | 32 | 从站→主站 | CM→CPU1 成功提交次数 |
| 06 | commandDropCount | UINT32 | 32 | 从站→主站 | IPC 队列忙丢弃次数 |

### 10.3 最终 RxPDO：0x7000 MotorCommand

| 子索引 | 名称 | 类型 | 位宽 | 方向 | 说明 |
|---:|---|---|---:|---|---|
| 01 | command | UINT32 | 32 | 主站→从站 | 使用 ipc_protocol.h 命令号 |
| 02 | dataw1 | UINT32 | 32 | 主站→从站 | 命令参数1 |
| 03 | dataw2 | UINT32 | 32 | 主站→从站 | 命令参数2 |
| 04 | sequence | UINT32 | 32 | 主站→从站 | 每次新命令递增，防止重复执行 |

### 10.4 PDO 映射对象

建议配置：

| 索引 | 内容 |
|---|---|
| 0x1600 | 0x7000:01、:02、:03、:04 |
| 0x1A00 | 0x6000:01、:02 |
| 0x1A01 | 0x6001:01 至 :06 |
| 0x1C12 | 指向 0x1600 |
| 0x1C13 | 指向 0x1A00、0x1A01 |

正式生成后，TwinCAT 端必须使用 SSC 新生成的 ESI XML，不能继续用旧 EchoBack ESI 解释新对象。

## 11. SSC Tool 最终重新生成步骤

1. 打开 SSC Tool V5.12。
2. 导入 TI 提供的 f2838x_ssc_config.xml。
3. 选择 TI F2838x CM Sample。
4. 在对象字典中建立本文件第 10 节的对象和子索引。
5. 设置 0x1600、0x1A00、0x1A01、0x1C12、0x1C13。
6. 执行 Project -> Create new Slave Files。
7. 将新的 .c/.h 文件复制到工程 CM/ssc_stack/，覆盖同名生成层文件。
8. 保留 ecat_motor_appl.c 中的电机桥接逻辑；如果 SSC Tool 重新生成了同名应用文件，应把桥接逻辑重新合并到新的应用回调中。
9. 用新生成的 ESI XML 替换 TwinCAT 工程中的旧 ESI。
10. 重新构建 CM_FLASH，再构建 CPU1_FLASH 和 CPU2_FLASH。

不要手工修改 SSC 核心协议文件来伪造正式对象字典；正式对象应由 SSC Tool 和 ESI 同步生成。

## 12. 构建验证结果

已执行 CM_FLASH 强制全量构建：

    gmake -B -k -j 24 all -r -O

结果：退出码 0，CM 输出文件为：

    CM/CM_FLASH/Driverlib Empty CM Example CCS Project.out

CM map 中已确认存在：

- ecatslv.obj
- ecatappl.obj
- ecatcoe.obj
- coeappl.obj
- sdoserv.obj
- objdef.obj
- mailbox.obj
- f2838x_cm_hw.obj
- f2838x_cm_system.obj
- ecat_motor_appl.obj
- ethercat_slave_cm_hal.obj
- MainInit
- MainLoop
- PDI_Isr
- Sync0_Isr
- Sync1_Isr
- ESC_applicationLayerHandler

CPU1_FLASH 和 CPU2_FLASH 也已重新构建并通过：

    cpu1/CPU1_FLASH/Driverlib Empty Dual Example CCS Project.out
    cpu2/CPU2_FLASH/empty_c28x_dual_driverlib_project_cpu2.out

当前 CPU1 SysConfig 生成代码已经确认：

- AUXPLL 原始时钟为 500 MHz。
- CMCLK 使用 AUXPLL/4。
- ECAT 时钟使用 AUXPLL/5。
- ECAT 时钟输出为 100 MHz。
- CPU2 SCIA 仍使用 GPIO28/GPIO29。
- 当前 ECAT 引脚表没有占用 GPIO28/GPIO29。

## 13. 之前遇到的问题及修复

### 13.1 SysConfig 找不到 C2000Ware 产品

原因是 SysConfig CLI 使用的产品版本或 SDK 路径不匹配。

当前 CPU1/CPU2 SysConfig 使用：

    C:/ti/c2000/C2000Ware_26_00_00_00/.metadata/sdk.json

已重新生成并验证 CPU1、CPU2 的 SysConfig 输出。

### 13.2 Board_init、Device_init 等符号未定义

原因是生成的 device.c、board.c 或 f2838x_codestartbranch.obj 没有加入正确的 CPU1/CPU2 构建，或者混用了错误架构的生成文件。

当前 CPU1/CPU2 使用对应 C28x 工程生成文件，CM 使用 ARM device/cm.c 和 startup_cm.c。

### 13.3 CM 只链接 HAL，没有链接 SSC

原因是 SSC 文件存在于磁盘中，但没有进入 CM 的 OBJS 和 ORDERED_OBJS。

现在由 CM/makefile.defs 明确加入 SSC 对象，并为带目录源码增加专用规则。

### 13.4 PDI_Isr、Sync0_Isr 没有进入最终代码

原因是 TI HAL 中的完整中断实现受 ETHERCAT_STACK 条件编译控制。

现在 CM 编译命令包含：

    --define=ETHERCAT_STACK

map 中已经能看到 PDI_Isr、Sync0_Isr 和 Sync1_Isr。

### 13.5 两个 main() 冲突风险

SSC EchoBack 生成文件中带有默认 main()，当前 CM 工程也有自己的 main()。

现在 CM 编译命令包含：

    --define=USE_DEFAULT_MAIN=0

并且实际不编译 f2838x_cm_echoback.c，只编译合并后的 ecat_motor_appl.c。

### 13.6 CM 普通链接文件与 EtherCAT 链接文件混用

EtherCAT 官方链接文件需要独立的 ECAT IPC 共享区和官方 RAM/Flash 区域布局。当前已经把相同布局同步到：

    CM/2838x_FLASH_lnk_cm.cmd
    CM/2838x_RAM_lnk_cm.cmd

最终链接时只能选择当前配置对应的一个 cmd 文件，不能把普通 CM cmd 和 EtherCAT CM cmd 同时链接。

## 14. 烧录和上板验证顺序

软件构建通过不等于现场 EtherCAT 已经上线。上板时建议：

1. 先烧录 CPU2_FLASH 输出文件。
2. 再烧录 CM_FLASH 输出文件。
3. 最后烧录 CPU1_FLASH 输出文件。
4. 复位开发板。
5. CPU1 配置 ECAT GPIO、ECAT 时钟和 ECAT 外设归属。
6. CPU1 启动 CM，CM 在 IPC_FLAG31 与 CPU1 会合。
7. CPU1 启动 CPU2，CPU2 在自己的 IPC_FLAG31 与 CPU1 会合。
8. 检查 CPU2 串口助手是否继续收到 CPU1 的 current_count/current_speed。
9. 将网线连接到 EtherCAT 主站网卡后，在 TwinCAT 中选择对应网卡并扫描设备。
10. 检查从站能否从 INIT 进入 PREOP、SAFEOP 和 OP。
11. 在 TwinCAT 中读取 TxPDO，并写入 RxPDO 命令。

建议在 CCS Expressions 中观察：

    g_ecat_hw_init_status
    g_ecat_al_state
    g_cm_ipc_rx_count
    g_cm_ipc_status_rx_count
    g_cm_ipc_cmd_tx_count
    g_cm_ipc_cmd_ack_count
    g_cm_ipc_cmd_drop_count
    g_ecat_pdo_update_count
    g_ecat_command_apply_count
    g_ecat_motor_telemetry_txpdo
    g_ecat_motor_command_rxpdo
    g_ecat_motor_status_txpdo

如果 g_cm_ipc_rx_count 增加而 g_ecat_pdo_update_count 不增加，先检查 CM 主循环；如果 PDO 计数增加但 TwinCAT 无数据，检查 ECAT PHY、网线、ESC 状态和 ESI/PDO 映射；如果 RxPDO 写入后 g_cm_ipc_cmd_tx_count 不增加，检查命令号和 sequence。
