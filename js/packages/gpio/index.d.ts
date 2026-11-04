/**
 * The `gpio` global: digital pins by MCU GPIO number (Pico `GP25` is `25`).
 *
 * Available on: mquickjs, every board.
 *
 * @packageDocumentation
 */

declare namespace gpio {
    /** Pin direction. */
    type Mode = "input" | "output";

    /** Pin level: 0 low, 1 high. */
    type Level = 0 | 1;

    /**
     * Configure a pin as input (floating, no pull-up or pull-down) or output.
     *
     * @param pin - GPIO number
     * @param mode - `"input"` or `"output"`
     * @throws TypeError if `pin` is not a number or `mode` not a string
     * @throws RangeError if the pin doesn't exist on this board or the mode is unknown
     *
     * @example
     * gpio.mode(board.LED, "output");
     */
    function mode(pin: number, mode: Mode): void;

    /**
     * Drive an output pin. `true` and `false` count as 1 and 0.
     *
     * @param pin - GPIO number, configured with {@link gpio.mode} as `"output"`
     * @param value - 0 (low) or 1 (high)
     * @throws RangeError if the pin doesn't exist or the value is not 0 or 1
     */
    function write(pin: number, value: Level | boolean): void;

    /**
     * Read a pin's level. On an output pin this is the driven level.
     *
     * @param pin - GPIO number
     * @returns 0 (low) or 1 (high)
     * @throws RangeError if the pin doesn't exist on this board
     */
    function read(pin: number): Level;
}
