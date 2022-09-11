#ifndef GAP_BUFFER_H
#define GAP_BUFFER_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN_BUFFER_SIZE 1024

typedef struct buffer {
  uint64_t size;
  uint64_t cursor;
  uint64_t gap_end;
  char *buffer;
} Buffer;

Buffer *
new_buffer(uint64_t size)
{
	// reserve memory for a new buffer
	Buffer *b = malloc(sizeof(Buffer));

	if (!b) return NULL;

	// if the size we tried to alloc was smaller than the 
	// min sisze for a buffer, take the min size
	size = MAX(size, MIN_BUFFER_SIZE);

	// malloc a buffer for the text
	b->buffer = malloc(size);
	memset(b->buffer, 0, size);

	if (!b->buffer) {
		free(b);
		return NULL;
	}

	// total size of the buffer
	b->size = size;
	// the length of the buffer (and thus the end)
	b->gap_end = size;
	// the start of the buffer
	b->cursor = 0;

	return b;
}

// gets the end of the leftside of buffer
int 
get_leftside_buffer(Buffer *b)
{
	return b->cursor;
}

// get the size of the right half of the buffer
int
get_rightside_size(Buffer *b)
{
	return b->size - b->gap_end;
}

long unsigned int
gb_used(Buffer *b)
{
	return get_leftside_buffer(b) + get_rightside_size(b);
}

void
move_back(Buffer* b, char* new_buf, size_t new_size)
{
	// back of new_buf
	char *dest = new_buf + new_size - get_rightside_size(b);

	// back of buffer
	char *src = b->buffer + b->gap_end;

	memmove(dest, src, get_rightside_size(b));  
}

// shrink buf to new_size
// no return value; no bool as in grow_buffer() -> does not matter if we cannot shrink buffer
// in other words: shrink_buffer() cannot fail!
void
shrink_buffer(Buffer* b, size_t new_size)
{
  // we have to move first the back text forward and then shrink
  new_size = MAX(new_size, MIN_BUFFER_SIZE);
  if (new_size < gb_used(b)) return;            // we do not resize if we'd lose data!
  move_back(b, b->buffer, new_size);          // move back text forward
  b->gap_end  = new_size - get_rightside_size(b);        // set gap_end before updating the size or gb_back(buf) will be wrong
  b->size     = new_size;
  
  char* new_buf = realloc(b->buffer, new_size); // allocate a smaller buffer
  if (new_buf) b->buffer = new_buf;
}

// before:
// foo               bar
//    ^             ^
//    |             |
//    cursor        gap_end
// after:
// fo                bar
//   ^              ^
//   |              |
//   cursor         gap_end
void
backspace(Buffer* b)
{
  // the gap is not printed, just move the cursor
  if (b->cursor > 0) b->cursor--;
  if (gb_used(b) < b->size / 4) shrink_buffer(b, b->size / 2);
}

// before:
// foo               bar
//    ^             ^
//    |             |
//    cursor        gap_end
// after:
// foo                ar
//    ^              ^
//    |              |
//    cursor         gap_end
void delete(Buffer* b)
{
  if (b->gap_end < b->size) b->gap_end++;
  if (gb_used(b) < b->size / 4)  shrink_buffer(b, b->size / 2);
}

void
free_buffer(Buffer *b)
{
  if (!b) return;
  free(b->buffer);
  free(b);
}


int
grow_buffer(Buffer *b, size_t new_size)
{
	new_size = MAX(new_size, MIN_BUFFER_SIZE);	

	if (b->size >= new_size) return 0;

	char *new_buf = realloc(b->buffer, new_size);

	if (!new_buf) return 0;

	move_back(b, new_buf, new_size);

	b->buffer = new_buf;
	b->gap_end = new_size - get_rightside_size(b);
	b->size = new_size;

	return 1;
}

int 
insert_character(Buffer *b, char c)
{
	size_t new_size;
	// if we're at the end of the buffer we need to resize the buffer
	if (b->cursor == b->gap_end) {
		if (b->size < SIZE_MAX / 2) {
			new_size = 2 * b->size;
		} else {
			new_size = SIZE_MAX;
		}

		if (!grow_buffer(b, new_size)) return 0;
	}

	b->buffer[b->cursor++] = c;

	return 1;
}
char*
extract_text(Buffer* buf)
{
  // It is insanely unlikely to happen, but if it does then we do not have space for the zero terminal.
  if (SIZE_MAX == gb_used(buf)) return NULL;

  char* text = malloc(gb_used(buf) + 1);
  if (!text) return NULL;

  strncpy(text              , buf->buffer               , buf->cursor);
  strncpy(text + buf->cursor, buf->buffer + buf->gap_end, get_rightside_size(buf));
  text[gb_used(buf)] = '\0';

  return text;
}

/* #if ED_DEBUG */
/* void */
/* print_buffer(Buffer* b) { */
/*   char* text = extract_text(b); */

/*   printf("CURSOR: %lu\n", b->cursor); */

/*   for(size_t i = 0; i < strlen(text); i++) { */
/*     if (i == b->cursor) { */
/*       printf("_"); */
/*     } else { */
/*       printf("%c", text[i]); */
/*     } */
/*   } */
/*   free(text); */
/* } */
/* #endif */

#endif
