#include "ak_render.h"

#include <assert.h>
#include <string.h>

static void fill_overworld(AkGameState *state, AkGameTile tile) {
    int x;
    int y;

    for (x = 0; x < AK_GAME_OVERWORLD_SIZE; x++) {
        for (y = 0; y < AK_GAME_OVERWORLD_SIZE; y++) {
            state->overworld[x][y] = tile;
        }
    }
}

static void fill_dungeon(AkGameState *state, AkGameDungeonTile tile) {
    int x;
    int y;

    for (x = 0; x < AK_GAME_DUNGEON_SIZE; x++) {
        for (y = 0; y < AK_GAME_DUNGEON_SIZE; y++) {
            state->dungeon[x][y] = tile;
        }
    }
}

static int has_text(const AkRenderCommandBuffer *buffer, AkRenderCommandType type, const char *text) {
    size_t i;

    for (i = 0; i < buffer->count; i++) {
        if (buffer->commands[i].type == type &&
            strcmp(buffer->commands[i].text, text) == 0) {
            return 1;
        }
    }
    return 0;
}

static int has_status(const AkRenderCommandBuffer *buffer, const char *label, int value) {
    size_t i;

    for (i = 0; i < buffer->count; i++) {
        if (buffer->commands[i].type == AK_RENDER_COMMAND_STATUS &&
            strcmp(buffer->commands[i].text, label) == 0 &&
            buffer->commands[i].value == value) {
            return 1;
        }
    }
    return 0;
}

