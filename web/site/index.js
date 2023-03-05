
import { render } from 'https://unpkg.com/preact@latest?module';
import html from './edge-web.js'
import { App } from "./app.js"

render(html`<${App} name="World" />`, document.body)

