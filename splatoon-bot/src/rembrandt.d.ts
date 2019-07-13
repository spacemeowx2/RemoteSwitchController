declare module "rembrandt" {
  import { Image, ImageData, Canvas } from 'canvas'
  class RembrandtImage {
    static fromImage (image: Image): RembrandtImage
    static fromBuffer (buffer: Buffer): RembrandtImage
    constructor (width: number, height: number, image?: Image | null)
    readonly canvas: Canvas
    readonly imageData: ImageData
    toBuffer (): Buffer
    persist(): void
    clone(): RembrandtImage
    setImageData(imageData: ImageData): void
    getImageData(): ImageData
    getColorAt(x: number, y: number): RembrandtColor
    setColorAt(x: number, y: number, color: RembrandtColor): void
  }
  class RembrandtColor {
    static TRANSPARENT: RembrandtColor
    static WHITE: RembrandtColor
    static BLACK: RembrandtColor
    static RED: RembrandtColor
    constructor (r: number, g: number, b: number, a?: number)
    clone(): RembrandtColor
    equals(color: RembrandtColor): boolean
    toString(): string
  }
  export enum ThresholdType {
    THRESHOLD_PERCENT = 0,
    THRESHOLD_PIXELS = 1
  }
  export type ImageType = Buffer
  export interface Config {
    imageA: ImageType,
    imageB: ImageType,
    thresholdType?: ThresholdType,
    maxThreshold?: number,
    maxDelta?: number,
    renderComposition?: boolean,
    compositionMaskColor?: RembrandtColor,
    maxOffset?: number
  }
  export default class Rembrandt {
    static THRESHOLD_PERCENT: ThresholdType
    static THRESHOLD_PIXELS: ThresholdType
    static Color: RembrandtColor
    static RembrandtImage: RembrandtImage
    static version: string
    static createImage (width: number, height: number): RembrandtImage

    compare(): Promise<void>
  }
}
