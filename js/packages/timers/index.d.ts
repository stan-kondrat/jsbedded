/**
 * Timer globals, run by the firmware event loop on the board's millisecond
 * clock. Callbacks run one at a time, after the script and after each other;
 * an exception in a callback is logged and the loop continues. The script
 * "finishes" when no timer is left.
 *
 * Limits: at most 16 timers at once; arguments after `ms` are not passed to
 * the callback.
 *
 * Available on: mquickjs.
 *
 * @packageDocumentation
 */

/**
 * Call `callback` once, after at least `ms` milliseconds.
 *
 * @param callback - function to call; must be a function (TypeError otherwise)
 * @param ms - delay in milliseconds; missing, negative or NaN means 0
 * @returns timer id for {@link clearTimeout}
 * @throws RangeError when 16 timers are already pending
 */
declare function setTimeout(callback: () => void, ms?: number): number;

/**
 * Call `callback` every `ms` milliseconds until {@link clearInterval}.
 * Missed ticks are skipped, not caught up.
 *
 * @param callback - function to call
 * @param ms - period in milliseconds; 0 or less means 1
 * @returns timer id for {@link clearInterval}
 * @throws RangeError when 16 timers are already pending
 *
 * @example
 * var n = 0;
 * var id = setInterval(function () {
 *     console.log("tick", ++n);
 *     if (n === 3) clearInterval(id);
 * }, 1000);
 */
declare function setInterval(callback: () => void, ms?: number): number;

/**
 * Cancel a timer from {@link setTimeout}. Unknown or finished ids are ignored.
 *
 * @param id - timer id
 */
declare function clearTimeout(id?: number): void;

/**
 * Cancel a timer from {@link setInterval}. Unknown ids are ignored.
 * A callback may clear its own interval.
 *
 * @param id - timer id
 */
declare function clearInterval(id?: number): void;
