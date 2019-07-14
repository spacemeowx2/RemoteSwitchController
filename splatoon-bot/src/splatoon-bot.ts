import { ProController, ButtonBitMap } from './controller'
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
const NoneThreshold = 0.4

enum Scenes {
    None = 'None',
    Home = 'Home',
    SelectUser = 'SelectUser',
    LoadingGame = 'LoadingGame',
    TitleScreen = 'TitleScreen',
    Shoal = 'Shoal',
    LanShoal = 'LanShoal',
    WaitPlayer = 'WaitPlayer',
    NotEnoughPlayer = 'NotEnoughPlayer',
    SelectTeam = 'SelectTeam',
    SelectWeapon = 'SelectWeapon',
    InGame = 'InGame',
    InGameError = 'InGameError',
    SearchFailed = 'SearchFailed',
    SelectPrivateConfig = 'SelectPrivateConfig',
    FatalError = 'FatalError',
    Square = 'Square',
}

export class SplatoonBot {
    private imageCache = new Map<string, RembrandtImage>()
    constructor (private controller: ProController, private capturer: Capturer) {}
    private async press (key: ButtonBitMap, time: number = 500) {
        const { button } = this.controller
        button.setKey(key, true)
        this.controller.send()
        await delay(time)
        button.setKey(key, false)
        this.controller.send()
    }
    private async lan () {
        await Promise.all([
            this.press(ButtonBitMap.L, 5000),
            this.press(ButtonBitMap.R, 5000),
            this.press(ButtonBitMap.LStick, 5000),
        ])
    }
    private async doScene (scene: Scenes) {
        const c = this.controller
        switch (scene) {
            case Scenes.None:
            case Scenes.LoadingGame:
                break
            case Scenes.Home:
            case Scenes.SelectUser:
            case Scenes.FatalError:
                await this.press(ButtonBitMap.A)
                break
            case Scenes.TitleScreen:
                await Promise.all([this.press(ButtonBitMap.ZL), this.press(ButtonBitMap.ZR)])
                await delay(5000)
            case Scenes.Square:
                await this.press(ButtonBitMap.X)
                await delay(500)
                await this.press(ButtonBitMap.Down, 100)
                await delay(500)
                await this.press(ButtonBitMap.Down, 100)
                await delay(500)
                await this.press(ButtonBitMap.A, 100)
                await delay(10000)
                await this.press(ButtonBitMap.A, 100)
                break
            case Scenes.Shoal:
                await this.lan()
                await delay(5000)
                break
            case Scenes.LanShoal:
                await this.press(ButtonBitMap.Down)
                await this.press(ButtonBitMap.A)
                await delay(500)
                await this.press(ButtonBitMap.A)
                await delay(10000)
                await this.press(ButtonBitMap.Right, 100)
                break
            case Scenes.WaitPlayer:
                await this.press(ButtonBitMap.Down)
                await this.press(ButtonBitMap.Down)
                await this.press(ButtonBitMap.A)
                break
            case Scenes.NotEnoughPlayer:
                await this.press(ButtonBitMap.A)
                break
            case Scenes.SelectTeam:
                await this.press(ButtonBitMap.Up, 100)
                await this.press(ButtonBitMap.A, 100)
                break
            case Scenes.SelectWeapon:
                await this.press(ButtonBitMap.A, 100)
                await delay(5000)
                break
            case Scenes.InGameError:
                await this.press(ButtonBitMap.A, 100)
                await delay(3000)
                break
            case Scenes.SelectPrivateConfig:
                await this.press(ButtonBitMap.Left)
                await this.press(ButtonBitMap.A)
                await delay(1000)
                break
            case Scenes.SearchFailed:
                await this.press(ButtonBitMap.B, 100)
                await this.press(ButtonBitMap.A, 100)
                break
            case Scenes.InGame:
                await this.press(ButtonBitMap.Y, 50)
                await delay(100)
                await this.press(ButtonBitMap.R, 50)
                c.leftStick.y = 1
                c.send()
                c.button.setKey(ButtonBitMap.ZL, true)
                await delay(3000)

                for (let i = 0; i < 10; i++) {
                    await this.press(ButtonBitMap.R, 50)
                    await delay(100)
                    await this.press(ButtonBitMap.RStick, 50)
                    await delay(900)
                }

                c.button.setKey(ButtonBitMap.ZL, false)
                c.leftStick.y = 0.5
                c.send()

                // c.button.setKey(ButtonBitMap.X, true)
                // c.send()
                // await delay(50)
                // c.button.setKey(ButtonBitMap.Down, true)
                // c.send()
                // await delay(50)
                // await this.press(ButtonBitMap.A, 50)
                // c.button.setKey(ButtonBitMap.X, false)
                // c.button.setKey(ButtonBitMap.Down, false)
                // c.send()

                break
        }
    }
    private async reset () {
        await this.press(ButtonBitMap.Home, 100)
        await delay(500)
        await this.press(ButtonBitMap.X, 100)
        await delay(500)
        await this.press(ButtonBitMap.A, 100)
        await delay(500)
    }
    async run () {
        let noneCount = 0
        while (1) {
            await delay(1000)
            try {
                const image = await this.capturer.getCapture(500, TargetSize)
                const tops = await this.getTopScene(image)
                let scene: Scenes = Scenes.None
                if (tops[0].score <= NoneThreshold) {
                    scene = tops[0].scene
                }
                console.log(tops.slice(0, 3), scene)
                if (scene === Scenes.None) {
                    noneCount++
                } else {
                    noneCount = 0
                }
                if (noneCount > 10) {
                    await this.reset()
                    noneCount = 0
                }
                await this.doScene(scene)
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
            .filter(i => i !== 'None')
            .map(name => (async () => {
                const img = await this.getImage(name)
                const r = await this.compare(Rembrandt.Image.fromBuffer(imageBuf), img)
                const ret: [string, CompareResult] = [name, r]
                return ret
            })())
        let tops = await Promise.all(promises)
        tops = tops.sort((a, b) => a[1].threshold - b[1].threshold)
        await writeFile('./images/screenshot.png', imageBuf)
        await writeFile('./images/composition.png', tops[0][1].compositionImage)
        return tops.map(([name, result]): { scene: Scenes, score: number } => ({
            scene: name as any,
            score: result.threshold
        }))
    }
    private async compare (imageA: RembrandtImage, imageB: RembrandtImage) {
        // const imageA = Rembrandt.Image.fromBuffer(imageBBuf)
        // const imageB = Rembrandt.Image.fromBuffer(await readFile('./images/1.png'))
        let ignoredPixel = 0
        for (let x = 0; x < imageA.width; x++) {
            for (let y = 0; y < imageB.width; y++) {
                if (imageB.getColorAt(x, y).equals(IgnoreColor)) {
                    imageA.setColorAt(x, y, IgnoreColor)
                    ignoredPixel += 1
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
        r.threshold = r.differences / (imageB.width * imageB.height - ignoredPixel)
        return r
    }
}
