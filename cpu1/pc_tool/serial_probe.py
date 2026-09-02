from __future__ import annotations

import argparse
import time

import serial


def main() -> int:
    parser = argparse.ArgumentParser(description="Read raw C2000 SCIA telemetry.")
    parser.add_argument("--port", default="COM4")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=5.0)
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.05, write_timeout=0.2) as ser:
        print("OPEN {} @ {}".format(args.port, args.baud))
        print("Send: stream=1, period=10, status")
        ser.write(b"stream=1\r\n")
        ser.write(b"period=10\r\n")
        ser.write(b"status\r\n")

        deadline = time.time() + args.seconds
        total = 0
        buffer = bytearray()

        while time.time() < deadline:
            data = ser.read(512)
            if not data:
                continue
            total += len(data)
            buffer.extend(data)
            while b"\n" in buffer:
                line, _, buffer = buffer.partition(b"\n")
                print(line.decode("utf-8", errors="replace").rstrip("\r"))

        if buffer:
            print(buffer.decode("utf-8", errors="replace"))

        print("RX_BYTES {}".format(total))
        if total == 0:
            print("NO_BYTES: check SCIA TX->USB-RX, SCIA RX<-USB-TX, GND, firmware download, and 115200 8N1.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
