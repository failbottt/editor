#include "base.h"
#include "cmd.h"
#include "editor.h"

void process_cmd(arena cmd)
{
    if (cmd.data[0] != ':')
    {
        return;
    }

    if (cmd.data[1] == 'q')
    {
        u8 has_trailing = FALSE;
        int i = 2;
        while (i < cmd.cur_pos)
        {
            if (cmd.data[i] == ' ')
            {
                i++;
                continue;
            }
            has_trailing = TRUE;
            break;
        }

        if (has_trailing)
        {
            E.running = 0;
        }
        else
        {
            /* write status line about trailing text after quit command */
        }
    }
}
