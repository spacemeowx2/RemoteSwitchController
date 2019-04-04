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
        this.sixAxis.x
        const MAX = 400
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
        this.sixAxis.x = x
        this.sixAxis.y = y
    }
}
class SixAxis {
    constructor () {
        this.history = [
            [0, 0, 0, 0, 0, 0]
            [0, 0, 0, 0, 0, 0]
        ]
        this.x = 0
        this.y = 0
        this.z = 0
    }
    toBytes () {
        const cur = [0, 0, 0, this.x, this.y, this.z]
        this.x = this.y = this.z = 0

        const { history } = this
        const dat = new Int16Array([cur, ...history])
        history[1] = history[0]
        history[0] = cur
        return [...new Uint8Array(dat.buffer)]
    }
}
class Gamepad {
    constructor (pad, ws) {
        const $ = (q) => pad.querySelector(q)
        this.pad = pad
        this.ls = new AnalogStick($('#l-stick'))
        this.rs = new AnalogStick($('#r-stick'))
        this.sixAxis = new SixAxis()
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

        setInterval(() => this.send(), 1)
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
    }
    send () {
        const bytes = [
            0x30, 0x00, 0x91,
            ...this.button.toBytes(),
            ...this.ls.toBytes(),
            ...this.rs.toBytes(),
            0x00,
            ...this.sixAxis.toBytes()
        ]
        const u8 = new Uint8Array(bytes)
        this.ws.send(u8.buffer)
    }
}
let ws = new WebSocket('ws://localhost:26214')
ws.onopen = () => {
    let gamepad = new Gamepad(document.querySelector('.gamepad'), ws)
    gamepad.bind(document.querySelector('#input'))
}
