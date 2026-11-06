import { defineConfig } from 'vitepress'

// Site layout on GitHub Pages: /jsbedded/ = landing (site/), /jsbedded/docs/ = this.
export default defineConfig({
  title: 'JSBedded',
  description: 'JavaScript and TypeScript on microcontrollers',
  base: '/jsbedded/docs/',
  outDir: '../build/site/docs',
  cleanUrls: true,
  srcExclude: ['README.md'],
  // engine pages link to READMEs elsewhere in the repo (outside docs/)
  ignoreDeadLinks: [/firmware\/engines\/.*\/README/],
  themeConfig: {
    nav: [
      { text: 'Guide', link: '/guide/getting-started' },
      { text: 'API', link: '/api/' },
      { text: 'Boards', link: '/boards/' },
      { text: 'Engines', link: '/engines/' },
    ],
    socialLinks: [{ icon: 'github', link: 'https://github.com/stan-kondrat/jsbedded' }],
  },
})
