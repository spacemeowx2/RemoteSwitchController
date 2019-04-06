class AnalogStick {
    constructor (elem) {
        this.el = elem
        this.p = elem.querySelector('.point')
        this._x = 0.5
        this._y = 0.5
        this.move()
    }
    move () {
        this.p.style.left = `${this.x * 80}px`
        this.p.style.top = `${this.y * 80}px`
    }
    set x (v) {
        if (v > 1) {
            v = 1
        }
        if (v < 0) {
            v = 0
        }
        this._x = v
        this.move()
    }
    get x () {
        return this._x
    }
    set y (v) {
        if (v > 1) {
            v = 1
        }
        if (v < 0) {
            v = 0
        }
        this._y = v
        this.move()
    }
    get y () {
        return this._y
    }
    toBytes () {
        // 2^12 = 4096
        const x = Math.ceil(this.x * 4095)
        const y = Math.ceil(this.y * 4095)

        const data = [x & 0xFF, ((y & 0xF) << 8) | ((x >> 8) & 0xF), y >> 4]

        return data
    }
}

const hatMap = [
    [7,0,1],
    [6,8,2],
    [5,4,3]
]
class PadButton {
    constructor () {
        this.btns = []
        for (let i = 0; i < 22; i++) {
            this.btns.push(0)
        }
    }
    toBytes () {
        const btns = this.btns
        let byte1 = 0, byte2 = 0, byte3 = 0
        for (let i = 0; i < 8; i++) {
            byte1 |=  (btns[i] & 1) << i
        }
        for (let i = 0; i < 6; i++) {
            byte2 |=  (btns[i + 8] & 1) << i
        }
        for (let i = 0; i < 8; i++) {
            byte3 |=  (btns[i + 8 + 6] & 1) << i
        }
        return [byte1, byte2, byte3]
    }
}
class MouseSixAxis {
    constructor (sixAxis) {
        this.sixAxis = sixAxis
        this.rX = 0
        this.rY = 0
        this.lt = performance.now()
        this.ds = []
    }
    onMove (x, y) {
        this.rX += x
        this.rY += y
        const MAX = 800
        if (this.rX > MAX) {
            this.rX = MAX
        } else if (this.rX < -MAX) {
            this.rX = -MAX
        }
        if (this.rY > MAX) {
            this.rY = MAX
        } else if (this.rY < -MAX) {
            this.rY = -MAX
        }
    }
    onSend () {
        const A = 25
        const x = this.rX * A
        const y = this.rY * A
        this.rX = this.rY = 0

        this.sixAxis.gy = y
        this.sixAxis.gz = -x
        // console.log(x, y, this.sixAxis.gz, this.sixAxis.gy)
        this.sixAxis.update()
    }
}
class SixAxis {
    constructor (elem) {
        this.elem = elem
        this.data = [
            [0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0]
        ]
        this.gx = 0
        this.gy = 0
        this.gz = 0

        this.ax = 0
        this.ay = 0
        this.az = 0

        const keys = ['ax', 'ay', 'az', 'gx', 'gy', 'gz']
        for (let k of keys) {
            const e = document.getElementById(k)
            e.addEventListener('input', _ => {
                this[k] = parseInt(e.value)
            })
            this[k] = parseInt(e.value)
        }
    }
    update () {
        const cur = [
            this.ax, this.ay, this.az,
            this.gx, this.gy, this.gz
        ]
        // this.gx = this.gy = this.gz = 0
        // this.ax = this.ay = this.az = 0

        const { data } = this
        data[2] = data[1]
        data[1] = data[0]
        data[0] = cur

        const fields = ['ax', 'ay', 'az', 'gx', 'gy', 'gz']
        const vs = new Int16Array(cur)
        let s = []
        for (let i = 0; i < fields.length; i++) {
            s.push(`${fields[i]}: ${vs[i]}`)
        }
        this.elem.innerText = s.join('\n')
    }
    toBytes () {
        const { data } = this
        const dat = new Int16Array([...data[0], ...data[1], ...data[2]])
        return [...new Uint8Array(dat.buffer)]
    }
}
class Gamepad {
    constructor (pad, ws) {
        const $ = (q) => pad.querySelector(q)
        this.pad = pad
        this.ls = new AnalogStick($('#l-stick'))
        this.rs = new AnalogStick($('#r-stick'))
        this.sixAxis = new SixAxis($('#six-axis'))
        this.ms = new MouseSixAxis(this.sixAxis)
        this.lockMouse = false
        this.ws = ws
        this.button = new PadButton()
        this.btnMap = new Map([
            [73, 15], // ijkl, dpad
            [75, 14],
            [76, 16],
            [74, 17],

            [32, 2],   // space, B
            [70, 6],   // f, R
            [82, 1],   // r, X
            [88, 1],   // x, X
            [49, 8],   // 1, -
            [50, 9],   // 2, +
            [81, 10],  // q, RClick
            [89, 0],   // y, Y
            [69, 3],   // e, A
            [66, 2],   // b, B

            [72, 12],  // h, HOME
            [80, 13],  // p, CAPTURE
        ])
        this.mouseBtnMap = new Map([
            [0, 7],    // ZR
            [2, 21]     // ZL
        ])
        this.fakeLSMap = new Map([
            [87, [ 0,  1]],
            [83, [ 0, -1]],
            [68, [ 1,  0]],
            [65, [-1,  0]]
        ])
        this.fakeLSState = new Map([...this.fakeLSMap.keys()].map(k => [k, 0]))
        this.rX = 0
        this.rY = 0

        setTimeout(() => this.send(), 1)
    }
    bind (ipt) {
        this.ipt = ipt
        ipt.addEventListener('click', () => {
            ipt.requestPointerLock()
            this.lockMouse = true
        })
        ipt.addEventListener('mousemove', ({movementX, movementY}) => {
            if (document.pointerLockElement === ipt) {
                this.onMove(movementX, movementY)
            }
        })
        document.addEventListener('keydown', ({keyCode}) => {
            this.onKeyDown(keyCode)
        }, false)
        document.addEventListener('keyup', ({keyCode}) => {
            this.onKeyUp(keyCode)
        })
        document.addEventListener('mousedown', ({button}) => {
            this.onMouseDown(button)
        }, false)
        document.addEventListener('mouseup', ({button}) => {
            this.onMouseUp(button)
        })
    }
    fakeLS () {
        let x = 1, y = 1
        for (let k of this.fakeLSMap.keys()) {
            if (this.fakeLSState.get(k) === 1) {
                let t = this.fakeLSMap.get(k)
                x += t[0]
                y += t[1]
            }
        }
        this.ls.x = x / 2
        this.ls.y = y / 2
    }
    onMouseDown (b) {
        const btns = this.button.btns
        if (this.mouseBtnMap.has(b)) {
            btns[this.mouseBtnMap.get(b)] = 1
        }
        // this.send()
    }
    onMouseUp (b) {
        const btns = this.button.btns
        if (this.mouseBtnMap.has(b)) {
            btns[this.mouseBtnMap.get(b)] = 0
        }
        // this.send()
    }
    onKeyDown (c) {
        const btns = this.button.btns
        if (this.fakeLSState.has(c)) {
            this.fakeLSState.set(c, 1)
            this.fakeLS()
        } else if (this.btnMap.has(c)) {
            btns[this.btnMap.get(c)] = 1
        }
        // this.send()
    }
    onKeyUp (c) {
        const btns = this.button.btns
        if (this.fakeLSState.has(c)) {
            this.fakeLSState.set(c, 0)
            this.fakeLS()
        } else if (this.btnMap.has(c)) {
            btns[this.btnMap.get(c)] = 0
        }
        // this.send()
    }
    onMove (x, y) {
        this.ms.onMove(x, y)
        this.rX += x
        this.rY += y
    }
    mouseToBytes () {
        const ary = new Int16Array([this.rX * 20, this.rY * 20])
        this.rX = this.rY = 0
        return [...new Uint8Array(ary.buffer)]
    }
    send () {
        // const bytes = [
        //     0x30, 0x00, 0x91,
        //     ...this.button.toBytes(),
        //     ...this.ls.toBytes(),
        //     ...this.rs.toBytes(),
        //     0x00,
        //     ...this.sixAxis.toBytes(),
        //     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        //     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        // ]
        const bytes = [
            0xFF, 0x30,
            ...this.button.toBytes(),
            ...this.ls.toBytes(),
            ...this.rs.toBytes(),
            ...this.mouseToBytes()
        ]
        const u8 = new Uint8Array(bytes)
        this.ws.send(u8.buffer)
        this.ms.onSend()

        setTimeout(() => this.send(), 1)
    }
}
let ws = new WebSocket('ws://localhost:26214')
ws.onopen = () => {
    let gamepad = new Gamepad(document.querySelector('.gamepad'), ws)
    gamepad.bind(document.querySelector('#input'))
    let timer = null
    window.autoA = () => {
        let a = true
        if (timer) {
            clearInterval(timer)
        }
        timer = setInterval(() => {
            if (a) {
                gamepad.onKeyDown(69)
            } else {
                gamepad.onKeyUp(69)
            }
            a = !a
        }, 100)
    }
    window.cancelA = () => clearInterval(timer)
}