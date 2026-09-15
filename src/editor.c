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
    else if (E.mode == INSERT)
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
        }
    }
}

void editor_process_insert_mode_key(key k)
{
    if (E.active_buffer == NULL)
    {
        return;
    }

    if (k.value == KEY_ESCAPE)
    {
        E.mode = NORMAL;
        return;
    }

    u8 b2[128];
    sprintf((char *)b2, "%c", k.value);

    string s_tmp = {.data = b2, .len = 1};
    buffer_insert(E.active_buffer, E.cursor_offset, s_tmp);

    E.cursor_offset++;
}

void editor_process_visual_mode_key(key k)
{

}