static int has_tile(const AkRenderCommandBuffer *buffer, int value, const char *name) {
    size_t i;

    for (i = 0; i < buffer->count; i++) {
        if (buffer->commands[i].type == AK_RENDER_COMMAND_TILE &&
            buffer->commands[i].value == value &&
            strcmp(buffer->commands[i].text, name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int count_commands(const AkRenderCommandBuffer *buffer, AkRenderCommandType type) {
    size_t i;
    int count = 0;

    for (i = 0; i < buffer->count; i++) {
        if (buffer->commands[i].type == type) {
            count++;
        }
    }
    return count;
}

static void test_startup_prompts_are_text_mode(void) {
    AkGameState state;
    AkRenderCommandBuffer buffer;

    ak_game_init(&state);
    assert(ak_render_game(&state, &buffer));
    assert(buffer.commands[0].type == AK_RENDER_COMMAND_SET_MODE);
    assert(buffer.commands[0].mode == AK_RENDER_MODE_TEXT);
    assert(buffer.commands[1].type == AK_RENDER_COMMAND_CLEAR);
    assert(has_text(&buffer, AK_RENDER_COMMAND_PROMPT, "TYPE THY LUCKY NUMBER....."));

    state.mode = AK_GAME_MODE_STARTUP_LEVEL;
    assert(ak_render_game(&state, &buffer));
    assert(has_text(&buffer, AK_RENDER_COMMAND_PROMPT, "LEVEL OF PLAY (1-10)......"));

    state.mode = AK_GAME_MODE_CHARACTER_REVIEW;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 12;
    assert(ak_render_game(&state, &buffer));
    assert(has_status(&buffer, "HIT POINTS.....", 12));
    assert(has_text(&buffer, AK_RENDER_COMMAND_PROMPT, "SHALT THOU PLAY WITH THESE QUALITIES?"));
}

static void test_town_screen_contains_store_tables(void) {
    AkGameState state;
    AkRenderCommandBuffer buffer;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_TOWN;
    state.location = AK_GAME_LOCATION_TOWN;
    state.stats[AK_GAME_STAT_GOLD] = 15;
    state.inventory[AK_GAME_ITEM_MAGIC_AMULET] = 1;

    assert(ak_render_game(&state, &buffer));
    assert(buffer.commands[0].mode == AK_RENDER_MODE_TEXT);
    assert(has_text(&buffer, AK_RENDER_COMMAND_TEXT, "     STAT'S              WEAPONS"));
    assert(has_status(&buffer, "GOLD...........", 15));
    assert(has_status(&buffer, "magic_amulet", 1));
    assert(has_text(&buffer, AK_RENDER_COMMAND_PROMPT, "WELCOME TO THE ADVENTURE SHOP"));
    assert(has_text(&buffer, AK_RENDER_COMMAND_PROMPT, "WHICH ITEM SHALT THOU BUY"));
}

static void test_overworld_screen_emits_hires_tiles_status_and_prompt(void) {
    AkGameState state;
    AkRenderCommandBuffer buffer;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 9;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 20;
    fill_overworld(&state, AK_GAME_TILE_OPEN);
    state.overworld[10][10] = AK_GAME_TILE_TOWN;
    state.overworld[11][10] = AK_GAME_TILE_DUNGEON;

    assert(ak_render_game(&state, &buffer));
    assert(buffer.commands[0].mode == AK_RENDER_MODE_HIRES);
    assert(has_tile(&buffer, AK_GAME_TILE_TOWN, "town"));
    assert(has_tile(&buffer, AK_GAME_TILE_DUNGEON, "dungeon"));
    assert(has_status(&buffer, "FOOD", 9));
    assert(has_status(&buffer, "H.P.", 20));
    assert(has_text(&buffer, AK_RENDER_COMMAND_PROMPT, "COMMAND?"));
}

static void test_dungeon_screen_emits_lines_encounter_and_monster(void) {
    AkGameState state;
    AkRenderCommandBuffer buffer;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.dungeon[6][5] = AK_GAME_DUNGEON_CHEST;
    state.monster_active[10] = 1;
    state.monster_x[10] = 6;
    state.monster_y[10] = 5;

    assert(ak_render_game(&state, &buffer));
    assert(buffer.commands[0].mode == AK_RENDER_MODE_HIRES);
    assert(buffer.commands[3].type == AK_RENDER_COMMAND_LINE);
    assert(count_commands(&buffer, AK_RENDER_COMMAND_LINE) > 10);
    assert(has_text(&buffer, AK_RENDER_COMMAND_CREATURE, "balrog"));
    assert(has_text(&buffer, AK_RENDER_COMMAND_TEXT, "CHEST"));
}

static void test_quest_and_victory_text_contracts(void) {
    AkGameState state;
    AkRenderCommandBuffer buffer;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_QUEST;
    state.location = AK_GAME_LOCATION_CASTLE;
    state.quest_target = 10;

    assert(ak_render_game(&state, &buffer));
    assert(buffer.commands[0].mode == AK_RENDER_MODE_TEXT);
    assert(has_text(&buffer, AK_RENDER_COMMAND_TEXT, "THE MIGHTY LORD BRITISH."));
    assert(has_text(&buffer, AK_RENDER_COMMAND_TEXT, "balrog"));
    assert(has_text(&buffer, AK_RENDER_COMMAND_PROMPT, "PRESS -SPACE- TO CONT."));

    state.mode = AK_GAME_MODE_VICTORY;
    assert(ak_render_game(&state, &buffer));
    assert(has_text(&buffer, AK_RENDER_COMMAND_TEXT, "       THOU HAST PROVED THYSELF WORTHY"));
    assert(has_text(&buffer, AK_RENDER_COMMAND_TEXT, "THE MAIN OBJECTIVE OF THIS GAME..."));
}

static void test_render_names_are_stable(void) {
    assert(strcmp(ak_render_command_name(AK_RENDER_COMMAND_LINE), "line") == 0);
    assert(strcmp(ak_render_command_name(AK_RENDER_COMMAND_CREATURE), "monster") == 0);
    assert(strcmp(ak_render_mode_name(AK_RENDER_MODE_HIRES), "hires") == 0);
}

int main(void) {
    test_startup_prompts_are_text_mode();
    test_town_screen_contains_store_tables();
    test_overworld_screen_emits_hires_tiles_status_and_prompt();
    test_dungeon_screen_emits_lines_encounter_and_monster();
    test_quest_and_victory_text_contracts();
    test_render_names_are_stable();
    return 0;
}
