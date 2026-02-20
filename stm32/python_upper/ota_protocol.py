import serial
import struct
import time
import os
import sys

# --- 配置区域 ---
SERIAL_PORT = 'COM3'
BAUD_RATE = 115200
CMD_START = 0x00
CMD_COMPLETE = 0x01
CMD_WRITE = 0x02
CMD_COPY = 0x03
FRAME_HEAD = b'\x55\xAA'


# --- CRC16 算法 (与 MCU 端保持一致) ---
def calc_crc16(data):
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for i in range(8):
            if (crc & 1) != 0:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


# --- 组包函数 ---
def build_frame(cmd, payload):
    length = len(payload)
    # 格式: Head(2) + Len(1) + Cmd(1) + Payload(N)
    # 这里的 'BB' 表示两个 unsigned char
    frame_body = struct.pack('B', length) + struct.pack('B', cmd) + payload

    # 计算 Body 的 CRC
    crc = calc_crc16(frame_body)
    # print(f"crc is {crc}")

    # 拼接完整帧: Head + Body + CRC(2 bytes, Little Endian)
    full_frame = FRAME_HEAD + frame_body + struct.pack('<H', crc)
    return full_frame


# --- 发送主流程 ---
def ota_update(file_path):
    if not os.path.exists(file_path):
        print("Error: File not found.")
        return

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1.0)    # 这里设置阻塞时间
        print(f"Opened {SERIAL_PORT} successfully.")
    except Exception as e:
        print(f"Serial Error: {e}")
        return

    file_size = os.path.getsize(file_path)
    print(f"Start OTA: {file_path} ({file_size} bytes)")

    # 先发送开始信号
    frame = build_frame(CMD_START, b"")     # 字符串不能和字节串相加，因此这里确保是字节串
    ser.write(frame)
    ack = ser.read(1)
    for attempt in range(5):
        if ack == b'\x06':
            break
        else:
            print(f"\nStart Error, No ACK or NACK (Got {ack})")
            ser.write(frame)
            ack = ser.read(1)
            if attempt == 4:
                print("\nClose serial")
                ser.close()
                return False

    with open(file_path, 'rb') as f:
        offset = 0
        while True:
            chunk = f.read(128)
            if not chunk:
                break

            # 1. 组包发送
            frame = build_frame(CMD_WRITE, chunk)
            for attempt in range(5):
                ser.write(frame)

                # RL78 等慢速芯片可能需要延时，STM32 通常不需要
                # time.sleep(0.02)

                # 2. 等待 ACK (握手)
                ack = ser.read(1)

                if ack == b'\x06':  # 假设 0x06 是 ACK
                    offset += len(chunk)
                    # 打印进度条
                    progress = offset / file_size * 100
                    sys.stdout.write(f"\rProgress: {progress:.1f}% [{offset}/{file_size}]")
                    sys.stdout.flush()
                    break
                else:
                    print(f"\nError at offset {offset}: No ACK or NACK (Got {ack})")
                    if attempt == 4:
                        print("\nClose serial")
                        ser.close()
                        return False
        # 发送结束信号
        frame = build_frame(CMD_COMPLETE, b"")
        ser.write(frame)
        ack = ser.read(1)
        for attempt in range(5):
            if ack == b'\x06':
                break
            else:
                print("\nStart Error")
                ser.write(frame)
                ack = ser.read(1)
                if attempt == 4:
                    print("\nClose serial")
                    ser.close()
                    return False

    print("\nOTA Success! Device should reboot now.")
    ser.close()
    return True


# --- 差分升级主流程 ---
def ota_update_diff(path_old, path_new):
    if not os.path.exists(path_old):
        print("Error: Old file not found.")
        return
    if not os.path.exists(path_new):
        print("Error: New file not found.")
        return

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1.0)    # 这里设置阻塞时间
        print(f"Opened {SERIAL_PORT} successfully.")
    except Exception as e:
        print(f"Serial Error: {e}")
        return

    file_size = os.path.getsize(path_new)
    actual_size = file_size     # 记录实际更新包大小（主要用于差分升级相关）
    print(f"Start OTA: {path_new} ({file_size} bytes)")

    # 先发送开始信号
    frame = build_frame(CMD_START, b"")     # 字符串不能和字节串相加，因此这里确保是字节串
    ser.write(frame)
    ack = ser.read(1)
    for attempt in range(5):
        if ack == b'\x06':
            break
        else:
            print(f"\nStart Error, No ACK or NACK (Got {ack})")
            ser.write(frame)
            ack = ser.read(1)
            if attempt == 4:
                print("\nClose serial")
                ser.close()
                return False

    with (open(path_old, 'rb') as f1,
          open(path_new, 'rb') as f2): # with使得结束后自动close
        offset = 0
        while True:
            chunk1 = f1.read(128)
            chunk2 = f2.read(128)
            if not chunk2:
                break

            # 分情况组包
            if chunk1 == chunk2:
                # print(f"\nSame at offset {offset}")

                actual_size -= len(chunk2)
                frame = build_frame(CMD_COPY, bytes([len(chunk2)]))
            else:
                frame = build_frame(CMD_WRITE, chunk2)

            for attempt in range(5):
                ser.write(frame)
                ack = ser.read(1)
                if ack == b'\x06':
                    offset += len(chunk2)
                    progress = offset/file_size*100
                    sys.stdout.write(f"\rProgress: {progress:.1f}% [{offset}/{file_size}]")
                    sys.stdout.flush()
                    break
                else:
                    print(f"\nError at offset {offset}: No ACK or NACK (Got {ack})")
                    if attempt == 4:
                        print("\nClose serial")
                        ser.close()
                        return False

        # 结束信号
        frame = build_frame(CMD_COMPLETE, b"")
        ser.write(frame)
        ack = ser.read(1)
        for attempt in range(5):
            if ack == b'\x06':
                break
            else:
                print("\nStart Error")
                ser.write(frame)
                ack = ser.read(1)
                if attempt == 4:
                    print("\nClose serial")
                    ser.close()
                    return False
        print(f"\nactual download size : {actual_size}")


if __name__ == "__main__":
    res = input("\nPlease choose update mode:\n[A] normal\n[B] diff: ")

    if res == "A":
        path = input("\nPlease enter path of target file(.bin): ")
        if len(path)==0:
            ota_update(r"J:\my_project\04_my_github\ota-update\stm32\app\version_bin\version_1_0.bin")
        else:
            ota_update(path)

    elif res == "B":
        old = input("\nPlease enter old path of target file(.bin): ")
        new = input("\nPlease enter new path of target file(.bin): ")
        ota_update_diff(old, new)