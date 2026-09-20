#include "buffer.h"
#include "editor.h"
#include "input.h"
#include "cmd.h"

void editor_process_input(key k)
{

    if (E.mode == NORMAL)
    {
        /* @cleanup */
        switch(k.value)
        {
            case KEY_ESCAPE:
                {
                    E.running = 0;
                    break;
                }
        }
        editor_process_normal_mode_key(k);
    }
    else if (E.mode == INSERT || E.mode == INSERT_RIGHT_OF_CURSOR)
    {
        editor_process_insert_mode_key(k);
    }
    else if (E.mode == VISUAL)
    {
        editor_process_visual_mode_key(k);
    }

    return;
}

void editor_process_normal_mode_key(key k)
{
    switch(k.value)
    {
        case KEY_I_LOWER:
            {
                E.mode = INSERT;
                break;
            }
        case KEY_L_LOWER:
            {
                cmd_move_cursor_right();
                break;
            }
        case KEY_H_LOWER:
            {
                cmd_move_cursor_left();
                break;
            }
        case KEY_J_LOWER:
            {
                cmd_move_cursor_down();
                break;
            }
        case KEY_K_LOWER:
            {
                cmd_move_cursor_up();
                break;
            }
        case KEY_A_LOWER:
            {
                E.mode = INSERT_RIGHT_OF_CURSOR;
                E.cursor_offset++;
                break;
            }
        case KEY_A_UPPER:
            {
                E.mode = INSERT_RIGHT_OF_CURSOR;
                cmd_move_cursor_to_end_of_line();
                E.cursor_offset++;
                break;
            }
        default:
            {
                break;
            }
    }
}

void editor_process_insert_mode_key(key k)
{
    /* @cleanup: probably init a buffer the editor was opened without a file */
    if (E.active_buffer == NULL)
    {
        return;
    }

    switch (k.value)
    {
        case KEY_ESCAPE:
            {
                if (E.mode == INSERT_RIGHT_OF_CURSOR)
                {
                    /*
                     * @note: it inserts to the right of the cursor, but
                     * when pressing escape the cursor should go back to
                     * the left. That's how vim handles it.
                     */
                    E.cursor_offset--;
                }
                E.mode = NORMAL;
                break;
            }
        case KEY_DELETE:
        case KEY_BACKSPACE:
            {
                cmd_delete_character();
                cmd_move_cursor_left();
                return;
            }
        default:
            {
                u8 b2[128];
                sprintf((char *)b2, "%c", k.value);

                string s_tmp = {.data = b2, .len = 1};
                buffer_insert(E.active_buffer, E.cursor_offset, s_tmp);

                if (E.mode == INSERT_RIGHT_OF_CURSOR)
                {
                    E.cursor_offset++;
                }
                else
                {
                    cmd_move_cursor_right();
                }
            }
    }
}

void editor_process_visual_mode_key(key k)
{

}
