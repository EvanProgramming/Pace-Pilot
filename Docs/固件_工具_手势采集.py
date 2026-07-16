#!/usr/bin/env python3
"""
BeatSync 手势模板采集工具
通过串口与戒指端通信，交互式录制 5 类手势模板

用法: python gesture_trainer.py --port COM3

手势类型:
  1 = 单击 (播放/暂停)
  2 = 双击 (下一首)
  3 = 上滑 (音量+)
  4 = 下滑 (音量-)
  5 = 旋转 (步频匹配 开/关)
"""

import sys
import argparse
import time

try:
    import serial
except ImportError:
    print("错误: 请先安装 pyserial (pip install pyserial)")
    sys.exit(1)

GESTURE_NAMES = {
    1: "单击 (Single Tap)",
    2: "双击 (Double Tap)",
    3: "上滑 (Swipe Up)",
    4: "下滑 (Swipe Down)",
    5: "旋转 (Rotate)",
}

RECORDS_PER_GESTURE = 5  # 每类录制 5 次

def main():
    parser = argparse.ArgumentParser(description="BeatSync 手势模板采集工具")
    parser.add_argument("--port", required=True, help="串口端口 (如 COM3 / /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="波特率 (默认 115200)")
    args = parser.parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except Exception as e:
        print(f"错误: 无法打开串口 {args.port}: {e}")
        sys.exit(1)

    print(f"已连接 {args.port} @ {args.baud} baud")
    print(f"\n手势模板采集工具")
    print(f"每类手势录制 {RECORDS_PER_GESTURE} 次\n")
    print("请确保戒指端已烧录固件并处于模板录制模式")
    print("录制指令格式: RECORD <gesture_id>\n")

    all_records = []

    for gesture_id in range(1, 6):
        print(f"\n{'='*50}")
        print(f"手势 {gesture_id}: {GESTURE_NAMES[gesture_id]}")
        print(f"{'='*50}")

        for i in range(RECORDS_PER_GESTURE):
            input(f"  第 {i+1}/{RECORDS_PER_GESTURE} 次 - 按 Enter 开始录制...")
            print(f"  录制中... 请在 1.5 秒内完成手势")

            # 发送录制命令
            ser.write(f"RECORD {gesture_id}\n".encode())

            # 等待响应
            start = time.time()
            response = ""
            while time.time() - start < 3:
                if ser.in_waiting:
                    line = ser.readline().decode().strip()
                    response += line + "\n"
                    if "Template" in line or "recorded" in line:
                        break
                time.sleep(0.01)

            print(f"  响应: {response.strip()}")
            all_records.append((gesture_id, i + 1))

    # 生成模板头文件
    print(f"\n{'='*50}")
    print("录制完成! 正在生成 templates.h ...\n")

    h_path = "templates.h"
    with open(h_path, "w", encoding="utf-8") as f:
        f.write("/* Auto-generated gesture templates */\n")
        f.write("#ifndef TEMPLATES_H\n#define TEMPLATES_H\n\n")
        f.write(f"// {len(all_records)} samples across {len(GESTURE_NAMES)} gestures\n")
        f.write("// TODO: 实际模板数据需从戒指端 Flash 读取并填入\n\n")
        f.write("#endif // TEMPLATES_H\n")

    print(f"模板文件已保存: {h_path}")
    print("\n下一步: 将模板数据烧录到戒指端 Flash")

    ser.close()

if __name__ == "__main__":
    main()
