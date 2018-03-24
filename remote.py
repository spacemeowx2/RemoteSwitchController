from socket import *
from math import cos, sin
from time import sleep
TIME_BUTTON = 2
def ary2byte(a):
    o = 0
    for i in range(8):
        o |= (a[i] & 1) << i
    return o
class SwitchController:
    def __init__(self):
        self.reset()
    def reset(self):
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

        self.LX = 127
        self.LY = 127
        self.RX = 127
        self.RY = 127
    def __repr__(self):
        return 'ABXY: {0}{1}{2}{3} LR: {4}{5} ZLZR: {6}{7}'.format(
            self.A, self.B, self.X, self.Y,
            self.L, self.R, self.ZL, self.ZR
        )
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
class AutoDrawer:
    def __init__(self, sock, addr):
        self.sock = sock
        self.c = SwitchController()
        self.addr = addr
    def ctl(self):
        c = self.c
        c.ZL = 1
        c.ZR = 1
        self.send(1)
        c.A = 1
        self.send(1)
    def test(self):
        c = self.c
        c.ZL = 1
        self.send(1)
    def send(self, wait = 0):
        # print 'send' + repr(self.c)
        self.sock.sendto(str(self.c), self.addr)
        if wait > 0:
            sleep(wait)
            self.c.reset()
            self.sock.sendto(str(self.c), self.addr)
        # print 'send done'
addr = ('192.168.233.135', 34952)
udpClient = socket(AF_INET, SOCK_DGRAM)
drawer = AutoDrawer(udpClient, addr)
drawer.send()
while True:
    cmd = raw_input('>> ')
    if cmd == 'ctl':
        drawer.ctl()
    if cmd == 'w':
        drawer.c.d_up = 1
        drawer.send(TIME_BUTTON)
    if cmd == 's':
        drawer.c.d_down = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'a':
        drawer.c.d_left = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'd':
        drawer.c.d_right = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'l':
        drawer.c.L = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'r':
        drawer.c.R = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'A':
        drawer.c.A = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'home':
        drawer.c.home = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'test':
        drawer.test()
