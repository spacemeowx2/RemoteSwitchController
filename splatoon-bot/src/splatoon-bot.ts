import { ProController } from './controller'
import { writeFile as writeFileAsync, readFile as readFileAsync } from 'fs'
import { promisify } from 'util'
import { Image } from 'canvas'
import Rembrandt from 'rembrandt'
import { Capturer } from './capture'
import { delay } from './util'
const writeFile = promisify(writeFileAsync)
const readFile = promisify(readFileAsync)
const IgnoreColor = new Rembrandt.Color(0, 1, 0)
const TargetSize = { w: 1280, h: 720 }
enum Scenes {
    Shoal
}

export class SplatoonBot {
    constructor (private controller: ProController, private capturer: Capturer) {}
    async run () {
        //
        while (1) {
            try {
                const r = await this.capturer.getCapture(500)
                console.log(r)
            } catch (e) {
                console.error(e)
            }
            await delay(1000)
            // await this.compare()
        }
    }
    private resize (image: Buffer, size: { w: number, h: number }) {

    }
    private async compare () {
        const imageA = Rembrandt.Image.fromBuffer(await readFile('./images/1.png'))
        const imageB = Rembrandt.Image.fromBuffer(await readFile('./images/2.png'))
        for (let x = 0; x < imageA.width; x++) {
            for (let y = 0; y < imageB.width; y++) {
                if (imageA.getColorAt(x, y).equals(IgnoreColor)) {
                    imageB.setColorAt(x, y, IgnoreColor)
                }
            }
        }
        const r = await new Rembrandt({
            imageA,
            imageB,
            renderComposition: true,
            maxOffset: 4,
            maxThreshold: 0.05,
        }).compare()
        console.log(r)
        await writeFile('./images/3.png', r.compositionImage)
    }
}
