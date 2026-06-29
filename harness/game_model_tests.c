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
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_UNSUPPORTED_RULE);
    assert(state.level_of_play == 5);
    assert(state.mode == AK_GAME_MODE_CHARACTER_REVIEW);
    assert(state.command_count == 2);
    assert(result.event_count == 2);
    assert(result.events[0].type == AK_GAME_EVENT_MODE_CHANGED);
    assert(result.events[1].type == AK_GAME_EVENT_RULE_UNIMPLEMENTED);

    command = command_with_value(AK_GAME_COMMAND_ACCEPT_CHARACTER, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.mode == AK_GAME_MODE_CLASS_SELECT);
    assert(state.command_count == 3);

    command = class_command(AK_GAME_CLASS_MAGE);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_UNSUPPORTED_RULE);
    assert(state.player_class == AK_GAME_CLASS_MAGE);
    assert(state.mode == AK_GAME_MODE_OVERWORLD);
    assert(state.location == AK_GAME_LOCATION_OVERWORLD);
    assert(state.command_count == 4);
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

static void test_rule_placeholders_accept_known_commands_in_valid_modes(void) {
    AkGameState state;
    AkGameCommandResult result;
    AkGameCommand command;

    ak_game_init(&state);
    state.mode = AK_GAME_MODE_OVERWORLD;
    state.location = AK_GAME_LOCATION_OVERWORLD;

    command = command_with_value(AK_GAME_COMMAND_MOVE_EAST, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_UNSUPPORTED_RULE);
    assert(state.command_count == 1);
    assert(result.event_count == 2);
    assert(result.events[0].type == AK_GAME_EVENT_COMMAND_ACCEPTED);
    assert(result.events[1].type == AK_GAME_EVENT_RULE_UNIMPLEMENTED);

    state.mode = AK_GAME_MODE_TOWN;
    state.location = AK_GAME_LOCATION_TOWN;
    command = item_command(AK_GAME_ITEM_AXE);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_UNSUPPORTED_RULE);
    assert(state.command_count == 2);
    assert(result.events[0].command == AK_GAME_COMMAND_BUY_ITEM);
    assert(result.events[0].value == AK_GAME_ITEM_AXE);

    state.mode = AK_GAME_MODE_DUNGEON;
    state.location = AK_GAME_LOCATION_DUNGEON;
    command = command_with_value(AK_GAME_COMMAND_ATTACK, AK_GAME_ITEM_RAPIER);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_UNSUPPORTED_RULE);
    assert(state.command_count == 3);
    assert(result.events[0].command == AK_GAME_COMMAND_ATTACK);

    command = command_with_value(AK_GAME_COMMAND_ACKNOWLEDGE, 0);
    assert(ak_game_apply_command(&state, &command, &result) == AK_GAME_RESULT_OK);
    assert(state.command_count == 4);
    assert(result.event_count == 1);
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
    assert(strcmp(ak_game_location_name(AK_GAME_LOCATION_DUNGEON), "dungeon") == 0);
    assert(strcmp(ak_game_item_name(AK_GAME_ITEM_MAGIC_AMULET), "magic_amulet") == 0);
}

int main(void) {
    test_initial_state_is_deterministic();
    test_startup_command_flow();
    test_invalid_commands_do_not_advance_state();
    test_rule_placeholders_accept_known_commands_in_valid_modes();
    test_reset_restores_initial_state();
    test_names_are_stable();
    return 0;
}
