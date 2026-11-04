/*
 * Global `board`: facts about the board this image was built for, from
 * boards/<board>/board.mk. board.LED is defined only where the on-board LED is
 * a plain GPIO (JSB_BOARD_LED); on the W and ESP32 boards it is undefined.
 */
#include "jsb/core.h"
#include "jsb/engine.h"
#include "jsb/version.h"

int jsb_board_install(jsb_engine *engine)
{
    static const jsb_host_def defs[] = {
        JSB_DEF_STRING("name", JSB_BOARD),
#ifdef JSB_BOARD_LED
        JSB_DEF_NUMBER("LED", JSB_BOARD_LED),
#endif
    };
    return jsb_engine_define(engine, "board", defs, sizeof defs / sizeof defs[0]);
}
