#ifndef GFX_H
#define GFX_H

#include "base.h"

void gfx_init();
void gfx_draw_text(u8 *text, u64 len);
void gfx_clear_screen();
void gfx_cleanup();

#endif
