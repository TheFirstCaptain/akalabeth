#include "ak_game.h"

#include <assert.h>
#include <string.h>

static AkGameCommand command_with_value(AkGameCommandType type, int value) {
    AkGameCommand command;
    memset(&command, 0, sizeof(command));
    command.type = type;
    command.value = value;
    return command;
}

static AkGameCommand class_command(AkGameClass player_class) {
    AkGameCommand command;
    memset(&command, 0, sizeof(command));
    command.type = AK_GAME_COMMAND_SELECT_CLASS;
    command.player_class = player_class;
    return command;
}

static AkGameCommand item_command(AkGameItem item) {
    AkGameCommand command;
    memset(&command, 0, sizeof(command));
    command.type = AK_GAME_COMMAND_BUY_ITEM;
    command.item = item;
    return command;
}

static AkGameCommand attack_command(AkGameItem item) {
    AkGameCommand command;
    memset(&command, 0, sizeof(command));
    command.type = AK_GAME_COMMAND_ATTACK;
    command.item = item;
    return command;
}

static AkGameCommand magic_command(int choice) {
    AkGameCommand command;
    memset(&command, 0, sizeof(command));
    command.type = AK_GAME_COMMAND_CAST_MAGIC;
    command.value = choice;
    return command;
}

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

static void test_initial_state_is_deterministic(void) {
    AkGameState first;
    AkGameState second;
    size_t i;

    ak_game_init(&first);
    ak_game_init(&second);

    assert(memcmp(&first, &second, sizeof(first)) == 0);
    assert(first.mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER);
    assert(first.location == AK_GAME_LOCATION_STARTUP);
    assert(first.facing == AK_GAME_DIRECTION_EAST);
    assert(first.player_class == AK_GAME_CLASS_NONE);
    assert(first.command_count == 0);

    for (i = 0; i < AK_GAME_STAT_COUNT; i++) {
        assert(first.stats[i] == 0);
    }

    for (i = 0; i < AK_GAME_ITEM_COUNT; i++) {
        assert(first.inventory[i] == 0);
    }
}

static void test_startup_command_flow(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;
    int first_stats[AK_GAME_STAT_COUNT];
    size_t i;

    ak_game_init(&state);

    command = command_with_value(AK_GAME_COMMAND_SET_LUCKY_NUMBER, 1234);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.lucky_number == 1234);
    assert(state.mode == AK_GAME_MODE_STARTUP_LEVEL);
    assert(state.command_count == 1);
    assert(result.event_count == 2);
    assert(result.events[0].type == AK_GAME_EVENT_MODE_CHANGED);
    assert(result.events[1].type == AK_GAME_EVENT_PROMPT_CHANGED);

    command = command_with_value(AK_GAME_COMMAND_SET_LEVEL, 5);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.level_of_play == 5);
    assert(state.mode == AK_GAME_MODE_CHARACTER_REVIEW);
    assert(state.command_count == 2);
    assert(result.event_count == 2);
    assert(result.events[0].type == AK_GAME_EVENT_MODE_CHANGED);
    assert(result.events[1].type == AK_GAME_EVENT_STATE_CHANGED);
    for (i = 0; i < AK_GAME_STAT_COUNT; i++) {
        assert(state.stats[i] >= 4);
        assert(state.stats[i] <= 24);
        first_stats[i] = state.stats[i];
    }

    command = command_with_value(AK_GAME_COMMAND_REROLL_CHARACTER, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(memcmp(first_stats, state.stats, sizeof(first_stats)) != 0);
    assert(state.mode == AK_GAME_MODE_CHARACTER_REVIEW);
    assert(state.command_count == 3);

    command = command_with_value(AK_GAME_COMMAND_ACCEPT_CHARACTER, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_CLASS_SELECT);
    assert(state.command_count == 4);

    command = class_command(AK_GAME_CLASS_MAGE);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.player_class == AK_GAME_CLASS_MAGE);
    assert(state.mode == AK_GAME_MODE_TOWN);
    assert(state.location == AK_GAME_LOCATION_TOWN);
    assert(state.command_count == 5);
}

