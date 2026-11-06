import { defineConfig } from 'vitepress'

// Site layout on GitHub Pages: /jsbedded/ = landing (site/), /jsbedded/docs/ = this.
export default defineConfig({
  title: 'JSBedded',
  description: 'JavaScript and TypeScript on microcontrollers',
  base: '/jsbedded/docs/',
  outDir: '../build/site/docs',
  cleanUrls: true,
  lastUpdated: true,
  srcExclude: ['README.md'],
  // engine pages link to READMEs elsewhere in the repo (outside docs/)
  ignoreDeadLinks: [/firmware\/engines\/.*\/README/],
  themeConfig: {
    // Wordmark is rendered by the theme (see theme/index.ts); clicking it goes to the landing page.
    siteTitle: false,
    logoLink: { link: '/jsbedded/', target: '_self' }, // target makes the router do a full page load
    nav: [
      { text: 'Guide', link: '/guide/getting-started' },
      { text: 'API', link: '/api/' },
      { text: 'Boards', link: '/boards/' },
      { text: 'Engines', link: '/engines/' },
      { text: 'Matrix', link: '/matrix' },
    ],

    // Left: one sidebar for every page, always visible.
    sidebar: [
      {
        text: 'Introduction',
        items: [
          { text: 'Overview', link: '/' },
          { text: 'Architecture', link: '/architecture' },
          { text: 'Support matrix', link: '/matrix' },
        ],
      },
      {
        text: 'Guide',
        items: [
          { text: 'Getting started', link: '/guide/getting-started' },
          { text: 'Install firmware', link: '/guide/install-firmware' },
          { text: 'First script', link: '/guide/first-script' },
          { text: 'Recovery', link: '/guide/recovery' },
        ],
      },
      {
        text: 'Reference',
        items: [
          { text: 'API', link: '/api/' },
          { text: 'Device protocol', link: '/protocol/' },
        ],
      },
      {
        text: 'Boards',
        collapsed: false,
        items: [
          { text: 'Overview', link: '/boards/' },
          { text: 'Pico', link: '/boards/pico' },
          { text: 'Pico W', link: '/boards/pico_w' },
          { text: 'Pico 2', link: '/boards/pico2' },
          { text: 'Pico 2 W', link: '/boards/pico2_w' },
          { text: 'ESP32-C3 DevKitM', link: '/boards/esp32c3_devkitm' },
          { text: 'ESP32-S3 DevKitC', link: '/boards/esp32s3_devkitc' },
        ],
      },
      {
        text: 'Engines',
        collapsed: false,
        items: [
          { text: 'Overview', link: '/engines/' },
          { text: 'mquickjs', link: '/engines/mquickjs' },
          { text: 'QuickJS', link: '/engines/quickjs' },
          { text: 'Duktape', link: '/engines/duktape' },
          { text: 'JerryScript', link: '/engines/jerryscript' },
          { text: 'Ant', link: '/engines/ant' },
        ],
      },
      {
        text: 'Porting',
        collapsed: false,
        items: [
          { text: 'Overview', link: '/porting/' },
          { text: 'Add a board', link: '/porting/add-board' },
          { text: 'Add an engine', link: '/porting/add-engine' },
          { text: 'Add a platform', link: '/porting/add-platform' },
        ],
      },
    ],

    // Right: sticky "On this page" outline (h2-h3) that highlights the current section.
    outline: { level: [2, 3], label: 'On this page' },

    search: { provider: 'local' },
    editLink: {
      pattern: 'https://github.com/stan-kondrat/jsbedded/edit/main/docs/:path',
      text: 'Edit this page on GitHub',
    },
    docFooter: { prev: 'Previous', next: 'Next' },
    socialLinks: [{ icon: 'github', link: 'https://github.com/stan-kondrat/jsbedded' }],
  },
})
