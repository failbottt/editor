#ifndef INPUT_H
#define INPUT_H
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <X11/Xutil.h>
void
poll_events(Editor *ed) {
#if LINUX_IMPLEMENTATION
	while (XPending(display) > 0) 
	{
		XEvent event = {0};
		XNextEvent(display, &event);
		if (event.type == KeyPress) {
			KeySym key = XLookupKeysym(&event.xkey, 0);

			if (key == XK_K) {
				printf("PRESSED K\n");
			}

			switch(key) {
				case XK_Escape:
					quit = 1;
					break;
				case XK_j:
					if (ed.status == NORMAL) {
						ed->cursor.y++;
					}
					break;

				case XK_k:
					if (ed.status == NORMAL) {
						if (ed->cursor.y != 0) {
							ed->cursor.y--;
						}
					}
					break;
				case XK_l:
					if (ed.status == NORMAL) {
						ed->cursor.x++;
					}
					break;
				case XK_h:
					if (ed.status == NORMAL) {
						ed->cursor.x--;
					}
					break;
			}
		}
	}
#endif
}

#endif

