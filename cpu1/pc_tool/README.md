# C2000 PMSM Serial Tuner

This tool reads text telemetry from the C2000 serial port and plots any
`key:value` fields it sees. The current firmware sends fields such as:

```text
status:1,run:1,estop:0,stream:1,period_ms:10,enc_count:5269,current_speed:1.231,target_speed:5.000,pid_output:1234.000,i_term:25.000,kp:50.000,ki:5.000,kd:0.000
```

Serial settings used by this project:

- UART: `SCIA`
- Baudrate: `115200`
- Format: `8N1`

## Install

```powershell
python -m pip install -r .\pc_tool\requirements.txt
```

## Run

```powershell
python .\pc_tool\pyqt_motor_tuner.py
```

## Commands

The new `SYSTEM/pc_serial` module accepts simple line commands:

- `start`
- `stop`
- `estop`
- `clear_estop`
- `stream=1` or `stream=0`
- `period=10`
- `target=20`
- `pid=50,5,0`
- `kp=50`
- `ki=5`
- `kd=0`
- `status`
- `reset_pid`
- `help`
