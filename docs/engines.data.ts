// Declared engine facts for the support matrix: read from firmware/engines/*/engine.json
// at build time, so the page never holds hand-edited copies.
import fs from 'node:fs'
import path from 'node:path'

export interface EngineFacts {
  name: string
  upstream: string | null
  language: string | null
  language_level?: string
  license: string | null
  unsupported_boards: string[]
  min_flash_kb: number | null
  min_ram_kb: number | null
  features: Record<string, boolean>
}

declare const data: EngineFacts[]
export { data }

const dir = path.resolve(__dirname, '../firmware/engines')

// Column order; engines not listed here follow, alphabetically.
const order = ['quickjs', 'mquickjs', 'jerryscript', 'duktape', 'mjs', 'ant']
const rank = (n: string) => (order.includes(n) ? order.indexOf(n) : order.length)

export default {
  watch: [path.join(dir, '*/engine.json')],
  load(): EngineFacts[] {
    return fs
      .readdirSync(dir)
      .filter((d) => fs.existsSync(path.join(dir, d, 'engine.json')) && d !== 'none')
      .sort((a, b) => rank(a) - rank(b) || a.localeCompare(b))
      .map((d) => JSON.parse(fs.readFileSync(path.join(dir, d, 'engine.json'), 'utf8')))
  },
}
