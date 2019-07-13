const Width = 1920
const Height = 1080

class App {
    constructor () {
        this.initWs()
    }
    initWs () {
        if (this.ws && this.ws.readyState !== this.ws.OPEN) {
            this.ws.close()
        }
        const ws = new WebSocket(`ws://${location.host}/`)
        ws.onclose = () => {
            if (ws === this.ws) {
                this.initWs()
            }
        }
        ws.onmessage = ({data}) => this.onMessage(JSON.parse(data))
        this.ws = ws
    }
    onMessage (dat) {
        if (dat.cmd === 'capture') {
            const video = document.querySelector('video')
            const canvas = document.querySelector('canvas')
            canvas.width = video.videoWidth
            canvas.height = video.videoHeight
            const ctx = canvas.getContext('2d')
            ctx.drawImage(video, 0, 0)
            canvas.toBlob(async (blob) => {
                const ab = await new Response(blob).arrayBuffer()
                this.ws.send(ab)
            })
        }
    }
}
async function main () {
    const app = new App()
    const video = document.querySelector('video')
    // const canvas = document.querySelector('canvas')
    let devs = await navigator.mediaDevices.enumerateDevices()
    devs = devs.filter(i => i.kind === 'videoinput')
    console.log(devs)
    const stream = await navigator.mediaDevices.getUserMedia({
        video: {
            advanced: [{
                deviceId: {
                    exact: devs[0].deviceId
                },
                width: 1920,
                height: 1080,
                frameRate: 60
            }]
        }
    })
    video.srcObject = stream
    // const ctx = canvas.getContext('2d')
    // ctx.drawImage(video, 0, 0)
}
main()
