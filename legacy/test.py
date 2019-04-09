# coding: utf-8
from PIL import Image

def DrawPath(img, x = 0, y = 0, w = 320, h = 120):
    # 从给定的x, y开始, 寻找离上一个x, y最近的0点, 直到整张图片遍历过一遍
    # img: 二维数组, (x, y)的点 = img[y][x]
    visited = [[False for j in range(w)] for i in range(h)]
    queue = [(x, y)]
    while len(queue) > 0:
        x, y = queue.pop(0)
        if x < 0 or x >= w or y < 0 or y >= h or visited[y][x]:
            continue
        visited[y][x] = True
        if img[y][x] == 0:
            yield x, y, img[y][x]
        queue.append((x + 1, y))
        queue.append((x - 1, y))
        queue.append((x, y + 1))
        queue.append((x, y - 1))
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
def getImage(filename):
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

img = getImage('out2.bmp')
i = 0
cx = 0
cy = 0
dis = 0
for x, y, c in DrawPathSV(img):
    i += 1
    #print x, y
    dis += abs(x - cx) + abs(y - cy)
    cx = x
    cy = y

print 'Done A: {0} distance: {1}'.format(i, dis)

batch = 0
dis = 0
for y in range(120):
    for x in range(320):
        c = img[y][x]
        if c == 0: # black
            dis += batch
            for _ in range(batch + 1):
                cx += 1
            batch = 0
        else:
            batch += 1
    if cx > 0:
        dis += 60
    dis += 1
    batch = 0
    cx = 0
print 'Done distance: {0}'.format(dis)