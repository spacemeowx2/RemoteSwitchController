import { ProController } from './controller'
import { writeFile as writeFileAsync, readFile as readFileAsync } from 'fs'
import { promisify } from 'util'
import { Image, Canvas } from 'canvas'
import Rembrandt, { RembrandtImage, ImageType, CompareResult } from 'rembrandt'
import { Capturer } from './capture'
import { delay } from './util'
const writeFile = promisify(writeFileAsync)
const readFile = promisify(readFileAsync)
const IgnoreColor = new Rembrandt.Color(0, 1, 0)
const TargetSize = { w: 320, h: 180 }

enum Scenes {
    Shoal,
    LanShoal,
}

export class SplatoonBot {
    private imageCache = new Map<string, RembrandtImage>()
    constructor (private controller: ProController, private capturer: Capturer) {}
    async run () {
        //
        while (1) {
            await delay(1000)
            try {
                const image = await this.capturer.getCapture(500, TargetSize)
                const tops = await this.getTopScene(image)
                console.log(tops)
            } catch (e) {
                console.error(e)
            }
            // await this.compare()
        }
    }
    private async getImage (name: string) {
        let buf = this.imageCache.get(name)
        if (buf) return buf
        buf = Rembrandt.Image.fromBuffer(await readFile(`./images/${name}.png`))
        this.imageCache.set(name, buf)
        return buf
    }
    private async getTopScene (imageBuf: Buffer) {
        let promises: Promise<[string, CompareResult]>[] = Object.values(Scenes)
            .filter(i => typeof i === 'string')
            .map(name => (async () => {
                const img = await this.getImage(name)
                const r = await this.compare(Rembrandt.Image.fromBuffer(imageBuf), img)
                const ret: [string, CompareResult] = [name, r]
                return ret
            })())
        let tops = await Promise.all(promises)
        tops = tops.sort((a, b) => a[1].threshold - b[1].threshold)
        await writeFile('./images/screenshot.png', tops[0][1].compositionImage)
        return tops.map(([name, result]): { name: Scenes, score: number } => ({
            name: name as any,
            score: result.threshold
        }))
    }
    private async compare (imageA: RembrandtImage, imageB: RembrandtImage) {
        // const imageA = Rembrandt.Image.fromBuffer(imageBBuf)
        // const imageB = Rembrandt.Image.fromBuffer(await readFile('./images/1.png'))
        for (let x = 0; x < imageA.width; x++) {
            for (let y = 0; y < imageB.width; y++) {
                if (imageB.getColorAt(x, y).equals(IgnoreColor)) {
                    imageA.setColorAt(x, y, IgnoreColor)
                }
            }
        }
        const r = await new Rembrandt({
            imageA,
            imageB,
            renderComposition: true,
            maxThreshold: 0.1,
            maxDelta: 0.05
        }).compare()
        return r
    }
}
