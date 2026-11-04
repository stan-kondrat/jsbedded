# api/timers

Native implementation of the `timers` module. Spec: [`js/packages/timers/index.d.ts`](../../../js/packages/timers/index.d.ts) (`@jsbedded/timers`),
rendered at `docs/api/timers.md`.

The timer queue and the bindings live in core, next to the event loop that runs
them: [`core/src/timers.c`](../../core/src/timers.c). 16 timers at once.