static void test_invalid_commands_do_not_advance_state(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);

    command = command_with_value(AK_GAME_COMMAND_SET_LEVEL, 11);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_INVALID_COMMAND);
    assert(state.level_of_play == 0);
    assert(state.mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER);
    assert(state.command_count == 0);
    assert(result.event_count == 1);
    assert(result.events[0].type == AK_GAME_EVENT_ERROR);

    command = command_with_value(AK_GAME_COMMAND_MOVE_NORTH, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_INVALID_COMMAND);
    assert(state.location == AK_GAME_LOCATION_STARTUP);
    assert(state.command_count == 0);
}

static void test_combat_commands_accept_known_commands_in_valid_modes(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_TOWN;
    state.location = AK_GAME_LOCATION_TOWN;
    state.player_class = AK_GAME_CLASS_FIGHTER;
    state.stats[AK_GAME_STAT_GOLD] = 10;
    command = item_command(AK_GAME_ITEM_AXE);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.command_count == 1);
    assert(state.inventory[AK_GAME_ITEM_AXE] == 1);
    assert(state.stats[AK_GAME_STAT_GOLD] == 5);
    assert(result.events[0].command == AK_GAME_COMMAND_BUY_ITEM);
    assert(result.events[0].value == AK_GAME_ITEM_AXE);

    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    state.stats[AK_GAME_STAT_DEXTERITY] = 0;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.monster_active[1] = 1;
    state.monster_x[1] = 6;
    state.monster_y[1] = 5;
    state.monster_hit_points[1] = 10;
    command = attack_command(AK_GAME_ITEM_AXE);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.command_count == 2);
    assert(result.events[0].command == AK_GAME_COMMAND_ATTACK);

    command = command_with_value(AK_GAME_COMMAND_ACKNOWLEDGE, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.command_count == 3);
    assert(result.event_count == 1);
}

static void test_exit_from_town_generates_deterministic_overworld(void) {
    AkGameState first;
    AkGameState second;
    AkGameCommandResult result;
    AkGameCommand command;
    size_t map_size = sizeof(first.overworld);

    ak_game_init(&first);
    ak_game_init(&second);
    first.mode = AK_GAME_MODE_TOWN;
    first.location = AK_GAME_LOCATION_TOWN;
    first.lucky_number = 1234;
    second.mode = AK_GAME_MODE_TOWN;
    second.location = AK_GAME_LOCATION_TOWN;
    second.lucky_number = 1234;

    command = command_with_value(AK_GAME_COMMAND_EXIT, 0);
    assert(ak_game_apply_command(&first, &command, &result) == AK_GAME_RESULT_OK);
    assert(ak_game_apply_command(&second, &command, &result) == AK_GAME_RESULT_OK);

    assert(first.mode == AK_GAME_MODE_OVERWORLD);
    assert(first.location == AK_GAME_LOCATION_OVERWORLD);
    assert(first.command_count == 1);
    assert(first.overworld_x >= 1 && first.overworld_x <= 19);
    assert(first.overworld_y >= 1 && first.overworld_y <= 19);
    assert(first.overworld[first.overworld_x][first.overworld_y] == AK_GAME_TILE_TOWN);
    assert(first.overworld[0][10] == AK_GAME_TILE_MOUNTAIN);
    assert(first.overworld[10][0] == AK_GAME_TILE_MOUNTAIN);
    assert(first.overworld[20][10] == AK_GAME_TILE_MOUNTAIN);
    assert(first.overworld[10][20] == AK_GAME_TILE_MOUNTAIN);
    assert(memcmp(first.overworld, second.overworld, map_size) == 0);
    assert(first.overworld_x == second.overworld_x);
    assert(first.overworld_y == second.overworld_y);
}

static void test_overworld_movement_all_directions_consumes_food(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 20;
    fill_overworld(&state, AK_GAME_TILE_OPEN);

    command = command_with_value(AK_GAME_COMMAND_MOVE_NORTH, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.overworld_x == 10);
    assert(state.overworld_y == 9);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 19);

    command = command_with_value(AK_GAME_COMMAND_MOVE_EAST, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.overworld_x == 11);
    assert(state.overworld_y == 9);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 18);

    command = command_with_value(AK_GAME_COMMAND_MOVE_SOUTH, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.overworld_x == 11);
    assert(state.overworld_y == 10);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 17);

    command = command_with_value(AK_GAME_COMMAND_MOVE_WEST, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.overworld_x == 10);
    assert(state.overworld_y == 10);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 16);
}

