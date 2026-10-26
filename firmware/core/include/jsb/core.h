/* Engine-neutral runtime entry point, called by the platform after reset. */
#ifndef JSB_CORE_H
#define JSB_CORE_H

/* Run the firmware main loop; never returns. */
void jsb_main(void) __attribute__((noreturn));

#endif
