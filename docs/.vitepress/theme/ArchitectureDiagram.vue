<script setup lang="ts">
// Layer diagram for docs/architecture.md. Inline SVG so it follows the site's light/dark theme.
const engines = ['quickjs', 'mquickjs', 'jerryscript', 'duktape', 'mjs', 'ant']
const chipX = (i: number) => 160 + i * 84

// Layers on the board, top to bottom: [title, description, y].
const layers: [string, string, number][] = [
  ['Hardware APIs', 'console · timers · gpio · …', 136],
  ['HAL', 'pico-sdk · esp-idf', 256],
  ['Board', 'pin map · memory · peripherals', 316],
]
</script>

<template>
  <figure class="arch">
    <svg viewBox="0 0 720 400" role="img" aria-labelledby="arch-title arch-desc" xmlns="http://www.w3.org/2000/svg">
      <title id="arch-title">JSBedded architecture</title>
      <desc id="arch-desc">
        Your script goes to the board over USB. On the board the hardware APIs run on one JavaScript
        engine, which runs on the HAL of the board's SDK.
      </desc>
      <defs>
        <marker id="arch-head" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="7" markerHeight="7" orient="auto">
          <path d="M1 1 L9 5 L1 9 z" class="head" />
        </marker>
      </defs>

      <!-- Your computer -->
      <rect class="box" x="48" y="12" width="624" height="48" rx="10" />
      <text class="title" x="68" y="41">Your script</text>
      <text class="sub" x="196" y="41">main.js · TypeScript typings</text>

      <line class="arrow" x1="360" y1="64" x2="360" y2="132" marker-end="url(#arch-head)" />
      <text class="note" x="374" y="95">USB drive</text>

      <!-- On the board -->
      <rect class="zone" x="28" y="104" width="664" height="284" rx="16" />
      <text class="zone-label" x="48" y="126" dy="-6">ON THE BOARD</text>

      <g v-for="[title, sub, y] in layers" :key="title">
        <rect class="box" x="48" :y="y" width="624" height="48" rx="10" />
        <text class="title" x="68" :y="y + 29">{{ title }}</text>
        <text class="sub" x="196" :y="y + 29">{{ sub }}</text>
      </g>

      <!-- Engine: one per image -->
      <rect class="box accent" x="48" y="196" width="624" height="48" rx="10" />
      <text class="title" x="68" y="225">Engine</text>
      <g v-for="(name, i) in engines" :key="name">
        <rect class="chip" :x="chipX(i)" y="207" width="78" height="26" rx="13" />
        <text class="chip-text" :x="chipX(i) + 39" y="224" text-anchor="middle">{{ name }}</text>
      </g>
    </svg>
  </figure>
</template>

<style scoped>
.arch { margin: 24px 0; }
svg { display: block; width: 100%; height: auto; max-width: 720px; margin: 0 auto; font-family: var(--vp-font-family-base); }

.box { fill: var(--vp-c-bg-soft); stroke: var(--vp-c-divider); stroke-width: 1.2; }
.box.accent { fill: var(--vp-c-brand-soft); stroke: var(--vp-c-brand-3); }
.zone { fill: none; stroke: var(--vp-c-divider); stroke-width: 1.2; stroke-dasharray: 5 5; }
.zone-label { font-size: 11px; letter-spacing: 0.08em; font-weight: 600; fill: var(--vp-c-text-3); }

.title { font-size: 15px; font-weight: 650; fill: var(--vp-c-text-1); }
.sub { font-size: 13px; fill: var(--vp-c-text-2); }
.note { font-size: 12px; fill: var(--vp-c-text-3); }

.chip { fill: var(--vp-c-bg); stroke: var(--vp-c-divider); stroke-width: 1; }
.chip-text { font-size: 11.5px; font-family: var(--vp-font-family-mono); fill: var(--vp-c-text-1); }

.arrow { stroke: var(--vp-c-text-3); stroke-width: 1.6; }
.head { fill: var(--vp-c-text-3); }
</style>