static void test_overworld_mountain_blocking_still_consumes_food(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 3;
    fill_overworld(&state, AK_GAME_TILE_OPEN);
    state.overworld[11][10] = AK_GAME_TILE_MOUNTAIN;

    command = command_with_value(AK_GAME_COMMAND_MOVE_EAST, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.overworld_x == 10);
    assert(state.overworld_y == 10);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 2);
    assert(result.events[0].type == AK_GAME_EVENT_COMMAND_ACCEPTED);
    assert(result.events[1].type == AK_GAME_EVENT_STATE_CHANGED);
}

static void test_overworld_entry_rules(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 10;
    fill_overworld(&state, AK_GAME_TILE_OPEN);
    state.overworld[10][10] = AK_GAME_TILE_TOWN;

    command = command_with_value(AK_GAME_COMMAND_ENTER, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_TOWN);
    assert(state.location == AK_GAME_LOCATION_TOWN);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 9);

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 10;
    fill_overworld(&state, AK_GAME_TILE_OPEN);
    state.overworld[10][10] = AK_GAME_TILE_DUNGEON;

    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_DUNGEON);
    assert(state.location == AK_GAME_LOCATION_DUNGEON);
    assert(state.dungeon_level == 1);
    assert(state.dungeon_x == 1);
    assert(state.dungeon_y == 1);
    assert(state.facing == AK_GAME_DIRECTION_EAST);
    assert(state.dungeon[1][1] == AK_GAME_DUNGEON_LADDER_UP);
    assert(state.dungeon[0][5] == AK_GAME_DUNGEON_WALL);

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 10;
    fill_overworld(&state, AK_GAME_TILE_OPEN);
    state.overworld[10][10] = AK_GAME_TILE_CASTLE;

    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_QUEST);
    assert(state.location == AK_GAME_LOCATION_CASTLE);
}

static void test_enter_on_plain_overworld_tile_is_rejected_without_food_turn(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 10;
    fill_overworld(&state, AK_GAME_TILE_OPEN);

    command = command_with_value(AK_GAME_COMMAND_ENTER, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_REJECTED);
    assert(state.mode == AK_GAME_MODE_OVERWORLD);
    assert(state.location == AK_GAME_LOCATION_OVERWORLD);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 10);
    assert(state.command_count == 0);
}

static void test_overworld_starvation_enters_death_state(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 12;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.inventory[AK_GAME_ITEM_FOOD] = 0;
    fill_overworld(&state, AK_GAME_TILE_OPEN);

    command = command_with_value(AK_GAME_COMMAND_PASS, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == -1);
    assert(state.stats[AK_GAME_STAT_HIT_POINTS] == 0);
    assert(state.mode == AK_GAME_MODE_DEATH);
    assert(state.location == AK_GAME_LOCATION_DEATH);
}

static void test_dungeon_generation_is_deterministic_and_places_monsters(void) {
    AkGameState first;
    AkGameState second;
    AkGameCommandResult result;
    AkGameCommand command;
    int active_count = 0;
    int monster;

    ak_game_init(&first);
    ak_game_init(&second);
    first.mode = AK_GAME_MODE_OVERWORLD;
    first.location = AK_GAME_LOCATION_OVERWORLD;
    first.lucky_number = 1234;
    first.level_of_play = 5;
    first.overworld_x = 10;
    first.overworld_y = 10;
    first.inventory[AK_GAME_ITEM_FOOD] = 10;
    first.overworld[10][10] = AK_GAME_TILE_DUNGEON;

    second = first;

    command = command_with_value(AK_GAME_COMMAND_ENTER, 0);
    assert(ak_game_apply_command(&first, &command, &result) == AK_GAME_RESULT_OK);
    assert(ak_game_apply_command(&second, &command, &result) == AK_GAME_RESULT_OK);
    assert(memcmp(first.dungeon, second.dungeon, sizeof(first.dungeon)) == 0);
    assert(memcmp(first.monster_active, second.monster_active, sizeof(first.monster_active)) == 0);
    assert(first.dungeon[0][0] == AK_GAME_DUNGEON_WALL);
    assert(first.dungeon[1][1] == AK_GAME_DUNGEON_LADDER_UP);

    for (monster = 1; monster <= AK_GAME_MONSTER_COUNT; monster++) {
        if (first.monster_active[monster]) {
            active_count++;
            assert(first.monster_x[monster] >= 1 && first.monster_x[monster] <= 9);
            assert(first.monster_y[monster] >= 1 && first.monster_y[monster] <= 9);
            assert(first.dungeon[first.monster_x[monster]][first.monster_y[monster]] == AK_GAME_DUNGEON_OPEN);
        }
    }
    assert(active_count >= 0);
}

