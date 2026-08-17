#include "base.h"
#include "cmd.h"
#include "editor.h"
#include "file.h"
#include <limits.h>

static void
set_write_status_message(write_file_result result)
{
    char message[256];
    int path_len;

    if (result.path.s == NULL || result.path.len == 0)
    {
        editor_set_cmd_status_message((u8*)"File written");
        return;
    }

    if (result.path.len > (u64)INT_MAX)
    {
        editor_set_cmd_status_message((u8*)"File written");
        return;
    }

    path_len = (int)result.path.len;
    snprintf(
            message,
            sizeof(message),
            "\"%.*s\" %lluL, %lluB written",
            path_len,
            (char *)result.path.s,
            (unsigned long long)result.line_count,
            (unsigned long long)result.bytes_written
            );

    editor_set_cmd_status_message((u8*)message);
}

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
        int force = 0;
        write_file_result result;

        i++;

        if (i < cmd->cur_pos && cmd->data[i] == '!')
        {
            force = 1;
            i++;
        }

        while (i < cmd->cur_pos && (cmd->data[i] == ' ' || cmd->data[i] == '\t'))
        {
            i++;
        }

        if (i != cmd->cur_pos)
        {
            editor_set_cmd_status_message((u8*)"Trailing characters on write");
        }
        else
        {
            result = write_file(editor_active_buffer(), force);

            if (result.status == WRITE_FILE_OK)
            {
                set_write_status_message(result);
            }
            else if (result.status == WRITE_FILE_NO_PATH)
            {
                editor_set_cmd_status_message((u8*)"No file name");
            }
            else if (result.status == WRITE_FILE_NEEDS_CONFIRMATION)
            {
                editor_set_cmd_status_message((u8*)"File changed on disk. Use :w! to overwrite");
            }
            else
            {
                editor_set_cmd_status_message((u8*)"Unable to write file");
            }
        }
    }
    else
    {
        editor_set_cmd_status_message((u8*)"Unknown command");
    }

    cmd->cur_pos = 0;
    E.mode = EDITOR_NORMAL_MODE;
}
