#include "ak_render.h"

#include <stdio.h>
#include <string.h>

static const char *AK_RENDER_STAT_LABELS[AK_GAME_STAT_COUNT] = {
    "HIT POINTS.....",
    "STRENGTH.......",
    "DEXTERITY......",
    "STAMINA........",
    "WISDOM.........",
    "GOLD..........."
};

static AkRenderCommand *append_command(AkRenderCommandBuffer *buffer, AkRenderCommandType type) {
    AkRenderCommand *command;

    if (buffer == NULL) {
        return NULL;
    }
    if (buffer->count >= AK_RENDER_MAX_COMMANDS) {
        buffer->truncated = 1;
        return NULL;
    }

    command = &buffer->commands[buffer->count];
    memset(command, 0, sizeof(*command));
    command->type = type;
    buffer->count++;
    return command;
}

static void copy_text(char destination[AK_RENDER_TEXT_SIZE], const char *source) {
    if (source == NULL) {
        destination[0] = '\0';
        return;
    }
    snprintf(destination, AK_RENDER_TEXT_SIZE, "%s", source);
}

static void append_mode(AkRenderCommandBuffer *buffer, AkRenderMode mode) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_SET_MODE);
    if (command != NULL) {
        command->mode = mode;
    }
}

static void append_clear(AkRenderCommandBuffer *buffer) {
    append_command(buffer, AK_RENDER_COMMAND_CLEAR);
}

static void append_color(AkRenderCommandBuffer *buffer, AkRenderColor color) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_SET_COLOR);
    if (command != NULL) {
        command->color = color;
    }
}

static void append_text(AkRenderCommandBuffer *buffer, int x, int y, const char *text, int inverse) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_TEXT);
    if (command != NULL) {
        command->x = x;
        command->y = y;
        command->inverse = inverse;
        copy_text(command->text, text);
    }
}

static void append_line(AkRenderCommandBuffer *buffer, int x, int y, int x2, int y2) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_LINE);
    if (command != NULL) {
        command->x = x;
        command->y = y;
        command->x2 = x2;
        command->y2 = y2;
    }
}

static void append_prompt(AkRenderCommandBuffer *buffer, int x, int y, const char *text) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_PROMPT);
    if (command != NULL) {
        command->x = x;
        command->y = y;
        copy_text(command->text, text);
    }
}

static void append_status(AkRenderCommandBuffer *buffer, int x, int y, const char *label, int value) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_STATUS);
    if (command != NULL) {
        command->x = x;
        command->y = y;
        command->value = value;
        copy_text(command->text, label);
    }
}

static void append_tile(AkRenderCommandBuffer *buffer, int x, int y, int tile, const char *name) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_TILE);
    if (command != NULL) {
        command->x = x;
        command->y = y;
        command->value = tile;
        copy_text(command->text, name);
    }
}

static void append_monster(AkRenderCommandBuffer *buffer, int monster, int x, int y) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_CREATURE);
    if (command != NULL) {
        command->x = x;
        command->y = y;
        command->value = monster;
        copy_text(command->text, ak_game_monster_name(monster));
    }
}

static void append_main_status(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    append_status(buffer, 30, 22, "FOOD", state->inventory[AK_GAME_ITEM_FOOD]);
    append_status(buffer, 30, 23, "H.P.", state->stats[AK_GAME_STAT_HIT_POINTS]);
    append_status(buffer, 30, 24, "GOLD", state->stats[AK_GAME_STAT_GOLD]);
}

static void render_startup(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    int i;

    append_mode(buffer, AK_RENDER_MODE_TEXT);
    append_clear(buffer);
    if (state->mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER) {
        append_prompt(buffer, 1, 5, "TYPE THY LUCKY NUMBER.....");
    } else if (state->mode == AK_GAME_MODE_STARTUP_LEVEL) {
        append_prompt(buffer, 1, 7, "LEVEL OF PLAY (1-10)......");
    } else if (state->mode == AK_GAME_MODE_CLASS_SELECT) {
        append_prompt(buffer, 1, 15, "AND SHALT THOU BE A FIGHTER OR A MAGE?");
    } else {
        for (i = 0; i < AK_GAME_STAT_COUNT; i++) {
            append_status(buffer, 1, 8 + i, AK_RENDER_STAT_LABELS[i], state->stats[i]);
        }
        append_prompt(buffer, 1, 15, "SHALT THOU PLAY WITH THESE QUALITIES?");
    }
}