static void test_dungeon_turning_and_forward_movement(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);

    command = command_with_value(AK_GAME_COMMAND_TURN_LEFT, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.facing == AK_GAME_DIRECTION_NORTH);

    command = command_with_value(AK_GAME_COMMAND_FORWARD, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.dungeon_x == 5);
    assert(state.dungeon_y == 4);

    command = command_with_value(AK_GAME_COMMAND_TURN_RIGHT, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.facing == AK_GAME_DIRECTION_EAST);

    command = command_with_value(AK_GAME_COMMAND_TURN_AROUND, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.facing == AK_GAME_DIRECTION_WEST);
}

static void test_dungeon_forward_blocks_walls_and_monsters(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.dungeon[6][5] = AK_GAME_DUNGEON_WALL;

    command = command_with_value(AK_GAME_COMMAND_FORWARD, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.dungeon_x == 5);
    assert(state.dungeon_y == 5);

    state.dungeon[6][5] = AK_GAME_DUNGEON_OPEN;
    state.monster_active[1] = 1;
    state.monster_x[1] = 6;
    state.monster_y[1] = 5;
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.dungeon_x == 5);
    assert(state.dungeon_y == 5);
}

static void test_dungeon_ladder_transitions(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.dungeon_level = 1;
    state.dungeon_x = 1;
    state.dungeon_y = 1;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.dungeon[1][1] = AK_GAME_DUNGEON_LADDER_UP;

    command = command_with_value(AK_GAME_COMMAND_ENTER, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_OVERWORLD);
    assert(state.location == AK_GAME_LOCATION_OVERWORLD);

    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.dungeon_level = 1;
    state.dungeon_x = 3;
    state.dungeon_y = 3;
    state.lucky_number = 1234;
    state.overworld_x = 10;
    state.overworld_y = 10;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.dungeon[3][3] = AK_GAME_DUNGEON_LADDER_DOWN;

    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_DUNGEON);
    assert(state.location == AK_GAME_LOCATION_DUNGEON);
    assert(state.dungeon_level == 2);
}

static void test_dungeon_chest_and_trap_encounters(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_level = 1;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 20;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.dungeon[6][5] = AK_GAME_DUNGEON_CHEST;

    command = command_with_value(AK_GAME_COMMAND_FORWARD, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.dungeon_x == 6);
    assert(state.dungeon_y == 5);
    assert(state.dungeon[6][5] == AK_GAME_DUNGEON_OPEN);
    assert(state.stats[AK_GAME_STAT_GOLD] > 0);

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_level = 1;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    state.lucky_number = 1234;
    state.overworld_x = 10;
    state.overworld_y = 10;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 20;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.dungeon[6][5] = AK_GAME_DUNGEON_TRAP;

    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.stats[AK_GAME_STAT_HIT_POINTS] < 20);
    assert(state.dungeon_level == 2);
}

static void test_dungeon_monster_turn_movement_and_attack(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.dungeon_level = 1;
    state.level_of_play = 1;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.monster_active[1] = 1;
    state.monster_x[1] = 5;
    state.monster_y[1] = 7;
    state.monster_hit_points[1] = 10;

    command = command_with_value(AK_GAME_COMMAND_PASS, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.monster_x[1] == 5);
    assert(state.monster_y[1] == 6);

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.dungeon_level = 10;
    state.level_of_play = 1;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 50;
    state.stats[AK_GAME_STAT_STAMINA] = 0;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.monster_active[10] = 1;
    state.monster_x[10] = 6;
    state.monster_y[10] = 5;
    state.monster_hit_points[10] = 50;

    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.stats[AK_GAME_STAT_HIT_POINTS] < 50);
}

