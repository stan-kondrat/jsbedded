<script setup lang="ts">
import { data as engines } from '../../engines.data'

// Feature rows: key in engine.json "features" -> label. A missing key is "not declared".
const features: [string, string][] = [
  ['let', 'let'],
  ['closures', 'Closures'],
  ['exceptions', 'try / catch'],
  ['classes', 'Classes'],
  ['arrow_functions', 'Arrow functions'],
  ['promises', 'Promises'],
  ['json', 'JSON'],
  ['stdlib_objects', 'String, Number, Date, RegExp'],
  ['typed_arrays', 'Typed arrays'],
  ['host_functions', 'Host functions (timers, gpio, board)'],
]
const boards = 6
const mark = (v: boolean | undefined) => (v === true ? '✓' : v === false ? '✗' : '–')
const cls = (v: boolean | undefined) => (v === true ? 'yes' : v === false ? 'no' : 'nd')
const boardsText = (u: string[]) => (u.length >= boards ? 'none' : `${boards - u.length}/${boards}`)
const licenseText = (l: string | null) => (l ? l.replace(' OR ', ' / ') : '–')
</script>

<template>
  <div class="engine-facts">
    <table>
      <thead>
        <tr>
          <th></th>
          <th v-for="e in engines" :key="e.name">{{ e.name }}</th>
        </tr>
      </thead>
      <tbody>
        <tr class="group"><th :colspan="engines.length + 1">Language</th></tr>
        <tr>
          <td>Level</td>
          <td v-for="e in engines" :key="e.name" :title="e.language ?? undefined" :class="{ help: e.language }">
            {{ e.language_level ?? '–' }}
          </td>
        </tr>
        <tr v-for="[key, label] in features.slice(0, 6)" :key="key">
          <td>{{ label }}</td>
          <td v-for="e in engines" :key="e.name" :class="cls(e.features[key])">{{ mark(e.features[key]) }}</td>
        </tr>

        <tr class="group"><th :colspan="engines.length + 1">Library and host</th></tr>
        <tr v-for="[key, label] in features.slice(6)" :key="key">
          <td>{{ label }}</td>
          <td v-for="e in engines" :key="e.name" :class="cls(e.features[key])">{{ mark(e.features[key]) }}</td>
        </tr>

        <tr class="group"><th :colspan="engines.length + 1">Project</th></tr>
        <tr>
          <td>Boards</td>
          <td v-for="e in engines" :key="e.name">{{ boardsText(e.unsupported_boards) }}</td>
        </tr>
        <tr>
          <td>Flash / RAM (KB)</td>
          <td v-for="e in engines" :key="e.name">{{ e.min_flash_kb ?? '?' }} / {{ e.min_ram_kb ?? '?' }}</td>
        </tr>
        <tr>
          <td>License</td>
          <td v-for="e in engines" :key="e.name" class="license">{{ licenseText(e.license) }}</td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<style scoped>
.engine-facts { overflow-x: auto; margin: 16px 0; }
table { display: table; width: 100%; font-size: 0.875em; }
th, td { padding: 4px 10px; white-space: nowrap; text-align: center; }
th:first-child, td:first-child { text-align: left; }
tr.group th {
  text-align: left;
  font-size: 0.75em;
  letter-spacing: 0.04em;
  text-transform: uppercase;
  color: var(--vp-c-text-2);
  padding-top: 12px;
}
td.help { cursor: help; text-decoration: underline dotted var(--vp-c-text-3); }
td.license { white-space: normal; font-size: 0.85em; min-width: 7em; }
td.yes { color: var(--vp-c-green-1); font-weight: 700; }
td.no { color: var(--vp-c-red-1); font-weight: 700; }
td.nd { color: var(--vp-c-text-3); }
</style>
