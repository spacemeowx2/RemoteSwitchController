from socket import *
from math import cos, sin
from time import sleep
from PIL import Image
TIME_BUTTON = 0.05
addr = ('192.168.233.135', 34952)

def ary2byte(a):
    o = 0
    for i in range(8):
        o |= (a[i] & 1) << i
    return o
class SwitchController:
    hat_map = [
        [7,0,1],
        [6,8,2],
        [5,4,3]
    ]
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

        self.LX = 128
        self.LY = 128
        self.RX = 128
        self.RY = 128
    def _dpad2byte(self):
        x = 1
        y = 1
        x += self.d_right & 1
        x -= self.d_left & 1
        y -= self.d_up & 1
        y += self.d_down & 1
        return self.hat_map[y][x]
    def __repr__(self):
        return 'ABXY: {0}{1}{2}{3} LR: {4}{5} ZLZR: {6}{7}'.format(
            self.A, self.B, self.X, self.Y,
            self.L, self.R, self.ZL, self.ZR
        )
    def __str__(self):
        byte1 = [self.Y, self.B, self.A, self.X, self.L, self.R, self.ZL, self.ZR]
        byte2 = [self.minus, self.plus, self.lclick, self.rclick, self.home, self.capture, 0, 0]
        byte1 = ary2byte(byte1)
        byte2 = ary2byte(byte2)
        byte3 = self._dpad2byte()
        return chr(byte1) + chr(byte2) + chr(byte3) + \
               chr(int(self.LX) & 0xFF) + chr(int(self.LY) & 0xFF) + \
               chr(int(self.RX) & 0xFF) + chr(int(self.RY) & 0xFF) + \
               chr(0)
class Cursor:
    def __init__(self, drawer):
        self.drawer = drawer
        self.x = 0
        self.y = 0
    def go(self, x, y):
        dx = x - self.x
        dy = y - self.y
        while dx != 0:
            if dx > 0:
                self.drawer.goRight()
                dx -= 1
            else:
                self.drawer.goLeft()
                dx += 1
        while dy != 0:
            if dy > 0:
                self.drawer.goDown()
                dy -= 1
            else:
                self.drawer.goUp()
                dy += 1
        self.x = x
        self.y = y
def DrawPathSV(img, w = 320, h = 120):
    x = 0
    y = 0
    vx = 1
    for y in range(h):
        while x >= 0 and x < w:
            if img[y][x] == 0:
                yield x, y, img[y][x]
            x += vx
        vx *= -1
        x += vx
def DrawPathSH(img, w = 320, h = 120):
    x = 0
    y = 0
    vy = 1
    for x in range(w):
        while y >= 0 and y < h:
            if img[y][x] == 0:
                yield x, y, img[y][x]
            y += vy
        vy *= -1
        y += vy
class AutoDrawer:
    def __init__(self, sock, addr):
        self.sock = sock
        self.c = SwitchController()
        self.addr = addr
        self.cx = 0
        self.cy = 0
    def A(self):
        self.c.A = 1
        self.send(TIME_BUTTON)
    def goLeftB(self):
        self.c.d_left = 1
        self.send(6)
    def goLeft(self):
        self.c.d_left = 1
        self.send(TIME_BUTTON)
        sleep(TIME_BUTTON)
    def goRight(self):
        self.c.d_right = 1
        self.send(TIME_BUTTON)
        sleep(TIME_BUTTON)
    def goDown(self):
        self.c.d_down = 1
        self.send(TIME_BUTTON)
        sleep(TIME_BUTTON)
    def goUp(self):
        self.c.d_down = 1
        self.send(TIME_BUTTON)
        sleep(TIME_BUTTON)
    def getImage(self, filename):
        image = [[0 for j in range(320)] for i in range(120)]
        with Image.open(filename) as img:
            img = img.resize((320, 120))
            for y in range(120):
                for x in range(320):
                    c = img.getpixel((x, y))
                    if type(c) == tuple:
                        c = 1.0 * sum(c) / len(c)
                    image[y][x] = 0 if c < 128 else 1
        return image
    def drawv2(self, filename):
        img = self.getImage(filename)
        visited = [[False for j in range(320)] for i in range(120)]
        # print 'going to left top'
        # self.go0()
        cur = Cursor(self)
        for x, y, c in DrawPathSV(img):
            print 'go', x, y
            cur.go(x, y)
            self.A()
    def draw(self, filename):
        img = self.getImage(filename)
        print 'going to left top'
        self.go0()
        cx = 0

        i = 0
        batch = 0
        print 'start drawing'
        for y in range(120):
            for x in range(320):
                if img[y][x] == 0: # black
                    print 'move right', batch + 1
                    for _ in range(batch + 1):
                        self.goRight()
                        cx += 1
                    batch = 0
                    self.A()
                    print 'A'
                else:
                    batch += 1
            print 'newline'
            if cx > 0:
                self.goLeftB()
            self.goDown()
            batch = 0
            cx = 0
    def ctl(self):
        c = self.c
        c.A = 1
        self.send(TIME_BUTTON)
        sleep(2)
        c.ZL = 1
        c.ZR = 1
        self.send(1)
        c.A = 1
        self.send(TIME_BUTTON)
    def go0(self):
        c = self.c
        c.d_up = 1
        self.send(2.5)
        c.d_left = 1
        self.send(6)
    def go1(self):
        c = self.c
        c.d_down = 1
        self.send(2.5)
        c.d_right = 1
        self.send(6)
    def test(self):
        pass
    def send(self, wait = 0):
        # print 'send' + repr(self.c)
        self.sock.sendto(str(self.c), self.addr)
        if wait > 0:
            sleep(wait)
            self.c.reset()
            self.sock.sendto(str(self.c), self.addr)
        # print 'send done'
udpClient = socket(AF_INET, SOCK_DGRAM)
drawer = AutoDrawer(udpClient, addr)
drawer.send()
while True:
    args = raw_input('>> ').split(' ')
    cmd = args[0]
    args = args[1:]
    if cmd == 'draw':
        drawer.draw(args[0])
    if cmd == 'drawv2':
        drawer.drawv2(args[0])
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
    if cmd == 'b':
        drawer.c.B = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'x':
        drawer.c.X = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'y':
        drawer.c.Y = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'home':
        drawer.c.home = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'minus':
        drawer.c.minus = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'plus':
        drawer.c.plus = 1
        drawer.send(TIME_BUTTON)
    if cmd == 'go0':
        drawer.go0()
    if cmd == 'go1':
        drawer.go1()
    if cmd == 'test':
        drawer.test()
