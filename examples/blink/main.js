"use strict";
// Blink the on-board LED every 500 ms and log each toggle.
// main.js is generated from main.ts (cd js && npm run examples); edit main.ts.
var led = board.LED;
var on = false;
if (led === undefined) {
    console.log("blink: board " + board.name + " has no plain-GPIO LED (board.LED is undefined)");
}
else {
    gpio.mode(led, "output");
    setInterval(function () {
        on = !on;
        gpio.write(led, on ? 1 : 0);
        console.log(on ? "LED on" : "LED off");
    }, 500);
}
