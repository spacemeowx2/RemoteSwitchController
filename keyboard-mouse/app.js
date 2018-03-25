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
        return [Math.ceil(this.x * 255), Math.ceil(this.y * 255)]
    }
}
/**
 *      self.Y = 0
        self.B = 0
        self.A = 0
        self.X = 0
        self.L = 0
        self.R = 0
        self.ZL = 0
        self.ZR = 0

        self.minus = 0    8
        self.plus = 0
        self.lclick = 0
        self.rclick = 0
        self.home = 0
        self.capture = 0

        self.d_up = 0     15
        self.d_down = 0
        self.d_right = 0
        self.d_left = 0
 */
const hatMap = [
    [7,0,1],
    [6,8,2],
    [5,4,3]
]
class PadButton {
    constructor () {
        this.btns = []
        for (let i = 0; i < 14; i++) {
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
        let x = 1, y = 1
        y -= btns[14] & 1
        y += btns[15] & 1
        x += btns[16] & 1
        x -= btns[17] & 1
        byte3 = hatMap[y][x]
        return [byte1, byte2, byte3]
    }
}
class Gamepad {
    constructor (pad, ws) {
        const $ = (q) => pad.querySelector(q)
        this.pad = pad
        this.ls = new AnalogStick($('#l-stick'))
        this.rs = new AnalogStick($('#r-stick'))
        this.lockMouse = false
        this.ws = ws
        this.button = new PadButton()
        this.btnMap = new Map([
            [73, 14], // ijkl, dpad
            [75, 15],
            [76, 16],
            [74, 17],

            [32, 1],   // space, B
            [70, 5],   // f, R
            [82, 3],   // r, X
            [88, 3],   // x, X
            [49, 8],   // 1, -
            [50, 9],   // 2, +
            [81, 11],  // q, RClick
            [89, 0],   // y, Y
            [69, 2],   // e, A
            [66, 1],   // b, B
        ])
        this.mouseBtnMap = new Map([
            [0, 7],
            [2, 6]
        ])
        this.fakeLSMap = new Map([
            [87, [ 0, -1]],
            [83, [ 0,  1]],
            [68, [ 1,  0]],
            [65, [-1,  0]]
        ])
        this.fakeLSState = new Map([...this.fakeLSMap.keys()].map(k => [k, 0]))
        this.rX = 0
        this.rY = 0
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
        this.send()
    }
    onMouseUp (b) {
        const btns = this.button.btns
        if (this.mouseBtnMap.has(b)) {
            btns[this.mouseBtnMap.get(b)] = 0
        }
        this.send()
    }
    onKeyDown (c) {
        const btns = this.button.btns
        if (this.fakeLSState.has(c)) {
            this.fakeLSState.set(c, 1)
            this.fakeLS()
        } else if (this.btnMap.has(c)) {
            btns[this.btnMap.get(c)] = 1
        }
        this.send()
    }
    onKeyUp (c) {
        const btns = this.button.btns
        if (this.fakeLSState.has(c)) {
            this.fakeLSState.set(c, 0)
            this.fakeLS()
        } else if (this.btnMap.has(c)) {
            btns[this.btnMap.get(c)] = 0
        }
        this.send()
    }
    onMove (x, y) {
        if (this.tmr) {
            clearTimeout(this.tmr)
            this.tmr = undefined
        }
        x += this.rX
        y += this.rY
        this.rs.x = x / 128 + 0.5
        this.rs.y = y / 128 + 0.5
        this.send()
        if (x != 0 || y != 0) {
            this.tmr = setTimeout(() => this.onMove(0, 0), 20)
        }
    }
    send () {
        const bytes = [...this.button.toBytes(), ...this.ls.toBytes(), ...this.rs.toBytes(), 0]
        const u8 = new Uint8Array(bytes)
        this.ws.send(u8.buffer)
    }
}
let ws = new WebSocket('ws://localhost:26214')
let gamepad = new Gamepad(document.querySelector('.gamepad'), ws)
gamepad.bind(document.querySelector('#input'))
