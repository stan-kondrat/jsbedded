/**
 * The `board` global: facts about the board the firmware image was built for
 * (from `firmware/boards/<board>/board.json`).
 *
 * Available on: mquickjs.
 *
 * @packageDocumentation
 */

declare namespace board {
    /** Board name as in `make firmware BOARD=`, e.g. `"pico"`, `"esp32s3_devkitc"`. */
    const name: string;

    /**
     * GPIO number of the on-board LED: 25 on Pico and Pico 2. `undefined`
     * where the LED is not a plain GPIO: on Pico W / Pico 2 W it sits on the
     * CYW43 Wi-Fi chip, on the ESP32 dev boards it is an addressable RGB LED.
     *
     * @example
     * if (board.LED !== undefined) gpio.mode(board.LED, "output");
     */
    const LED: number | undefined;
}