static void render_town(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    int i;

    append_mode(buffer, AK_RENDER_MODE_TEXT);
    append_clear(buffer);
    append_text(buffer, 1, 3, "     STAT'S              WEAPONS", 0);
    for (i = 0; i < AK_GAME_STAT_COUNT; i++) {
        append_status(buffer, 1, 5 + i, AK_RENDER_STAT_LABELS[i], state->stats[i]);
        append_status(buffer, 25, 5 + i, ak_game_item_name((AkGameItem)i), state->inventory[i]);
    }
    append_text(buffer, 18, 11, "Q-QUIT", 0);
    append_text(buffer, 5, 17, "PRICE", 0);
    append_text(buffer, 15, 17, "DAMAGE", 0);
    append_text(buffer, 25, 17, "ITEM", 0);
    append_text(buffer, 5, 19, "1 FOR 10", 0);
    append_text(buffer, 15, 19, "N/A", 0);
    append_text(buffer, 5, 20, "8", 0);
    append_text(buffer, 15, 20, "1-10", 0);
    append_text(buffer, 5, 21, "5", 0);
    append_text(buffer, 15, 21, "1-5", 0);
    append_text(buffer, 5, 22, "6", 0);
    append_text(buffer, 15, 22, "1", 0);
    append_text(buffer, 5, 23, "3", 0);
    append_text(buffer, 15, 23, "1-4", 0);
    append_text(buffer, 5, 24, "15", 0);
    append_text(buffer, 15, 24, "?????", 0);
    append_prompt(buffer, 1, 1, "WELCOME TO THE ADVENTURE SHOP");
    append_prompt(buffer, 1, 2, "WHICH ITEM SHALT THOU BUY");
}

static int overworld_tile_at(const AkGameState *state, int x, int y) {
    if (x < 0 || x >= AK_GAME_OVERWORLD_SIZE || y < 0 || y >= AK_GAME_OVERWORLD_SIZE) {
        return AK_GAME_TILE_MOUNTAIN;
    }
    return state->overworld[x][y];
}

static void render_overworld(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    int dx;
    int dy;

    append_mode(buffer, AK_RENDER_MODE_HIRES);
    append_clear(buffer);
    append_color(buffer, AK_RENDER_COLOR_WHITE);
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int tile = overworld_tile_at(state, state->overworld_x + dx, state->overworld_y + dy);
            append_tile(buffer, 139 + dx * 44, 80 + dy * 34, tile, ak_game_tile_name((AkGameTile)tile));
        }
    }
    append_text(buffer, 134, 80, "@", 1);
    append_main_status(state, buffer);
    append_prompt(buffer, 1, 24, "COMMAND?");
}

static int dungeon_tile_at(const AkGameState *state, int x, int y) {
    if (x < 0 || x >= AK_GAME_DUNGEON_SIZE || y < 0 || y >= AK_GAME_DUNGEON_SIZE) {
        return AK_GAME_DUNGEON_WALL;
    }
    return state->dungeon[x][y];
}

static int active_monster_at(const AkGameState *state, int x, int y) {
    int monster;

    for (monster = 1; monster <= AK_GAME_MONSTER_COUNT; monster++) {
        if (state->monster_active[monster] &&
            state->monster_x[monster] == x &&
            state->monster_y[monster] == y) {
            return monster;
        }
    }
    return 0;
}

static int direction_dx(AkGameDirection direction) {
    if (direction == AK_GAME_DIRECTION_EAST) {
        return 1;
    }
    if (direction == AK_GAME_DIRECTION_WEST) {
        return -1;
    }
    return 0;
}

static int direction_dy(AkGameDirection direction) {
    if (direction == AK_GAME_DIRECTION_SOUTH) {
        return 1;
    }
    if (direction == AK_GAME_DIRECTION_NORTH) {
        return -1;
    }
    return 0;
}

static void render_dungeon(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    int ahead_x = state->dungeon_x + direction_dx(state->facing);
    int ahead_y = state->dungeon_y + direction_dy(state->facing);
    int tile = dungeon_tile_at(state, ahead_x, ahead_y);
    int monster = active_monster_at(state, ahead_x, ahead_y);

    append_mode(buffer, AK_RENDER_MODE_HIRES);
    append_clear(buffer);
    append_color(buffer, AK_RENDER_COLOR_WHITE);
    append_line(buffer, 20, 20, 90, 70);
    append_line(buffer, 260, 20, 190, 70);
    append_line(buffer, 90, 70, 190, 70);
    append_line(buffer, 90, 120, 190, 120);
    append_line(buffer, 20, 170, 90, 120);
    append_line(buffer, 260, 170, 190, 120);
    append_tile(buffer, 139, 100, tile, ak_game_dungeon_tile_name((AkGameDungeonTile)tile));
    if (monster > 0) {
        append_monster(buffer, monster, 139, 92);
    }
    if (tile == AK_GAME_DUNGEON_CHEST) {
        append_text(buffer, 116, 10, "CHEST", 1);
    } else if (tile == AK_GAME_DUNGEON_TRAP) {
        append_text(buffer, 106, 10, "HIDDEN TRAP", 1);
    }
    append_main_status(state, buffer);
    append_prompt(buffer, 1, 24, "COMMAND?");
}

