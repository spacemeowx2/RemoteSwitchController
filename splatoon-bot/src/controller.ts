import { createSocket, RemoteInfo, Socket } from 'dgram'

interface Byteable {
    toBytes(): Uint8Array
}
export abstract class ProTransport {
    abstract send (buf: ArrayBuffer): void
}
export class ProUDPTransport extends ProTransport {
    private socket: Socket
    private port: number
    private address: string
    constructor (private addr: string) {
        super()
        this.socket = createSocket('udp4', (msg: Buffer, rinfo: RemoteInfo) => this.onMessage(msg, rinfo))
        const [address, port] = addr.split(':')
        this.address = address
        this.port = parseInt(port)
    }
    private onMessage (msg: Buffer, rinfo: RemoteInfo) {
        // never called
    }
    send (buf: ArrayBuffer) {
        this.socket.send(new Uint8Array(buf), this.port, this.address)
    }
}
const enum ButtonBitMap {
    Y = 0,
    X = 1,
    B = 2,
    A = 3,
    _RightSR = 4,
    _RightSL = 5,
    R = 6,
    ZR = 7,

    Minus = 8,
    Plus = 9,
    RStick = 10,
    LStick = 11,
    Home = 12,
    Capture = 13,
    _reversed1 = 14,
    _reversed2 = 15,

    Down = 16,
    Up = 17,
    Right = 18,
    Left = 19,
    _LeftSR = 20,
    _LeftSL = 21,
    L = 22,
    ZL = 23,
}
class PadButton implements Byteable {
    private btns = new Uint8Array(3)
    toBytes () {
        return this.btns
    }
    setKey (key: ButtonBitMap, pressed: boolean) {
        const idx = Math.floor(key / 8)
        const bit = key % 8
        const { btns } = this
        if (pressed) {
            btns[idx] = btns[idx] | (1 << bit)
        } else {
            btns[idx] = btns[idx] & ~(1 << bit)
        }
    }
    getKey (key: ButtonBitMap) {
        const idx = Math.floor(key / 8)
        const bit = key % 8
        const { btns } = this
        return (btns[idx] & (1 << bit)) !== 0
    }
    reset() {
        const { btns } = this
        btns.fill(0)        
    }
}
class AnalogStick implements Byteable {
    protected _x = 0.5
    protected _y = 0.5
    constructor () {
    }
    set x (v) {
        if (v > 1) {
            v = 1
        }
        if (v < 0) {
            v = 0
        }
        this._x = v
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
    }
    get y () {
        return this._y
    }
    toBytes () {
        // 2^12 = 4096
        const x = Math.ceil(this.x * 4095)
        const y = Math.ceil(this.y * 4095)

        const data = [x & 0xFF, ((y & 0xF) << 8) | ((x >> 8) & 0xF), y >> 4]

        return new Uint8Array(data)
    }
}
class Motion implements Byteable {
    offsetX = 0
    offsetY = 0
    toBytes () {
        const ary = new Int16Array([this.offsetX, this.offsetY])

        this.offsetX = this.offsetY = 0
        return new Uint8Array(ary.buffer)
    }
}
export class ProController implements Byteable {
    button = new PadButton()
    leftStick = new AnalogStick()
    rightStick = new AnalogStick()
    motion = new Motion()
    constructor (private transport: ProTransport) {

    }
    send () {
        this.transport.send(this.toBytes().buffer)
    }
    toBytes () {
        const bytes = [
            0xFF, 0x30,
            ...this.button.toBytes(),
            ...this.leftStick.toBytes(),
            ...this.rightStick.toBytes(),
            ...this.motion.toBytes()
        ]
        return new Uint8Array(bytes)
    }
}
