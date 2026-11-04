/**
 * The `console` global: text output on the device console (USB serial on
 * Pico boards, UART0 on ESP32 dev boards), one line per call with `\r\n`.
 *
 * Available on: mquickjs. Other engines have their own `console.log`.
 *
 * @packageDocumentation
 */

/** Device console. */
interface Console {
    /**
     * Write the arguments separated by spaces, then a line break.
     * Strings are written as-is; other values in the engine's inspect format
     * (`{ a: 1 }`, `[ 1, 2 ]`).
     *
     * @example
     * console.log("temperature", 21.5); // temperature 21.5
     */
    log(...args: unknown[]): void;
}

/** Device console. */
declare var console: Console;
