// Serve build/site under /jsbedded/ like GitHub Pages (landing at /, docs at /docs/).
// usage: node scripts/docs-serve.mjs [port]   (binds 0.0.0.0)
import { createServer } from 'node:http'
import { readFile, stat } from 'node:fs/promises'
import { extname, join, normalize } from 'node:path'
import { fileURLToPath } from 'node:url'

const root = fileURLToPath(new URL('../build/site/', import.meta.url))
const port = Number(process.argv[2] || 8080)
const prefix = '/jsbedded'
const types = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.css': 'text/css',
  '.json': 'application/json', '.svg': 'image/svg+xml', '.png': 'image/png',
  '.woff2': 'font/woff2', '.ico': 'image/x-icon', '.txt': 'text/plain',
}

async function resolveFile(p) {
  for (const c of [p, p + '.html', join(p, 'index.html')]) {
    try { if ((await stat(c)).isFile()) return c } catch {}
  }
}

createServer(async (req, res) => {
  const url = new URL(req.url, 'http://x')
  let path = decodeURIComponent(url.pathname)
  if (path === prefix) { res.writeHead(301, { Location: prefix + '/' }).end(); return }
  if (!path.startsWith(prefix + '/')) { res.writeHead(404).end('not found (site lives under /jsbedded/)'); return }
  const rel = normalize(path.slice(prefix.length)).replace(/^(\.\.[/\\])+/, '')
  const file = await resolveFile(join(root, rel))
  if (!file) { res.writeHead(404, { 'Content-Type': 'text/plain' }).end('404'); return }
  res.writeHead(200, { 'Content-Type': types[extname(file)] || 'application/octet-stream' })
  res.end(await readFile(file))
}).listen(port, '0.0.0.0', () => console.log(`http://0.0.0.0:${port}/jsbedded/  and  /jsbedded/docs/`))
