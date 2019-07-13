import { performance } from 'perf_hooks'

function hrTimeout(fn: Function, ms: number) {
    const start = performance.now()
    const end = start + ms
    const toSleep = ms - 15
    const busy = () => {
        const now = performance.now()
        const dist = end - now
        if (dist <= 0) {
            fn()
        } else {
            setImmediate(busy)
        }
    }
    if (toSleep > 0) {
        setTimeout(busy, toSleep)
    } else {
        busy()
    }
}

export function delay (ms: number) {
    return new Promise<void>(r => hrTimeout(r, ms))
}
