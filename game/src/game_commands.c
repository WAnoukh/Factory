#include "game_commands.h"
#include "console/console.h"
#include "string/BString.h"
#include "string/StringView.h"
#include "game.h"
#include <stdlib.h>

void command_time_scale(struct CommandContext *cctx, StringView args, BString *out)
{
    char* endptr;
    float time_scale = strtof(args.data, &endptr);

    if(endptr == args.data){
        bstr_cat_cstr(out, "Invalid arguments: uiscale need a numerical value");
        return;

    }
    else if (time_scale < 0.2) time_scale = 1;

    struct Game *game = cctx->ctx;
    game->time_scale = time_scale;
}

void game_register_commands()
{
    console_register_command(VIEW_FROM_CONST_STR("time_scale"), command_time_scale);
}
