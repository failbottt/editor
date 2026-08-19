#ifndef FUNCS_H
#define FUNCS_H

#include "base.h"
#include "buffer.h"

void join_lines(buffer *b, u64 line);
u64 insert_at_end_of_line(buffer *b, u64 line);
u64 insert_above_current_line(buffer *b, u64 line);
u64 insert_below_current_line(buffer *b, u64 line);

#endif
