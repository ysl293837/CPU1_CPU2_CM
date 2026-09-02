# F28388D CPU1/CM EtherCAT 工程构建说明

## 当前职责划分

- `cpu1/c2000_cpu1.syscfg`：CPU1 的实际 SysConfig 入口，保留 `ECAT -> MyECAT1` 配置。
- `CM/c2000_cm.syscfg`：仅保留为查看和对照文件，不参与 CM 的构建，也不会触发 SysConfig 代码生成。
- CPU1 负责 EtherCAT 外设时钟、引脚复用以及将 ECAT 外设分配给 CM。
- CM 使用 ARM Cortex-M4 工程方式，编译 `driverlib_cm.lib`、`startup_cm.c` 和 `device/cm.c`。

## CM 构建方式

CM 的 `CM_FLASH` 配置不再包含 C28x SysConfig 生成的 `f2838x_pinmux.c`、`pinmux.c`，也不再配置 SysConfig 编译步骤。实际构建输入为：

```text
CM/empty_driverlib_main_cm.c
CM/startup_cm.c
CM/device/cm.c
CM/2838x_FLASH_lnk_cm.cmd
C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f2838x/driverlib_cm/ccs/Debug/driverlib_cm.lib
```

官方 EtherCAT CM 工程模板和安装程序副本放在 `CM/official_ti_ecat_reference`，用于后续导入 EtherCAT SSC/HAL 应用。

## 已验证

- CPU1 `CPU1_FLASH`：SysConfig 生成成功，链接成功，生成 `cpu1/CPU1_FLASH/Driverlib Empty Dual Example CCS Project.out`。
- CM `CM_FLASH`：编译成功，生成 `CM/CM_FLASH/Driverlib Empty CM Example CCS Project.out`。
- CM 当前构建规则中没有 `SYSCFG_SRCS`、SysConfig CLI、`f2838x_pinmux.obj` 或 `pinmux.obj`。

## 注意

当前 CM 工程已经切换为官方 CM 的构建框架，但 `empty_driverlib_main_cm.c` 仍是空应用。要运行完整 EtherCAT 从站，还需要把官方 EtherCAT SSC 生成的协议栈应用文件和 `ethercat_slave_cm_hal.c` 按官方 `projectspec` 方式加入 CM 工程。

另外，CPU1 当前 ECAT 引脚配置占用了 GPIO29、GPIO34、GPIO35 等引脚；如果 CPU2 的 SCIA 仍配置在这些引脚上，会发生实际硬件引脚复用冲突。串口功能继续使用时，需要把 CPU2 SCIA 改到未被 ECAT 占用的引脚，并同步修改开发板接线。
