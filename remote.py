from socket import *
from math import cos, sin
from time import sleep
import struct
def ary2byte(a):
    o = 0
    for i in range(8):
        o |= (a[i] & 1) << (8 - i + 1)
    return o
class SwitchController:
    def __init__(self):
        self.Y = 0
        self.B = 0
        self.A = 0
        self.X = 0
        self.L = 0
        self.R = 0
        self.ZL = 0
        self.ZR = 0

        self.minus = 0
        self.plus = 0
        self.lclick = 0
        self.rclick = 0
        self.home = 0
        self.capture = 0

        self.d_up = 0
        self.d_down = 0
        self.d_right = 0
        self.d_left = 0

        self.LX = 128
        self.LY = 128
        self.RX = 128
        self.RY = 128
    def __str__(self):
        byte1 = [self.Y, self.B, self.A, self.X, self.L, self.R, self.ZL, self.ZR]
        byte2 = [self.minus, self.plus, self.lclick, self.rclick, self.home, self.capture, 0, 0]
        byte3 = [self.d_up, self.d_down, self.d_right, self.d_left, 0, 0, 0, 0]
        byte1 = ary2byte(byte1)
        byte2 = ary2byte(byte2)
        byte3 = ary2byte(byte3)
        return chr(byte1) + chr(byte2) + chr(byte3) + \
               chr(int(self.LX) & 0xFF) + chr(int(self.LY) & 0xFF) + \
               chr(int(self.RX) & 0xFF) + chr(int(self.RY) & 0xFF) + \
               chr(0)

addr = ('192.168.233.135', 34952)
udpClient = socket(AF_INET,SOCK_DGRAM)
c = SwitchController()
r = 0
while True:
    sleep(0.01)
    c.LX = cos(r) * 128 + 128
    c.LY = sin(r) * 128 + 128
    r += 0.01
    udpClient.sendto(str(c), addr)

udpClient.close()