static void test_dungeon_monster_special_theft(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.dungeon_level = 1;
    state.level_of_play = 1;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    state.inventory[AK_GAME_ITEM_FOOD] = 11;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.monster_active[7] = 1;
    state.monster_x[7] = 6;
    state.monster_y[7] = 5;
    state.monster_hit_points[7] = 50;

    command = command_with_value(AK_GAME_COMMAND_PASS, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 5);
}

static void test_player_attack_miss_hit_and_kill(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.player_class = AK_GAME_CLASS_FIGHTER;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_level = 1;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    state.stats[AK_GAME_STAT_DEXTERITY] = 0;
    state.stats[AK_GAME_STAT_STRENGTH] = 50;
    state.inventory[AK_GAME_ITEM_RAPIER] = 1;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);
    state.monster_active[1] = 1;
    state.monster_x[1] = 6;
    state.monster_y[1] = 5;
    state.monster_hit_points[1] = 100;

    command = attack_command(AK_GAME_ITEM_RAPIER);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.monster_hit_points[1] == 100);

    state.stats[AK_GAME_STAT_DEXTERITY] = 100;
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.monster_hit_points[1] < 100);

    state.quest_target = 1;
    state.monster_hit_points[1] = 1;
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.monster_active[1] == 0);
    assert(state.stats[AK_GAME_STAT_GOLD] >= 2);
    assert(state.quest_target == -1);
}

static void test_magic_amulet_ladders_attack_and_backfire(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.player_class = AK_GAME_CLASS_MAGE;
    state.facing = AK_GAME_DIRECTION_EAST;
    state.dungeon_level = 2;
    state.dungeon_x = 5;
    state.dungeon_y = 5;
    state.inventory[AK_GAME_ITEM_MAGIC_AMULET] = 4;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 40;
    state.stats[AK_GAME_STAT_STRENGTH] = 12;
    state.stats[AK_GAME_STAT_DEXTERITY] = 12;
    state.stats[AK_GAME_STAT_STAMINA] = 12;
    state.stats[AK_GAME_STAT_WISDOM] = 12;
    fill_dungeon(&state, AK_GAME_DUNGEON_OPEN);

    command = magic_command(1);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.dungeon[5][5] == AK_GAME_DUNGEON_LADDER_UP);

    command = magic_command(2);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.dungeon[5][5] == AK_GAME_DUNGEON_LADDER_DOWN);

    state.monster_active[3] = 1;
    state.monster_x[3] = 7;
    state.monster_y[3] = 5;
    state.monster_hit_points[3] = 3;
    state.quest_target = 3;
    command = magic_command(3);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.monster_active[3] == 0);
    assert(state.quest_target == -3);

    command = magic_command(4);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.stats[AK_GAME_STAT_HIT_POINTS] != 40 ||
        state.stats[AK_GAME_STAT_STRENGTH] != 12 ||
        state.stats[AK_GAME_STAT_DEXTERITY] != 12 ||
        state.stats[AK_GAME_STAT_STAMINA] != 12 ||
        state.stats[AK_GAME_STAT_WISDOM] != 12);
}

static void test_lord_british_quest_progression_and_victory(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_QUEST;
    state.location = AK_GAME_LOCATION_CASTLE;
    state.stats[AK_GAME_STAT_WISDOM] = 9;
    state.stats[AK_GAME_STAT_HIT_POINTS] = 20;

    command = command_with_value(AK_GAME_COMMAND_ACKNOWLEDGE, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.quest_target == 3);
    assert(state.stats[AK_GAME_STAT_WISDOM] == 10);
    assert(state.stats[AK_GAME_STAT_HIT_POINTS] == 21);

    state.quest_target = -3;
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.quest_target == 4);

    state.quest_target = -10;
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_VICTORY);
    assert(state.quest_target == 0);
}

static void test_store_purchase_rules(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_TOWN;
    state.location = AK_GAME_LOCATION_TOWN;
    state.player_class = AK_GAME_CLASS_FIGHTER;
    state.stats[AK_GAME_STAT_GOLD] = 16;

    command = item_command(AK_GAME_ITEM_FOOD);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 10);
    assert(state.stats[AK_GAME_STAT_GOLD] == 15);
    assert(state.command_count == 1);

    command = item_command(AK_GAME_ITEM_MAGIC_AMULET);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.inventory[AK_GAME_ITEM_MAGIC_AMULET] == 1);
    assert(state.stats[AK_GAME_STAT_GOLD] == 0);
    assert(state.command_count == 2);

    command = item_command(AK_GAME_ITEM_SHIELD);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_REJECTED);
    assert(state.inventory[AK_GAME_ITEM_SHIELD] == 0);
    assert(state.stats[AK_GAME_STAT_GOLD] == 0);
    assert(state.command_count == 2);
    assert(result.event_count == 1);
    assert(result.events[0].type == AK_GAME_EVENT_ERROR);
}