static void render_quest(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    append_mode(buffer, AK_RENDER_MODE_TEXT);
    append_clear(buffer);
    append_text(buffer, 1, 3, "     WELCOME PEASANT INTO THE HALLS OF", 0);
    append_text(buffer, 1, 4, "THE MIGHTY LORD BRITISH.", 0);
    if (state->quest_target > 0) {
        append_text(buffer, 1, 7, "THOU MUST KILL A(N)", 0);
        append_text(buffer, 24, 7, ak_game_monster_name(state->quest_target), 1);
    } else if (state->quest_target < 0) {
        append_text(buffer, 1, 7, "THOU HAST ACOMPLISHED THY QUEST!", 0);
    } else {
        append_prompt(buffer, 1, 7, "WHAT IS THY NAME PEASANT");
    }
    append_prompt(buffer, 9, 24, "PRESS -SPACE- TO CONT.");
}

static void render_death(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    (void)state;
    append_mode(buffer, AK_RENDER_MODE_TEXT);
    append_clear(buffer);
    append_text(buffer, 1, 3, "        WE MOURN THE PASSING OF", 0);
    append_prompt(buffer, 13, 24, "<HIT ESC KEY>");
}

static void render_victory(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    (void)state;
    append_mode(buffer, AK_RENDER_MODE_TEXT);
    append_clear(buffer);
    append_text(buffer, 1, 4, "       THOU HAST PROVED THYSELF WORTHY", 0);
    append_text(buffer, 1, 5, "OF KNIGHTHOOD, CONTINUE PLAY IF THOU", 0);
    append_text(buffer, 1, 6, "DOTH WISH, BUT THOU HAST ACOMPLISHED", 0);
    append_text(buffer, 1, 7, "THE MAIN OBJECTIVE OF THIS GAME...", 0);
}

void ak_render_buffer_init(AkRenderCommandBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    memset(buffer, 0, sizeof(*buffer));
}

int ak_render_game(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    if (state == NULL || buffer == NULL) {
        return 0;
    }

    ak_render_buffer_init(buffer);
    switch (state->mode) {
        case AK_GAME_MODE_STARTUP_LUCKY_NUMBER:
        case AK_GAME_MODE_STARTUP_LEVEL:
        case AK_GAME_MODE_CHARACTER_REVIEW:
        case AK_GAME_MODE_CLASS_SELECT:
            render_startup(state, buffer);
            break;
        case AK_GAME_MODE_TOWN:
            render_town(state, buffer);
            break;
        case AK_GAME_MODE_OVERWORLD:
            render_overworld(state, buffer);
            break;
        case AK_GAME_MODE_DUNGEON:
        case AK_GAME_MODE_COMBAT:
            render_dungeon(state, buffer);
            break;
        case AK_GAME_MODE_QUEST:
            render_quest(state, buffer);
            break;
        case AK_GAME_MODE_DEATH:
            render_death(state, buffer);
            break;
        case AK_GAME_MODE_VICTORY:
            render_victory(state, buffer);
            break;
    }

    return !buffer->truncated;
}

const char *ak_render_command_name(AkRenderCommandType type) {
    switch (type) {
        case AK_RENDER_COMMAND_NONE: return "none";
        case AK_RENDER_COMMAND_SET_MODE: return "set_mode";
        case AK_RENDER_COMMAND_CLEAR: return "clear";
        case AK_RENDER_COMMAND_SET_COLOR: return "set_color";
        case AK_RENDER_COMMAND_TEXT: return "text";
        case AK_RENDER_COMMAND_LINE: return "line";
        case AK_RENDER_COMMAND_PROMPT: return "prompt";
        case AK_RENDER_COMMAND_STATUS: return "status";
        case AK_RENDER_COMMAND_TILE: return "tile";
        case AK_RENDER_COMMAND_CREATURE: return "monster";
    }
    return "unknown";
}

const char *ak_render_mode_name(AkRenderMode mode) {
    switch (mode) {
        case AK_RENDER_MODE_TEXT: return "text";
        case AK_RENDER_MODE_HIRES: return "hires";
    }
    return "unknown";
}
