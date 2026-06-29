#include "ak_game.h"

#include <string.h>

static void push_event(
    AkGameCommandResult *result,
    AkGameEventType type,
    const AkGameState *state,
    AkGameCommandType command,
    int value
) {
    AkGameEvent *event;

    if (result == NULL || result->event_count >= AK_GAME_MAX_EVENTS) {
        return;
    }

    event = &result->events[result->event_count];
    event->type = type;
    event->mode = state != NULL ? state->mode : AK_GAME_MODE_STARTUP_LUCKY_NUMBER;
    event->location = state != NULL ? state->location : AK_GAME_LOCATION_STARTUP;
    event->command = command;
    event->value = value;
    result->event_count++;
}

static void set_mode(
    AkGameState *state,
    AkGameCommandResult *result,
    AkGameMode mode,
    AkGameCommandType command
) {
    if (state->mode != mode) {
        state->mode = mode;
        push_event(result, AK_GAME_EVENT_MODE_CHANGED, state, command, (int)mode);
    }
}

static void set_location(
    AkGameState *state,
    AkGameCommandResult *result,
    AkGameLocation location,
    AkGameCommandType command
) {
    if (state->location != location) {
        state->location = location;
        push_event(result, AK_GAME_EVENT_LOCATION_CHANGED, state, command, (int)location);
    }
}

static AkGameResultCode finish(
    AkGameCommandResult *result,
    AkGameResultCode code
) {
    if (result != NULL) {
        result->code = code;
    }
    return code;
}

static int is_overworld_move(AkGameCommandType type) {
    return type == AK_GAME_COMMAND_MOVE_NORTH ||
        type == AK_GAME_COMMAND_MOVE_EAST ||
        type == AK_GAME_COMMAND_MOVE_SOUTH ||
        type == AK_GAME_COMMAND_MOVE_WEST;
}

void ak_game_init(AkGameState *state) {
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->mode = AK_GAME_MODE_STARTUP_LUCKY_NUMBER;
    state->location = AK_GAME_LOCATION_STARTUP;
    state->facing = AK_GAME_DIRECTION_EAST;
}

void ak_game_result_init(AkGameCommandResult *result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->code = AK_GAME_RESULT_OK;
}

