#  Author: Mrinmoy sarkar
#  email: msarkar@aggies.ncat.edu
#  Date: 10/8/2019

import time
import pyautogui
from PIL import Image
import serial
import threading


# MacOS X
from mss.darwin import MSS as mss

# GNU/Linux
# from mss.linux import MSS as mss
from math import atan2, cos, sin

# Microsoft Windows
# from mss.windows import MSS as mss


def serialSendThread():
    global ser, breakSerial, cursorImage, sendimageflag
    x0 = 645 #300
    y0 = 450
    w = 530
    h = 50 #390
    datachunksize = 1024
    newPallet = [0]*datachunksize
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
            templateImage = Image.new("RGB",(265,100))
            while not breakSerial:
                if sendimageflag:
                    sct_img = sct.grab(monitor)
                    imgpill = Image.frombytes("RGB", sct_img.size, sct_img.bgra, "raw", "BGRX")
                    leftHalfimg = imgpill.crop((0,0,265,50))
                    rightHalfimg = imgpill.crop((265, 0, 530, 50))
                    templateImage.paste(leftHalfimg,(0,0))
                    templateImage.paste(rightHalfimg, (0, 50))
                    resizedimg = templateImage.resize((128, 48), Image.ANTIALIAS)
                    imgcvt = resizedimg.convert('P')
                    pallet = imgcvt.getpalette()
                    pindx = 0
                    for ii in range(len(pallet)):
                        if ii%3==0:
                            pindx += 1
                        newPallet[pindx] = pallet[ii]
                        pindx += 1
                    image = imgcvt.tobytes()
                    pallet = bytearray(newPallet)
                    for indx in range(0, len(image), datachunksize):
                        sendBuffer = image[indx:indx+datachunksize]
                        ser.write(sendBuffer)
                        time.sleep(0.2) #0.25 was best
                    ser.write(pallet)
                    time.sleep(0.2) #0.25 was best

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
    global sendimageflag
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
        elif code == "SEND_IMAGE\n":
            sendimageflag = True
    except Exception as e:
        print("error: " + str(e))

if __name__ == "__main__":
    # ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1) #for linux
    ser = serial.Serial('/dev/tty.usbmodemM43210051', 460800, timeout=1) #for mac
    breakSerial = False
    sendimageflag = False
    threading.Thread(target=serialSendThread).start()
    threading.Thread(target=serialReceiveThread).start()
    while True:
        option = input("input \"q\" for quite:\n")
        if option == 'q':
            breakSerial = True
            time.sleep(1)
            break
    ser.close()


