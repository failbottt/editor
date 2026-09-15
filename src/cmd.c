#include "cmd.h"
#include "editor.h"
#include "buffer.h"

void cmd_move_cursor_right()
{
    /*
     * cases:
     * 1. The cursor is at offset 0, but hte doc length is zero
     * 2. The cursor is at offset 0, the the doc length is not zero
     * 3. the cursor is at the end of the document
     */

    u64 doc_length = buffer_document_length(E.active_buffer);

    if (E.cursor_offset == 0 && doc_length == 0)
    {
        return;
    }

}
