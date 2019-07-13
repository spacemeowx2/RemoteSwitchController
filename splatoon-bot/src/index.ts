import { ProUDPTransport, ProController } from './controller'
import { SplatoonBot } from './splatoon-bot'
import { ChromeCapturer } from './capture'

async function main (...argv: string[]) {
    if (argv.length === 0) {
        argv[0] = 'localhost:34952'
        console.warn('use default localhost server:', argv[0])
    } else if (argv.length != 1) {
        console.error(`npm start UDPServer:Port`)
        return
    }
    const c = new ProController(new ProUDPTransport(argv[0]))
    const capturer = new ChromeCapturer()
    const bot = new SplatoonBot(c, capturer)
    await bot.run()
}
main(...process.argv.slice(2)).catch(e => console.error(e))
