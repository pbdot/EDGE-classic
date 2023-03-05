//import { h, Component, render } from 'https://unpkg.com/preact@latest?module';
//import { useState, useEffect } from 'https://unpkg.com/preact@latest/hooks/dist/hooks.module.js?module';
import html from './edge-web.js'


window.onerror = function (event) {
    console.log("onerror: " + event.message);
};

function Overlay(props) {
    return html`
    <div id="game-overlay" style="position:absolute;width:1280px;height:720px">
        <div style="color:white;text-align:center;vertical-align:middle">Loading</div>
    </div>
    `;
}

function Game(props) {
    return html`
    <div style="position:relative">
        <${Overlay}/>
        <canvas id="canvas" width="1280px" height="720px" oncontextmenu="event.preventDefault()"></canvas>        
    </div>
    `;
}

function Loader(props) {
    return html`
    <div>        
        <script type='text/javascript'>
            var Module = {
                edgePostInit: () => {
                    console.log("post init!");
                },
                preEdgeSyncFS: () => {
                },
                postEdgeSyncFS: () => {
                },
                arguments: ["-windowed", "-width", "1280", "-height", "720", "-iwad", "DOOM2.WAD"],
                preInit: () => {
                },
                preRun: [],
                postRun: [],
                print: (function () {
                    return function (text) {
                        text = Array.prototype.slice.call(arguments).join(' ');
                        console.log(text);
                    };
                })(),
                printErr: function (text) {
                    text = Array.prototype.slice.call(arguments).join(' ');
                    console.error(text);
                },
                canvas: (function () {
                    var canvas = document.querySelector('#canvas');
                    if (canvas) {
                    // As a default initial behavior, pop up an alert when webgl context is lost. To make your
                    // application robust, you may want to override this behavior before shipping!
                    // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
                    canvas.addEventListener("webglcontextlost", function (e) { alert('FIXME: WebGL context lost, please reload the page'); e.preventDefault(); }, false);
                    }            
                    return canvas;
                })(),
                setStatus: function (text) { },
                monitorRunDependencies: function (left) { console.log("left " + left) },
            };    
        </script>
        <script async src=edge-classic.js></script>
        <script src=edge-classic-data.js></script>
    </div>
    `;
}

export function App(props) {
    return html`
        <div>
        <${Game} />
        <${Loader} />
        </div>
    `;
}