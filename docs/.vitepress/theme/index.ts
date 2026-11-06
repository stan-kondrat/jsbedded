import { h } from 'vue'
import DefaultTheme from 'vitepress/theme'
import './custom.css'

// Same wordmark as the landing page: "JS" + accent-colored "Bedded".
// The nav title is a link (themeConfig.logoLink) to the landing page.
export default {
  extends: DefaultTheme,
  Layout: () =>
    h(DefaultTheme.Layout, null, {
      'nav-bar-title-before': () =>
        h('span', { class: 'jsb-logo' }, ['JS', h('b', 'Bedded')]),
    }),
}
