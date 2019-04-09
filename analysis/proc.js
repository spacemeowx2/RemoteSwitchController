const { readFileSync, writeFileSync } = require('fs')

const s = readFileSync('./left-90-right-90.c', 'utf-8')
const re = /\{([\s\S]+?)\};/gm
let ary = []
let r = re.exec(s)
while (r) {
    ary.push(eval(`[${r[1]}]`))
    r = re.exec(s)
}
const ipt30 = ary.filter(i => i.length === 112).filter(i => i[0x30] === 0x30)
const reports = ipt30.map(i => i.slice(0x30))

const sixAxis = reports.map(i => new Int16Array(new Uint8Array(i.slice(13, 13 + 2 * 6 * 3)).buffer))

let output = ['ax,ay,az,g1,g2,g3']
for (let six of sixAxis) {
    output.push([...six.slice(0, 6)].join(','))
    output.push([...six.slice(6, 12)].join(','))
    output.push([...six.slice(12, 18)].join(','))
}
let last = ''
for (let s of output) {
    if (s === last) {
        console.log('double s')
    }
    last = s
}
writeFileSync('left-90-right-90.csv', output.join('\n'))
// [13 ... (3*6*2)]
