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

# Microsoft Windows
# from mss.windows import MSS as mss

# send screenshot image to launchpad
def serialSendThread():
    global ser, breakSerial, cursorImage, sendimageflag
    x0 = 645  # top left x co-ordinate of the screen to be captured
    y0 = 450  # top left y co-ordinate of the screen to be captured
    w = 530   # width of the captured screen
    h = 50    # height of the captured screen
    datachunksize = 1024  # number of bytes sent to the launchpad at a time
    newPallet = [0]*datachunksize
    if ser.is_open: # check if serial port is open or not
        with mss() as sct:
            monitor_number = 0 # number of the monitor for which the screenshot will be captured
            mon = sct.monitors[monitor_number]
            monitor = {
                "top": mon["top"] + x0,
                "left": mon["left"] + y0,
                "width": w,
                "height": h,
                "mon": monitor_number,
            }
            templateImage = Image.new("RGB",(265,100)) # a template image on top of it the processed image will be pasted
            while not breakSerial:
                if sendimageflag:
                    sct_img = sct.grab(monitor) # capture a screenshot
                    imgpill = Image.frombytes("RGB", sct_img.size, sct_img.bgra, "raw", "BGRX") # convert the captured image into PIL format
                    leftHalfimg = imgpill.crop((0,0,265,50)) # take the left half of the image
                    rightHalfimg = imgpill.crop((265, 0, 530, 50)) # take the right half of the image
                    templateImage.paste(leftHalfimg,(0,0)) # paste the left half to the top of the template image
                    templateImage.paste(rightHalfimg, (0, 50)) # paste the right half to the bottom of the template image
                    resizedimg = templateImage.resize((128, 48), Image.ANTIALIAS) # resize the template image to lower resolution
                    imgcvt = resizedimg.convert('P') # convert the image to pallet-indexed image
                    pallet = imgcvt.getpalette() # get the pallet of the converted image
                    pindx = 0
                    for ii in range(len(pallet)): # format the pallet to consider the alpha channel as zero
                        if ii%3==0:
                            pindx += 1
                        newPallet[pindx] = pallet[ii]
                        pindx += 1
                    image = imgcvt.tobytes() # convert the image to byte array
                    pallet = bytearray(newPallet) # convert the pallet to byte array
                    for indx in range(0, len(image), datachunksize): # sent 1024 bytes of the image at a time
                        sendBuffer = image[indx:indx+datachunksize]
                        ser.write(sendBuffer) # sent data to the launchpad through serial port
                        time.sleep(0.2) # a small delay
                    ser.write(pallet) # send the pallet of the image
                    time.sleep(0.2) # a small delay

    print("finish send thread")

# receive command code from launchpad to be decoded and executed
def serialReceiveThread():
    global ser, breakSerial
    if ser.is_open:
        while not breakSerial:
            datareceive = ser.read_until()
            if datareceive:
                decodeCode(datareceive)
    print("finish revceive thread")

# decode the received unique code and send key stroke to the operating system
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

# main thread
if __name__ == "__main__":
    # create a serial object
    # ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1) #for linux
    ser = serial.Serial('/dev/tty.usbmodemM43210051', 460800, timeout=1) #for mac
    breakSerial = False
    sendimageflag = False
    threading.Thread(target=serialSendThread).start()       # start serial transmit thread
    threading.Thread(target=serialReceiveThread).start()    # start serial receive thread
    while True:
        option = input("input \"q\" for quite:\n")  # wait for user input
        if option == 'q':
            breakSerial = True
            time.sleep(1)
            break
    ser.close()   # close serial communication


