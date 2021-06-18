let fs = require('fs')
let jpegent = require('./jpegent.js')

let filename = 'test.jpg'
fs.readFile(filename, (err, data) => {
    if (err) { throw err }
    jpegent().then((instance) => {
        let buf = instance._malloc(data.length)
        instance.HEAPU8.set(data, buf)
        let type = ['number', 'number', 'number']
        let args = [buf, data.length, 1]
        let result = instance.ccall('jpeg_entropy', 'string', type, args)
        console.log(result)
    })
})
