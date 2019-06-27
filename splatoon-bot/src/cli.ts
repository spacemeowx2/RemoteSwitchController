import { createInterface } from 'readline'
import { ProController, ProUDPTransport, ButtonBitMap, PadButton } from './controller'
import { delay } from './util'

const DefaultPressDelay = 100

function keyPresser(controller: ProController) {
    const { button } = controller
    return async(key: ButtonBitMap, delayTime = DefaultPressDelay) => {
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
        output: process.stdout,
        prompt: '>>> '
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
    rl.prompt()
    rl.on('line', async (line) => {
        switch (line.trim()) {
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
            case 'w':
                await press(ButtonBitMap.Up)
                break
            case 's':
                await press(ButtonBitMap.Down)
                break
            case 'a':
                await press(ButtonBitMap.Left)
                break
            case 'd':
                await press(ButtonBitMap.Right)
                break
            case 'h':
            case 'home':
                await press(ButtonBitMap.Home)
                break
            case 'A':
                await press(ButtonBitMap.A)
                break
            case 'b':
            case 'B':
                await press(ButtonBitMap.B)
                break
            case 'x':
                await press(ButtonBitMap.X)
                break
            case 'y':
                await press(ButtonBitMap.Y)
                break
            case 'zlr':
                await Promise.all([
                    press(ButtonBitMap.ZL),
                    press(ButtonBitMap.ZR),
                ])
                break
            case 'zl':
                await press(ButtonBitMap.ZL)
                break
            case 'zl':
                await press(ButtonBitMap.ZL)
                break
            case 'zr':
                await press(ButtonBitMap.ZR)
                break
            case 'l':
                await press(ButtonBitMap.L)
                break
            case 'r':
                await press(ButtonBitMap.R)
                break
            case '+':
                await press(ButtonBitMap.Plus)
                break
            case '-':
                await press(ButtonBitMap.Minus)
                break
            case 'c':
            case 'capture':
                await press(ButtonBitMap.Capture)
                break
        }
        rl.prompt()
    }).on('close', () => {
        console.log('bye')
        process.exit()
    })
}

main(...process.argv.slice(2)).catch(e => console.error(e))
