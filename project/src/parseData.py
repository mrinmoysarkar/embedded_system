import time

import cv2
# import mss
import numpy as np
import pyautogui
from PIL import Image
import serial
import threading


# MacOS X
# from mss.darwin import MSS as mss

# GNU/Linux
from mss.linux import MSS as mss
from math import atan2, cos, sin

# Microsoft Windows
# from mss.windows import MSS as mss


ser = serial.Serial('/dev/ttyACM0',115200, timeout=1)
breakSerial = False
cursorImage = Image.open('cursor.jpg')
cursorImage = cursorImage.resize((20, 20), Image.ANTIALIAS)

def serialSendThread():
    global ser, breakSerial, cursorImage
    x0 = 450
    y0 = 690
    w = 530
    h = 390

    if ser.is_open:
        with mss() as sct:
            monitor_number = 0
            mon = sct.monitors[monitor_number]
            monitor = {
                "top": mon["top"] + x0,
                "left": mon["left"] + y0,
                "width": w,
                "height": h,
                "mon": monitor_number,
            }
            while not breakSerial:
            # for i in range(1):
                sct_img = sct.grab(monitor)
                mousex, mousey = pyautogui.position()
                # print(mousex,mousey)
                mousex, mousey = mousex-460, mousey-410
                imgpill = Image.frombytes("RGB", sct_img.size, sct_img.bgra, "raw", "BGRX")
                # if ((mousex+20) <= 640) and ((mousey+20) <= 380 and mousex >=0 and mousey >= 0):
                #     imgpill.paste(cursorImage, box=(mousex,mousey,mousex+20,mousey+20))
                # imgpill.show()
                imgpill = imgpill.resize((128, 128), Image.ANTIALIAS)
                imgcvt = imgpill.convert('P')
                pallet = imgcvt.getpalette()
                image = imgcvt.tobytes()
                pallet = bytearray(pallet)
                ser.write(b'STARTI')
                # time.sleep(0.2)
                ser.write(image)
                # time.sleep(0.5)
                ser.write(b'STARTP')
                # time.sleep(0.2)
                ser.write(pallet)

                # datareceive = ser.read_until()
                # if datareceive:
                #     decodeCode(datareceive)
                time.sleep(0.5)
    print("finish send thread")


def serialReceiveThread():
    global ser, breakSerial
    if ser.is_open:
        while not breakSerial:
            datareceive = ser.read_until()
            if datareceive:
                decodeCode(datareceive)
    print("finish revceive thread")

def decodeCode(code):
    try:
        code = code.decode("utf-8")
        if code == "RIGHT_ARROW\n":
            pyautogui.keyDown('right')
        elif code == "LEFT_ARROW\n":
            pyautogui.keyDown('left')
        elif code == "LR_ARROW_UP\n":
            pyautogui.keyUp('left')
            pyautogui.keyUp('right')
        elif code == "UP_ARROW\n":
            pyautogui.keyDown('up')
        elif code == "DOWN_ARROW\n":
            pyautogui.keyDown('down')
        elif code == "UD_ARROW_UP\n":
            pyautogui.keyUp('up')
            pyautogui.keyUp('down')
        elif code == "SPACE_BAR\n":
            pyautogui.keyDown('space')
        elif code == "SPACE_BAR_UP\n":
            pyautogui.keyUp('space')
        elif code == "Z\n":
            pyautogui.keyDown('z')
        elif code == "Z_UP\n":
            pyautogui.keyUp('z')
        elif code == "X\n":
            pyautogui.keyDown('x')
        elif code == "X_UP\n":
            pyautogui.keyUp('x')
    except Exception as e:
        print("error: " + str(e))


if __name__ == "__main__":
    threading.Thread(target=serialSendThread).start()
    threading.Thread(target=serialReceiveThread).start()

    while True:
        option = input("input \"q\" for quite:\n")
        if option == 'q':
            breakSerial = True
            time.sleep(3)
            break
    ser.close()
