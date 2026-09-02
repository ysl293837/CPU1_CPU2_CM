# EtherCAT 电机状态与命令通道

## 已接入的软件通道

CPU2 的串口功能保持原样。EtherCAT 使用另一条独立通道，最终结构如下：

```text
CPU1 电机控制
  ├─ CPU1 → CPU2 IPC → SCIA → 串口助手
  └─ CPU1 → CM IPC → EtherCAT TxPDO → TwinCAT

TwinCAT RxPDO → CM EtherCAT application → CM → CPU1 IPC → 电机控制
```

CPU1 与 CM 使用 `IPC_CPU1_L_CM_R` / `IPC_CM_L_CPU1_R` 的 DriverLib Message Queue。CPU1 到 CM 和 CPU1 到 CPU2 是不同的 IPC 实例；CM 命令接收中断为 CPU1 的 `INT_CMTOCPUXIPC1`（PIE Group 11），不会使用 SCIA 或修改 CPU2 的串口路径。

## 上报数据

CPU1 每约 10 ms 向 CM 发送两类非阻塞消息：

| IPC 命令 | 内容 | CM 中的 TxPDO 数据源 |
|---|---|---|
| `IPC_CMD_MOTOR_TELEMETRY` (`0x1001`) | `dataw1=current_count`，`dataw2=current_speed` 的 IEEE-754 位模式 | `g_ecat_motor_telemetry_txpdo`，计划映射 `0x6000:01 EncoderCount` 和 `0x6000:02 CurrentSpeed` |
| `IPC_CMD_MOTOR_STATUS` (`0x1002`) | `dataw1=运行/急停状态位`，`dataw2=目标速度` 的 IEEE-754 位模式 | `g_ecat_motor_status_txpdo`，计划映射 `0x6001` |

状态位：

- bit0：`IPC_MOTOR_STATUS_RUN_ENABLED`，闭环运行使能。
- bit1：`IPC_MOTOR_STATUS_ESTOP_ACTIVE`，急停有效。

`g_ecat_motor_status_txpdo` 还包含最近命令、命令执行结果、命令发送次数和丢弃次数，便于 TwinCAT 在线诊断。

## 下行命令接口

CM 已提供以下可由 SSC 的 RxPDO 回调直接调用的接口：

```c
bool ECAT_MotorCommand_Request(uint32_t command,
                               uint32_t dataw1,
                               uint32_t dataw2);

bool ECAT_MotorCommand_RequestFloat(uint32_t command, float value);

bool ECAT_MotorCommand_ApplyRxPDO(
    const volatile ECAT_MotorCommandRxPDO_t *rxpdo);
```

建议的 `0x7000 Motor Command` RxPDO 是 16 字节：

| SubIndex | 字段 | 类型 | 用途 |
|---:|---|---|---|
| `0x7000:01` | `command` | UDINT | IPC 命令号 |
| `0x7000:02` | `dataw1` | UDINT | 浮点命令参数时放 IEEE-754 的 32 位数据 |
| `0x7000:03` | `dataw2` | UDINT | 第二参数，当前命令通常为 0 |
| `0x7000:04` | `sequence` | UDINT | 每发一条新命令递增，避免周期 PDO 重复执行 |

支持的命令号与现有串口命令完全复用：

| EtherCAT `command` | 串口等价命令 | CPU1 行为 |
|---:|---|---|
| `0x1101` | `start` | 启动闭环 |
| `0x1102` | `stop` | 停止输出并将目标速度清零 |
| `0x1103` | `estop` | 急停 |
| `0x1104` | `clear_estop` | 清除急停，仍保持停止 |
| `0x1105` | `reset_pid` | 复位速度 PI 内部状态 |
| `0x1106` | `target=x` | `dataw1` 为目标速度 `float` 位模式 |
| `0x1107` | `kp=x` | `dataw1` 为 Kp `float` 位模式 |
| `0x1108` | `ki=x` | `dataw1` 为 Ki `float` 位模式 |
| `0x1109` | `kd=x` | `dataw1` 为 Kd `float` 位模式 |

CM 发送命令后会把 `lastCommandResult` 设为 `IPC_RESULT_PENDING`。CPU1 实际完成控制状态修改后，以 `IPC_CMD_MOTOR_ACK` 回应；CM 再更新 `g_ecat_motor_status_txpdo.lastCommandResult`：0 为成功，1 表示急停状态下拒绝启动或改速。

## SSC 接入点

当前工程还没有 Beckhoff SSC 生成的 `ecatappl.c`、`ecatslv.c`、`objdef.c` 等文件，因此还不能让 TwinCAT 实际进入 OP。生成 SSC 后，在该 SSC 生成的输出 PDO 映射/应用回调中：

1. 将 `0x7000` 的数据复制到 `g_ecat_motor_command_rxpdo`；
2. 调用 `ECAT_MotorCommand_ApplyRxPDO(&g_ecat_motor_command_rxpdo)`；
3. 将 `g_ecat_motor_telemetry_txpdo` 映射为 `0x6000`；
4. 将 `g_ecat_motor_status_txpdo` 映射为 `0x6001`。

这四步必须使用 SSC 实际生成的函数和对象名完成，不能凭空编写伪 EtherCAT 协议栈。

## CCS 诊断变量

- CPU1：`g_cm_ipc_tx_count`、`g_cm_ipc_tx_drop_count`、`g_cm_ipc_cmd_rx_count`、`g_cm_ipc_cmd_error_count`、`g_cm_ipc_cmd_ack_tx_count`、`g_cm_ipc_cmd_ack_drop_count`
- CM：`g_cm_ipc_rx_count`、`g_cm_ipc_status_rx_count`、`g_cm_ipc_cmd_tx_count`、`g_cm_ipc_cmd_drop_count`、`g_cm_ipc_cmd_ack_count`、`g_cm_last_command`、`g_cm_last_command_result`、`g_ecat_motor_telemetry_txpdo`、`g_ecat_motor_status_txpdo`
