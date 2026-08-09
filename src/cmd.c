#include "base.h"
#include "cmd.h"
#include "editor.h"

void cmd_process(arena *cmd)
{
    if (cmd == NULL || cmd->data == NULL)
    {
        return;
    }

    if (cmd->cur_pos == 0)
    {
        return;
    }

    /* enter with only colon */
    if (cmd->cur_pos == 1 && cmd->data[0] == ':')
    {
        cmd->cur_pos = 0;
        E.mode = EDITOR_NORMAL_MODE;
        return;
    }

    /* Skip the leading ':' and parse the command before clearing it. */
    u64 i = 1;

    while (i < cmd->cur_pos && (cmd->data[i] == ' ' || cmd->data[i] == '\t'))
    {
        i++;
    }

    if (i < cmd->cur_pos && cmd->data[i] == 'q')
    {
        i++;

        while (i < cmd->cur_pos && (cmd->data[i] == ' ' || cmd->data[i] == '\t'))
        {
            i++;
        }

        if (i == cmd->cur_pos)
        {
            cmd->cur_pos = 0;
            E.mode = EDITOR_NORMAL_MODE;
            E.running = FALSE;
            return;
        }

        editor_set_cmd_status_message((u8*)"Trailing characters on quit");
    }
    else if (i < cmd->cur_pos && cmd->data[i] == 'w')
    {
        editor_save_file(editor_active_buffer());
    }
    else
    {
        editor_set_cmd_status_message((u8*)"Unknown command");
    }

    cmd->cur_pos = 0;
    E.mode = EDITOR_NORMAL_MODE;
}
