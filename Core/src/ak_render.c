#include "ak_render.h"

#include <math.h>
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

static void append_polyline(AkRenderCommandBuffer *buffer, const int points[][2], int count) {
    int index;

    for (index = 0; index + 1 < count; index++) {
        append_line(buffer, points[index][0], points[index][1], points[index + 1][0], points[index + 1][1]);
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

static void append_monster(AkRenderCommandBuffer *buffer, int monster, int x, int y, int distance) {
    AkRenderCommand *command = append_command(buffer, AK_RENDER_COMMAND_CREATURE);
    if (command != NULL) {
        command->x = x;
        command->y = y;
        command->x2 = distance;
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
    for (i = 0; i < AK_GAME_ITEM_COUNT; i++) {
        append_text(buffer, 25, 19 + i, ak_game_item_name((AkGameItem)i), 0);
    }
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

static int source_tile_for_dungeon_tile(int tile) {
    if (tile == AK_GAME_DUNGEON_WALL ||
        tile == AK_GAME_DUNGEON_SECRET_DOOR ||
        tile == AK_GAME_DUNGEON_HIDDEN_DOOR ||
        tile == AK_GAME_DUNGEON_CHEST ||
        tile == AK_GAME_DUNGEON_LADDER_DOWN ||
        tile == AK_GAME_DUNGEON_LADDER_UP ||
        tile == AK_GAME_DUNGEON_LADDER_DOWN_ALT) {
        return tile;
    }
    return AK_GAME_DUNGEON_OPEN;
}

static int blocks_dungeon_view(int source_tile) {
    return source_tile == AK_GAME_DUNGEON_WALL ||
        source_tile == AK_GAME_DUNGEON_SECRET_DOOR ||
        source_tile == AK_GAME_DUNGEON_HIDDEN_DOOR;
}

static void dungeon_perspective_tables(
    int per[11][4],
    int ld[10][6],
    int cd[11][4],
    int ft[10][6],
    int lad[10][4]
) {
    int xx[11];
    int yy[11];
    int x;

    xx[0] = 139;
    yy[0] = 79;
    for (x = 2; x <= 20; x += 2) {
        int index = x / 2;
        xx[index] = (int)(((atan(1.0 / x) / atan(1.0)) * 140.0) + 0.5);
        yy[index] = (int)(xx[index] * 4.0 / 7.0);
    }

    for (x = 1; x <= 10; x++) {
        per[x][0] = 139 - xx[x];
        per[x][1] = 139 + xx[x];
        per[x][2] = 79 - yy[x];
        per[x][3] = 79 + yy[x];
    }
    per[0][0] = 0;
    per[0][1] = 279;
    per[0][2] = 0;
    per[0][3] = 159;

    for (x = 1; x <= 10; x++) {
        cd[x][0] = 139 - xx[x] / 3;
        cd[x][1] = 139 + xx[x] / 3;
        cd[x][2] = (int)(79 - yy[x] * 0.7);
        cd[x][3] = 79 + yy[x];
    }

    for (x = 0; x <= 9; x++) {
        int w;
        ld[x][0] = (per[x][0] * 2 + per[x + 1][0]) / 3;
        ld[x][1] = (per[x][0] + 2 * per[x + 1][0]) / 3;
        w = ld[x][0] - per[x][0];
        ld[x][2] = per[x][2] + w * 4 / 7;
        ld[x][3] = per[x][2] + 2 * w * 4 / 7;
        ld[x][4] = (per[x][3] * 2 + per[x + 1][3]) / 3;
        ld[x][5] = (per[x][3] + 2 * per[x + 1][3]) / 3;
        ld[x][2] = (int)(ld[x][4] - (ld[x][4] - ld[x][2]) * 0.8);
        ld[x][3] = (int)(ld[x][5] - (ld[x][5] - ld[x][3]) * 0.8);
        if (ld[x][3] == ld[x][4]) {
            ld[x][3]--;
        }

        ft[x][0] = 139 - xx[x] / 3;
        ft[x][1] = 139 + xx[x] / 3;
        ft[x][2] = 139 - xx[x + 1] / 3;
        ft[x][3] = 139 + xx[x + 1] / 3;
        ft[x][4] = 79 + (yy[x] * 2 + yy[x + 1]) / 3;
        ft[x][5] = 79 + (yy[x] + 2 * yy[x + 1]) / 3;

        lad[x][0] = (ft[x][0] * 2 + ft[x][1]) / 3;
        lad[x][1] = (ft[x][0] + 2 * ft[x][1]) / 3;
        lad[x][3] = ft[x][4];
        lad[x][2] = 159 - lad[x][3];
    }
}

static void append_chest_art(AkRenderCommandBuffer *buffer, int distance, int bottom_y) {
    int left = 139 - 10 / distance;
    int right = 139 + 10 / distance;
    int top = bottom_y - 10 / distance;
    int peak_left = 139 - 5 / distance;
    int peak_right = 139 + 15 / distance;
    int peak_y = bottom_y - 15 / distance;
    int side_y = bottom_y - 5 / distance;
    const int box[][2] = {{left, bottom_y}, {left, top}, {right, top}, {right, bottom_y}, {left, bottom_y}};
    const int lid[][2] = {{left, top}, {peak_left, peak_y}, {peak_right, peak_y}, {peak_right, side_y}, {right, bottom_y}};

    append_polyline(buffer, box, 5);
    append_polyline(buffer, lid, 5);
    append_line(buffer, right, top, peak_right, peak_y);
}

static void append_ladder(AkRenderCommandBuffer *buffer, int lx, int rx, int top, int base) {
    int y1 = (base * 4 + top) / 5;
    int y2 = (base * 3 + top * 2) / 5;
    int y3 = (base * 2 + top * 3) / 5;
    int y4 = (base + top * 4) / 5;

    append_line(buffer, lx, base, lx, top);
    append_line(buffer, rx, top, rx, base);
    append_line(buffer, lx, y1, rx, y1);
    append_line(buffer, lx, y2, rx, y2);
    append_line(buffer, lx, y3, rx, y3);
    append_line(buffer, lx, y4, rx, y4);
}

static void render_dungeon(const AkGameState *state, AkRenderCommandBuffer *buffer) {
    int per[11][4];
    int ld[10][6];
    int cd[11][4];
    int ft[10][6];
    int lad[10][4];
    int forward_x = direction_dx(state->facing);
    int forward_y = direction_dy(state->facing);
    int left_x = forward_y;
    int left_y = -forward_x;
    int distance;

    append_mode(buffer, AK_RENDER_MODE_HIRES);
    append_clear(buffer);
    append_color(buffer, AK_RENDER_COLOR_WHITE);

    dungeon_perspective_tables(per, ld, cd, ft, lad);

    for (distance = 0; distance < 10; distance++) {
        int center_x = state->dungeon_x + forward_x * distance;
        int center_y = state->dungeon_y + forward_y * distance;
        int center = source_tile_for_dungeon_tile(dungeon_tile_at(state, center_x, center_y));
        int left = source_tile_for_dungeon_tile(dungeon_tile_at(state, center_x + left_x, center_y + left_y));
        int right = source_tile_for_dungeon_tile(dungeon_tile_at(state, center_x - left_x, center_y - left_y));
        int monster = active_monster_at(state, center_x, center_y);
        int l1 = per[distance][0];
        int r1 = per[distance][1];
        int t1 = per[distance][2];
        int b1 = per[distance][3];
        int l2 = per[distance + 1][0];
        int r2 = per[distance + 1][1];
        int t2 = per[distance + 1][2];
        int b2 = per[distance + 1][3];

        if (distance != 0 && blocks_dungeon_view(center)) {
            const int wall[][2] = {{l1, t1}, {r1, t1}, {r1, b1}, {l1, b1}, {l1, t1}};
            append_polyline(buffer, wall, 5);
            if (center == AK_GAME_DUNGEON_HIDDEN_DOOR) {
                const int door[][2] = {{cd[distance][0], cd[distance][3]}, {cd[distance][0], cd[distance][2]}, {cd[distance][1], cd[distance][2]}, {cd[distance][1], cd[distance][3]}};
                append_polyline(buffer, door, 4);
            }
            if (monster > 0) {
                append_monster(buffer, monster, 139, 79 + (b1 - 79) / 2, distance);
            }
            break;
        }

        if (blocks_dungeon_view(left)) {
            append_line(buffer, l1, t1, l2, t2);
            append_line(buffer, l1, b1, l2, b2);
        } else {
            if (distance != 0) {
                append_line(buffer, l1, t1, l1, b1);
            }
            append_line(buffer, l1, t2, l2, t2);
            append_line(buffer, l2, t2, l2, b2);
            append_line(buffer, l2, b2, l1, b2);
        }

        if (left == AK_GAME_DUNGEON_HIDDEN_DOOR) {
            if (distance > 0) {
                const int door[][2] = {{ld[distance][0], ld[distance][4]}, {ld[distance][0], ld[distance][2]}, {ld[distance][1], ld[distance][3]}, {ld[distance][1], ld[distance][5]}};
                append_polyline(buffer, door, 4);
            } else {
                const int door[][2] = {{0, ld[distance][2] - 3}, {ld[distance][1], ld[distance][3]}, {ld[distance][1], ld[distance][5]}};
                append_polyline(buffer, door, 3);
            }
        }

        if (blocks_dungeon_view(right)) {
            append_line(buffer, r1, t1, r2, t2);
            append_line(buffer, r1, b1, r2, b2);
        } else {
            if (distance != 0) {
                append_line(buffer, r1, t1, r1, b1);
            }
            append_line(buffer, r1, t2, r2, t2);
            append_line(buffer, r2, t2, r2, b2);
            append_line(buffer, r2, b2, r1, b2);
        }

        if (right == AK_GAME_DUNGEON_HIDDEN_DOOR) {
            if (distance > 0) {
                const int door[][2] = {{279 - ld[distance][0], ld[distance][4]}, {279 - ld[distance][0], ld[distance][2]}, {279 - ld[distance][1], ld[distance][3]}, {279 - ld[distance][1], ld[distance][5]}};
                append_polyline(buffer, door, 4);
            } else {
                const int door[][2] = {{279, ld[distance][2] - 3}, {279 - ld[distance][1], ld[distance][3]}, {279 - ld[distance][1], ld[distance][5]}};
                append_polyline(buffer, door, 3);
            }
        }

        if (center == AK_GAME_DUNGEON_LADDER_DOWN || center == AK_GAME_DUNGEON_LADDER_DOWN_ALT) {
            const int floor_trap[][2] = {{ft[distance][0], ft[distance][4]}, {ft[distance][2], ft[distance][5]}, {ft[distance][3], ft[distance][5]}, {ft[distance][1], ft[distance][4]}, {ft[distance][0], ft[distance][4]}};
            append_polyline(buffer, floor_trap, 5);
            append_ladder(buffer, lad[distance][0], lad[distance][1], lad[distance][2], lad[distance][3]);
        } else if (center == AK_GAME_DUNGEON_LADDER_UP) {
            const int ceiling_trap[][2] = {{ft[distance][0], 158 - ft[distance][4]}, {ft[distance][2], 158 - ft[distance][5]}, {ft[distance][3], 158 - ft[distance][5]}, {ft[distance][1], 158 - ft[distance][4]}, {ft[distance][0], 158 - ft[distance][4]}};
            append_polyline(buffer, ceiling_trap, 5);
            append_ladder(buffer, lad[distance][0], lad[distance][1], lad[distance][2], lad[distance][3]);
        } else if (center == AK_GAME_DUNGEON_CHEST && distance > 0) {
            append_chest_art(buffer, distance, per[distance][3]);
            append_text(buffer, 116, 10, "CHEST", 1);
        } else if (center == AK_GAME_DUNGEON_TRAP && distance > 0) {
            append_text(buffer, 106, 10, "HIDDEN TRAP", 1);
        }

        if (monster > 0 && distance > 0) {
            append_monster(buffer, monster, 139, 79 + (b1 - 79) / 2, distance);
            break;
        }
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
