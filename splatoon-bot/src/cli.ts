import { createInterface } from 'readline'
import { ProController, ProUDPTransport, ButtonBitMap, PadButton } from './controller'
import { delay } from './util'

const StrBtnMap: Record<string, ButtonBitMap> = {
    'w': ButtonBitMap.Up,
    's': ButtonBitMap.Down,
    'a': ButtonBitMap.Left,
    'd': ButtonBitMap.Right,
    'h': ButtonBitMap.Home,
    'home': ButtonBitMap.Home,
    'A': ButtonBitMap.A,
    'b': ButtonBitMap.B,
    'B': ButtonBitMap.B,
    'x': ButtonBitMap.X,
    'y': ButtonBitMap.Y,
    'zl': ButtonBitMap.ZL,
    'zr': ButtonBitMap.ZR,
    'l': ButtonBitMap.L,
    'r': ButtonBitMap.R,
    '+': ButtonBitMap.Plus,
    '-': ButtonBitMap.Minus,
    'c': ButtonBitMap.Capture,
    'capture': ButtonBitMap.Capture,
}

const DefaultPressDelay = 100

function keyPresser(controller: ProController) {
    const { button } = controller
    return async(key: ButtonBitMap, delayTime = DefaultPressDelay) => {
        if (isNaN(delayTime)) {
            delayTime = DefaultPressDelay
        }
        button.setKey(key, true)
        controller.send()
        await delay(delayTime)
        button.setKey(key, false)
        controller.send()
    }
}

async function main (...argv: string[]) {
    const rl = createInterface({
        input: process.stdin,
        prompt: '>>> ',
        crlfDelay: Infinity
    })
    if (argv.length === 0) {
        argv[0] = 'localhost:34952'
        console.warn('use default localhost server:', argv[0])
    } else if (argv.length != 1) {
        console.error(`cli.ts UDPServer:Port`)
        rl.close()
        return
    }
    const c = new ProController(new ProUDPTransport(argv[0]))
    const { button, leftStick } = c
    const press = keyPresser(c)
    const StrBtnKeys = Object.keys(StrBtnMap)
    const handler = async (line: string) => {
        let [cmd, ...params] = line.trim().split(',')
        params = params.map(i => i.trim())
        if (StrBtnKeys.includes(cmd)) {
            const key = StrBtnMap[cmd]
            await press(key)
        } else {
            switch (cmd) {
                case 'ctl':
                    await press(ButtonBitMap.A)
                    await delay(2000)
                    await Promise.all([
                        press(ButtonBitMap.ZL),
                        press(ButtonBitMap.ZR)
                    ])
                    await delay(1000)
                    await press(ButtonBitMap.A)
                    break
                case 'sleep': {
                    const ms = parseInt(params[0])
                    await delay(ms)
                    break
                }
                case 'down': {
                    if (StrBtnKeys.includes(params[0])) {
                        const key = StrBtnMap[params[0]]
                        button.setKey(key, true)
                        c.send()
                    }
                    break
                }
                case 'up': {
                    if (StrBtnKeys.includes(params[0])) {
                        const key = StrBtnMap[params[0]]
                        button.setKey(key, false)
                        c.send()
                    }
                    break
                }
                case 'reset':
                    button.reset()
                    c.send()
                    break
                case 'end':
                    console.log('end')
                    process.exit(0)
            }
        }
        console.log(line)
        rl.prompt()
    }
    let queue: Promise<void> = Promise.resolve()
    rl.prompt()
    rl
        .on('line', line => {
            queue = queue.then(() => handler(line))
        })
}

main(...process.argv.slice(2)).catch(e => console.error(e))
