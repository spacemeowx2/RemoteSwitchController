import { server as WebSocketServer, connection as WebSocketConnection } from 'websocket'
import { createServer } from 'http'
import { parse } from 'url'
import { createReadStream } from 'fs'
import { normalize, join } from 'path'

export interface Capturer {
    getCapture (timeout: number): Promise<Buffer>
}

const SizeLimit = 10 * 1024 * 1024 // 10MB

enum CapturerState {
    None,
    Ready,
    Busy,
}
export class ChromeCapturer implements Capturer {
    state: CapturerState = CapturerState.None
    httpServer = createServer((request, response) => {
        if (!request.url) {
            response.writeHead(404)
            return response.end()
        }
        let pathName = parse(request.url).pathname!
        if (!pathName) {
            response.writeHead(404)
            return response.end()
        }
        pathName = pathName.replace(/\.\./g, '')
        if (pathName === '/') {
            pathName = 'index.html'
        }
        pathName = normalize(join(__dirname, '..', 'static', pathName))
        const s = createReadStream(pathName)
        s.pipe(response)
        s.on('error', e => {
            console.log('error', e)
            response.writeHead(404)
            return response.end()
        })
    })
    wsServer: WebSocketServer = new WebSocketServer({
        httpServer: this.httpServer,
        autoAcceptConnections: true,
        maxReceivedFrameSize: SizeLimit,
        maxReceivedMessageSize: SizeLimit
    })
    conn?: WebSocketConnection
    resolver?: Function
    
    constructor () {
        const port = 41919
        this.wsServer.on('connect', (conn) => {
            if (this.conn) {
                // this.conn.close()
            }
            this.conn = conn
            conn.on('message', data => {
                console.log(data)
                if (this.resolver) this.resolver(data.binaryData)
            })
        })
        this.httpServer.listen(port, () => {
            console.log(`listening at ${port}`)
        })
    }
    async getCapture (timeout: number): Promise<Buffer> {
        if (!this.conn) {
            throw new Error('not ready')
        }
        this.conn.send(JSON.stringify({ cmd: 'capture' }))
        return new Promise((res, rej) => {
            this.resolver = res
            setTimeout(() => rej(new Error('timeout')), timeout)
        })
    }
}
