import { h } from 'vue'
import DefaultTheme from 'vitepress/theme'
import './custom.css'
import EngineFacts from './EngineFacts.vue'
import ArchitectureDiagram from './ArchitectureDiagram.vue'

// Same wordmark as the landing page: "JS" + accent-colored "Bedded".
// The nav title is a link (themeConfig.logoLink) to the landing page.
export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('EngineFacts', EngineFacts)
    app.component('ArchitectureDiagram', ArchitectureDiagram)
  },
  Layout: () =>
    h(DefaultTheme.Layout, null, {
      'nav-bar-title-before': () =>
        h('span', { class: 'jsb-logo' }, ['JS', h('b', 'Bedded')]),
    }),
}