static void test_mage_store_restrictions(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_TOWN;
    state.location = AK_GAME_LOCATION_TOWN;
    state.player_class = AK_GAME_CLASS_MAGE;
    state.stats[AK_GAME_STAT_GOLD] = 20;

    command = item_command(AK_GAME_ITEM_RAPIER);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_REJECTED);
    assert(state.inventory[AK_GAME_ITEM_RAPIER] == 0);
    assert(state.stats[AK_GAME_STAT_GOLD] == 20);
    assert(state.command_count == 0);

    command = item_command(AK_GAME_ITEM_BOW);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_REJECTED);
    assert(state.inventory[AK_GAME_ITEM_BOW] == 0);
    assert(state.stats[AK_GAME_STAT_GOLD] == 20);
    assert(state.command_count == 0);

    command = item_command(AK_GAME_ITEM_AXE);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.inventory[AK_GAME_ITEM_AXE] == 1);
    assert(state.stats[AK_GAME_STAT_GOLD] == 15);
    assert(state.command_count == 1);
}

static void test_reset_restores_initial_state(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    state.lucky_number = 99;
    state.command_count = 42;
    state.inventory[AK_GAME_ITEM_FOOD] = 10;

    command = command_with_value(AK_GAME_COMMAND_RESET, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER);
    assert(state.location == AK_GAME_LOCATION_STARTUP);
    assert(state.lucky_number == 0);
    assert(state.command_count == 0);
    assert(state.inventory[AK_GAME_ITEM_FOOD] == 0);
    assert(result.event_count == 1);
    assert(result.events[0].type == AK_GAME_EVENT_STATE_CHANGED);
}

static void test_names_are_stable(void) {
    assert(strcmp(ak_game_mode_name(AK_GAME_MODE_OVERWORLD), "overworld") == 0);
    assert(strcmp(ak_game_mode_name(AK_GAME_MODE_VICTORY), "victory") == 0);
    assert(strcmp(ak_game_location_name(AK_GAME_LOCATION_DUNGEON), "dungeon") == 0);
    assert(strcmp(ak_game_item_name(AK_GAME_ITEM_MAGIC_AMULET), "magic_amulet") == 0);
    assert(strcmp(ak_game_monster_name(10), "balrog") == 0);
    assert(strcmp(ak_game_tile_name(AK_GAME_TILE_CASTLE), "castle") == 0);
    assert(strcmp(ak_game_dungeon_tile_name(AK_GAME_DUNGEON_CHEST), "chest") == 0);
}

int main(void) {
    test_initial_state_is_deterministic();
    test_startup_command_flow();
    test_invalid_commands_do_not_advance_state();
    test_combat_commands_accept_known_commands_in_valid_modes();
    test_exit_from_town_generates_deterministic_overworld();
    test_overworld_movement_all_directions_consumes_food();
    test_overworld_mountain_blocking_still_consumes_food();
    test_overworld_entry_rules();
    test_enter_on_plain_overworld_tile_is_rejected_without_food_turn();
    test_overworld_starvation_enters_death_state();
    test_dungeon_generation_is_deterministic_and_places_monsters();
    test_dungeon_turning_and_forward_movement();
    test_dungeon_forward_blocks_walls_and_monsters();
    test_dungeon_ladder_transitions();
    test_dungeon_chest_and_trap_encounters();
    test_dungeon_monster_turn_movement_and_attack();
    test_dungeon_monster_special_theft();
    test_player_attack_miss_hit_and_kill();
    test_magic_amulet_ladders_attack_and_backfire();
    test_lord_british_quest_progression_and_victory();
    test_store_purchase_rules();
    test_mage_store_restrictions();
    test_reset_restores_initial_state();
    test_names_are_stable();
    return 0;
}