AkGameResultCode ak_game_apply_command(
    AkGameState *state,
    const AkGameCommand *command,
    AkGameCommandResult *result
) {
    AkGameCommandType type;

    if (result != NULL) {
        ak_game_result_init(result);
    }

    if (state == NULL || command == NULL) {
        return finish(result, AK_GAME_RESULT_INVALID_ARGUMENT);
    }

    type = command->type;

    if (type == AK_GAME_COMMAND_RESET) {
        ak_game_init(state);
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, 0);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_SET_LUCKY_NUMBER) {
        if (state->mode != AK_GAME_MODE_STARTUP_LUCKY_NUMBER) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->lucky_number = command->value;
        state->command_count++;
        set_mode(state, result, AK_GAME_MODE_STARTUP_LEVEL, type);
        push_event(result, AK_GAME_EVENT_PROMPT_CHANGED, state, type, 0);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_SET_LEVEL) {
        if (state->mode != AK_GAME_MODE_STARTUP_LEVEL || command->value < 1 || command->value > 10) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, command->value);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->level_of_play = command->value;
        state->command_count++;
        set_mode(state, result, AK_GAME_MODE_CHARACTER_REVIEW, type);
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, command->value);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (type == AK_GAME_COMMAND_SELECT_CLASS) {
        if (state->mode != AK_GAME_MODE_CLASS_SELECT ||
            (command->player_class != AK_GAME_CLASS_FIGHTER && command->player_class != AK_GAME_CLASS_MAGE)) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, command->player_class);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->player_class = command->player_class;
        state->command_count++;
        set_mode(state, result, AK_GAME_MODE_OVERWORLD, type);
        set_location(state, result, AK_GAME_LOCATION_OVERWORLD, type);
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, command->player_class);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (type == AK_GAME_COMMAND_ACCEPT_CHARACTER) {
        if (state->mode != AK_GAME_MODE_CHARACTER_REVIEW) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->command_count++;
        set_mode(state, result, AK_GAME_MODE_CLASS_SELECT, type);
        push_event(result, AK_GAME_EVENT_PROMPT_CHANGED, state, type, 0);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_REROLL_CHARACTER) {
        if (state->mode != AK_GAME_MODE_CHARACTER_REVIEW) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->command_count++;
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, 0);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (is_overworld_move(type)) {
        if (state->mode != AK_GAME_MODE_OVERWORLD) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, 0);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (type == AK_GAME_COMMAND_FORWARD ||
        type == AK_GAME_COMMAND_TURN_LEFT ||
        type == AK_GAME_COMMAND_TURN_RIGHT ||
        type == AK_GAME_COMMAND_TURN_AROUND) {
        if (state->mode != AK_GAME_MODE_DUNGEON) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, 0);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (type == AK_GAME_COMMAND_ENTER || type == AK_GAME_COMMAND_EXIT || type == AK_GAME_COMMAND_PASS) {
        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, 0);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (type == AK_GAME_COMMAND_BUY_ITEM) {
        if (state->mode != AK_GAME_MODE_TOWN) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        if (command->item < AK_GAME_ITEM_FOOD || command->item > AK_GAME_ITEM_MAGIC_AMULET) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, command->item);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, command->item);
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, command->item);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (type == AK_GAME_COMMAND_ATTACK || type == AK_GAME_COMMAND_CAST_MAGIC) {
        if (state->mode != AK_GAME_MODE_COMBAT && state->mode != AK_GAME_MODE_DUNGEON) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, command->value);
        push_event(result, AK_GAME_EVENT_RULE_UNIMPLEMENTED, state, type, command->value);
        return finish(result, AK_GAME_RESULT_UNSUPPORTED_RULE);
    }

    if (type == AK_GAME_COMMAND_ACKNOWLEDGE) {
        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
        return finish(result, AK_GAME_RESULT_OK);
    }

    push_event(result, AK_GAME_EVENT_ERROR, state, type, 0);
    return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
}

const char *ak_game_mode_name(AkGameMode mode) {
    switch (mode) {
        case AK_GAME_MODE_STARTUP_LUCKY_NUMBER: return "startup_lucky_number";
        case AK_GAME_MODE_STARTUP_LEVEL: return "startup_level";
        case AK_GAME_MODE_CHARACTER_REVIEW: return "character_review";
        case AK_GAME_MODE_CLASS_SELECT: return "class_select";
        case AK_GAME_MODE_OVERWORLD: return "overworld";
        case AK_GAME_MODE_TOWN: return "town";
        case AK_GAME_MODE_DUNGEON: return "dungeon";
        case AK_GAME_MODE_COMBAT: return "combat";
        case AK_GAME_MODE_DEATH: return "death";
        case AK_GAME_MODE_QUEST: return "quest";
    }
    return "unknown";
}

const char *ak_game_location_name(AkGameLocation location) {
    switch (location) {
        case AK_GAME_LOCATION_STARTUP: return "startup";
        case AK_GAME_LOCATION_OVERWORLD: return "overworld";
        case AK_GAME_LOCATION_TOWN: return "town";
        case AK_GAME_LOCATION_DUNGEON: return "dungeon";
        case AK_GAME_LOCATION_CASTLE: return "castle";
        case AK_GAME_LOCATION_DEATH: return "death";
    }
    return "unknown";
}

const char *ak_game_item_name(AkGameItem item) {
    switch (item) {
        case AK_GAME_ITEM_FOOD: return "food";
        case AK_GAME_ITEM_RAPIER: return "rapier";
        case AK_GAME_ITEM_AXE: return "axe";
        case AK_GAME_ITEM_SHIELD: return "shield";
        case AK_GAME_ITEM_BOW: return "bow_and_arrows";
        case AK_GAME_ITEM_MAGIC_AMULET: return "magic_amulet";
    }
    return "unknown";
}
