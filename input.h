#ifndef INPUT_H
#define INPUT_H

void
move_cursor_right(Editor *ed)
{
	ed->cursor.x++;
}

void
move_cursor_left(Editor *ed)
{
	ed->cursor.x--;
}

void
move_cursor_up(Editor *ed)
{
	if (ed->cursor.y  != 0) {
		ed->cursor.y--;
	}
}

void
move_cursor_down(Buffer *b, long *line_ends, Editor *ed)
{
	ed->cursor.x = 0;
	ed->cursor.y++;
	printf("CURSOR X: %ld\n", ed->cursor.x);
	printf("CURSOR Y: %ld\n",ed->cursor.y);
}

#endif
