#include "ak_game.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const int AK_GAME_ITEM_PRICES[AK_GAME_ITEM_COUNT] = {
    1,
    8,
    5,
    6,
    3,
    15
};

static const char *AK_GAME_MONSTER_NAMES[AK_GAME_MONSTER_COUNT + 1] = {
    "",
    "skeleton",
    "thief",
    "giant_rat",
    "orc",
    "viper",
    "carrion_crawler",
    "gremlin",
    "mimic",
    "daemon",
    "balrog"
};

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

static int is_valid_item(AkGameItem item) {
    return item >= AK_GAME_ITEM_FOOD && item <= AK_GAME_ITEM_MAGIC_AMULET;
}

static int is_weapon_item(AkGameItem item) {
    return item == AK_GAME_ITEM_RAPIER ||
        item == AK_GAME_ITEM_AXE ||
        item == AK_GAME_ITEM_SHIELD ||
        item == AK_GAME_ITEM_BOW;
}

static int class_can_buy_item(AkGameClass player_class, AkGameItem item) {
    if (player_class == AK_GAME_CLASS_MAGE &&
        (item == AK_GAME_ITEM_RAPIER || item == AK_GAME_ITEM_BOW)) {
        return 0;
    }

    return 1;
}

static void generate_character_qualities(AkGameState *state) {
    int i;

    for (i = 0; i < AK_GAME_STAT_COUNT; i++) {
        double value = ak_random_rnd(&state->random, 1.0);
        state->stats[i] = (int)(sqrt(value) * 21.0 + 4.0);
    }
}

static void apply_purchase(AkGameState *state, AkGameItem item) {
    if (item == AK_GAME_ITEM_FOOD) {
        state->inventory[item] += 10;
        state->food_tenths += 100;
    } else {
        state->inventory[item]++;
    }

    state->stats[AK_GAME_STAT_GOLD] -= AK_GAME_ITEM_PRICES[item];
}

static int whole_food_from_tenths(int food_tenths) {
    if (food_tenths < 0) {
        return -1;
    }
    return food_tenths / 10;
}

static void sync_food_from_inventory(AkGameState *state) {
    if (state->inventory[AK_GAME_ITEM_FOOD] != whole_food_from_tenths(state->food_tenths)) {
        state->food_tenths = state->inventory[AK_GAME_ITEM_FOOD] * 10;
    }
}

static void sync_inventory_from_food(AkGameState *state) {
    state->inventory[AK_GAME_ITEM_FOOD] = whole_food_from_tenths(state->food_tenths);
}

static int overworld_tile_at(const AkGameState *state, int x, int y) {
    if (x < 0 || x >= AK_GAME_OVERWORLD_SIZE || y < 0 || y >= AK_GAME_OVERWORLD_SIZE) {
        return AK_GAME_TILE_MOUNTAIN;
    }

    return state->overworld[x][y];
}

static void generate_overworld(AkGameState *state) {
    int x;
    int y;
    int castle_x;
    int castle_y;

    ak_random_rnd(&state->random, (double)-abs(state->lucky_number));

    for (x = 0; x < AK_GAME_OVERWORLD_SIZE; x++) {
        state->overworld[x][0] = AK_GAME_TILE_MOUNTAIN;
        state->overworld[0][x] = AK_GAME_TILE_MOUNTAIN;
        state->overworld[x][AK_GAME_OVERWORLD_SIZE - 1] = AK_GAME_TILE_MOUNTAIN;
        state->overworld[AK_GAME_OVERWORLD_SIZE - 1][x] = AK_GAME_TILE_MOUNTAIN;
    }

    for (x = 1; x < AK_GAME_OVERWORLD_SIZE - 1; x++) {
        for (y = 1; y < AK_GAME_OVERWORLD_SIZE - 1; y++) {
            double value = ak_random_rnd(&state->random, 1.0);
            int tile = (int)(pow(value, 5.0) * 4.5);

            if (tile == AK_GAME_TILE_TOWN &&
                ak_random_chance_greater_than(&state->random, 0.5)) {
                tile = AK_GAME_TILE_OPEN;
            }

            state->overworld[x][y] = tile;
        }
    }

    castle_x = ak_random_int(&state->random, 19.0, 1.0);
    castle_y = ak_random_int(&state->random, 19.0, 1.0);
    state->overworld[castle_x][castle_y] = AK_GAME_TILE_CASTLE;

    state->overworld_x = ak_random_int(&state->random, 19.0, 1.0);
    state->overworld_y = ak_random_int(&state->random, 19.0, 1.0);
    state->overworld[state->overworld_x][state->overworld_y] = AK_GAME_TILE_TOWN;
}

static void consume_overworld_food(AkGameState *state) {
    state->food_tenths -= 10;
    sync_inventory_from_food(state);
    if (state->food_tenths < 0) {
        state->stats[AK_GAME_STAT_HIT_POINTS] = 0;
        state->mode = AK_GAME_MODE_DEATH;
        state->location = AK_GAME_LOCATION_DEATH;
    }
}

static void consume_dungeon_food(AkGameState *state) {
    state->food_tenths -= 1;
    sync_inventory_from_food(state);
    if (state->food_tenths < 0) {
        state->stats[AK_GAME_STAT_HIT_POINTS] = 0;
        state->mode = AK_GAME_MODE_DEATH;
        state->location = AK_GAME_LOCATION_DEATH;
    }
}

static int has_generated_overworld(const AkGameState *state) {
    int x;
    int y;

    for (x = 0; x < AK_GAME_OVERWORLD_SIZE; x++) {
        for (y = 0; y < AK_GAME_OVERWORLD_SIZE; y++) {
            if (state->overworld[x][y] != AK_GAME_TILE_OPEN) {
                return 1;
            }
        }
    }
    return 0;
}

static void return_to_overworld(AkGameState *state, AkGameCommandResult *result, AkGameCommandType command) {
    if (!has_generated_overworld(state)) {
        generate_overworld(state);
    }
    set_mode(state, result, AK_GAME_MODE_OVERWORLD, command);
    set_location(state, result, AK_GAME_LOCATION_OVERWORLD, command);
}

static void move_overworld(
    AkGameState *state,
    AkGameCommandResult *result,
    AkGameCommandType command,
    int dx,
    int dy
) {
    int next_x = state->overworld_x + dx;
    int next_y = state->overworld_y + dy;

    if (overworld_tile_at(state, next_x, next_y) != AK_GAME_TILE_MOUNTAIN) {
        state->overworld_x = next_x;
        state->overworld_y = next_y;
    }

    state->command_count++;
    consume_overworld_food(state);
    push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, command, overworld_tile_at(state, state->overworld_x, state->overworld_y));
    push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, 0);
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

static int find_first_owned_item(const AkGameState *state, AkRandom *random) {
    int attempts;

    for (attempts = 0; attempts < 32; attempts++) {
        int item = ak_random_int(random, 6.0, 0.0);
        if (item >= AK_GAME_ITEM_FOOD &&
            item < AK_GAME_ITEM_COUNT &&
            state->inventory[item] > 0) {
            return item;
        }
    }

    for (attempts = 0; attempts < AK_GAME_ITEM_COUNT; attempts++) {
        if (state->inventory[attempts] > 0) {
            return attempts;
        }
    }

    return -1;
}

static int dungeon_tile_at(const AkGameState *state, int x, int y) {
    if (x < 0 || x >= AK_GAME_DUNGEON_SIZE || y < 0 || y >= AK_GAME_DUNGEON_SIZE) {
        return AK_GAME_DUNGEON_WALL;
    }

    return state->dungeon[x][y];
}

static void clear_monsters(AkGameState *state) {
    int monster;

    for (monster = 0; monster <= AK_GAME_MONSTER_COUNT; monster++) {
        state->monster_active[monster] = 0;
        state->monster_x[monster] = 0;
        state->monster_y[monster] = 0;
        state->monster_hit_points[monster] = 0;
    }
}

static void place_dungeon_monsters(AkGameState *state) {
    int monster;

    clear_monsters(state);
    for (monster = 1; monster <= AK_GAME_MONSTER_COUNT; monster++) {
        state->monster_hit_points[monster] = monster + 3 + state->dungeon_level;
        if (monster - 2 > state->dungeon_level ||
            ak_random_chance_greater_than(&state->random, 0.4)) {
            continue;
        }

        do {
            state->monster_x[monster] = ak_random_int(&state->random, 9.0, 1.0);
            state->monster_y[monster] = ak_random_int(&state->random, 9.0, 1.0);
        } while (dungeon_tile_at(state, state->monster_x[monster], state->monster_y[monster]) != AK_GAME_DUNGEON_OPEN ||
            (state->monster_x[monster] == state->dungeon_x && state->monster_y[monster] == state->dungeon_y) ||
            active_monster_at(state, state->monster_x[monster], state->monster_y[monster]) != 0);

        state->monster_active[monster] = 1;
        state->monster_hit_points[monster] = monster * 2 + state->dungeon_level * 2 * state->level_of_play;
    }
}

static void generate_dungeon(AkGameState *state) {
    int x;
    int y;
    double seed = (double)-abs(state->lucky_number) -
        (double)state->overworld_x * 10.0 -
        (double)state->overworld_y * 1000.0 +
        (double)state->dungeon_level * 31.4;

    ak_random_rnd(&state->random, seed);

    for (x = 0; x < AK_GAME_DUNGEON_SIZE; x++) {
        for (y = 0; y < AK_GAME_DUNGEON_SIZE; y++) {
            state->dungeon[x][y] = AK_GAME_DUNGEON_OPEN;
        }
    }

    for (x = 0; x < AK_GAME_DUNGEON_SIZE; x++) {
        state->dungeon[x][0] = AK_GAME_DUNGEON_WALL;
        state->dungeon[x][AK_GAME_DUNGEON_SIZE - 1] = AK_GAME_DUNGEON_WALL;
        state->dungeon[0][x] = AK_GAME_DUNGEON_WALL;
        state->dungeon[AK_GAME_DUNGEON_SIZE - 1][x] = AK_GAME_DUNGEON_WALL;
    }

    for (x = 2; x <= 8; x += 2) {
        for (y = 1; y <= 9; y++) {
            state->dungeon[x][y] = AK_GAME_DUNGEON_WALL;
            state->dungeon[y][x] = AK_GAME_DUNGEON_WALL;
        }
    }

    for (x = 2; x <= 8; x += 2) {
        for (y = 1; y <= 9; y += 2) {
            if (ak_random_chance_greater_than(&state->random, 0.95)) {
                state->dungeon[x][y] = AK_GAME_DUNGEON_TRAP;
            }
            if (ak_random_chance_greater_than(&state->random, 0.95)) {
                state->dungeon[y][x] = AK_GAME_DUNGEON_TRAP;
            }
            if (ak_random_chance_greater_than(&state->random, 0.6)) {
                state->dungeon[y][x] = AK_GAME_DUNGEON_SECRET_DOOR;
            }
            if (ak_random_chance_greater_than(&state->random, 0.6)) {
                state->dungeon[x][y] = AK_GAME_DUNGEON_SECRET_DOOR;
            }
            if (ak_random_chance_greater_than(&state->random, 0.6)) {
                state->dungeon[x][y] = AK_GAME_DUNGEON_HIDDEN_DOOR;
            }
            if (ak_random_chance_greater_than(&state->random, 0.6)) {
                state->dungeon[y][x] = AK_GAME_DUNGEON_HIDDEN_DOOR;
            }
            if (ak_random_chance_greater_than(&state->random, 0.97)) {
                state->dungeon[y][x] = AK_GAME_DUNGEON_LADDER_DOWN_ALT;
            }
            if (ak_random_chance_greater_than(&state->random, 0.97)) {
                state->dungeon[x][y] = AK_GAME_DUNGEON_LADDER_DOWN_ALT;
            }
            if (ak_random_chance_greater_than(&state->random, 0.94)) {
                state->dungeon[x][y] = AK_GAME_DUNGEON_CHEST;
            }
            if (ak_random_chance_greater_than(&state->random, 0.94)) {
                state->dungeon[y][x] = AK_GAME_DUNGEON_CHEST;
            }
        }
    }

    state->dungeon[2][1] = AK_GAME_DUNGEON_OPEN;
    if (state->dungeon_level % 2 == 0) {
        state->dungeon[7][3] = AK_GAME_DUNGEON_LADDER_DOWN;
        state->dungeon[3][7] = AK_GAME_DUNGEON_LADDER_UP;
    } else {
        state->dungeon[7][3] = AK_GAME_DUNGEON_LADDER_UP;
        state->dungeon[3][7] = AK_GAME_DUNGEON_LADDER_DOWN;
    }
    if (state->dungeon_level == 1) {
        state->dungeon[1][1] = AK_GAME_DUNGEON_LADDER_UP;
        state->dungeon[7][3] = AK_GAME_DUNGEON_OPEN;
    }

    place_dungeon_monsters(state);
}

static void enter_dungeon(AkGameState *state, AkGameCommandResult *result, AkGameCommandType command) {
    state->dungeon_level = 1;
    state->dungeon_x = 1;
    state->dungeon_y = 1;
    state->facing = AK_GAME_DIRECTION_EAST;
    generate_dungeon(state);
    set_mode(state, result, AK_GAME_MODE_DUNGEON, command);
    set_location(state, result, AK_GAME_LOCATION_DUNGEON, command);
}

static void resolve_dungeon_arrival(AkGameState *state, AkGameCommandResult *result, AkGameCommandType command) {
    int tile = dungeon_tile_at(state, state->dungeon_x, state->dungeon_y);

    if (tile == AK_GAME_DUNGEON_TRAP) {
        int damage = ak_random_int(&state->random, (double)state->dungeon_level, 3.0);
        state->stats[AK_GAME_STAT_HIT_POINTS] -= damage;
        state->dungeon_level++;
        generate_dungeon(state);
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, AK_GAME_DUNGEON_TRAP);
        if (state->stats[AK_GAME_STAT_HIT_POINTS] <= 0) {
            state->mode = AK_GAME_MODE_DEATH;
            state->location = AK_GAME_LOCATION_DEATH;
            push_event(result, AK_GAME_EVENT_LOCATION_CHANGED, state, command, AK_GAME_LOCATION_DEATH);
        }
    } else if (tile == AK_GAME_DUNGEON_CHEST) {
        int gold = ak_random_int(&state->random, 5.0 * (double)state->dungeon_level, (double)state->dungeon_level);
        int item = ak_random_int(&state->random, 6.0, 0.0);
        state->dungeon[state->dungeon_x][state->dungeon_y] = AK_GAME_DUNGEON_OPEN;
        state->stats[AK_GAME_STAT_GOLD] += gold;
        if (item >= AK_GAME_ITEM_FOOD && item < AK_GAME_ITEM_COUNT) {
            state->inventory[item]++;
            if (item == AK_GAME_ITEM_FOOD) {
                state->food_tenths += 10;
            }
        }
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, AK_GAME_DUNGEON_CHEST);
    }
}

static int dungeon_cell_blocks_monster(const AkGameState *state, int x, int y) {
    int tile = dungeon_tile_at(state, x, y);

    return tile == AK_GAME_DUNGEON_WALL ||
        tile == AK_GAME_DUNGEON_TRAP ||
        active_monster_at(state, x, y) != 0;
}

static int dungeon_adjacent_to_player(const AkGameState *state, int monster) {
    int dx = state->dungeon_x - state->monster_x[monster];
    int dy = state->dungeon_y - state->monster_y[monster];

    return dx * dx + dy * dy < 2;
}

static int sign_of(int value) {
    if (value > 0) {
        return 1;
    }
    if (value < 0) {
        return -1;
    }
    return 0;
}

static void run_dungeon_monster_turns(AkGameState *state, AkGameCommandResult *result, AkGameCommandType command) {
    int monster;

    for (monster = 1; monster <= AK_GAME_MONSTER_COUNT; monster++) {
        int dx;
        int dy;
        int flee;
        int moved = 0;

        if (!state->monster_active[monster]) {
            continue;
        }

        if (dungeon_adjacent_to_player(state, monster)) {
            if ((monster == 2 || monster == 7) &&
                ak_random_rnd(&state->random, 1.0) >= 0.5) {
                if (monster == 7) {
                    state->food_tenths = (state->food_tenths / 20) * 10;
                    sync_inventory_from_food(state);
                    push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, monster);
                } else {
                    int item = find_first_owned_item(state, &state->random);
                    if (item >= 0) {
                        state->inventory[item]--;
                        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, item);
                    }
                }
                continue;
            }

            int shield_bonus = state->inventory[AK_GAME_ITEM_SHIELD] > 0 ? 1 : 0;
            double attack = ak_random_rnd(&state->random, 1.0) * 20.0 -
                (double)shield_bonus -
                (double)state->stats[AK_GAME_STAT_STAMINA] +
                (double)monster +
                (double)state->dungeon_level;

            if (attack >= 0.0) {
                int damage = ak_random_int(&state->random, (double)monster, (double)state->dungeon_level);
                state->stats[AK_GAME_STAT_HIT_POINTS] -= damage;
                push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, monster);
                if (state->stats[AK_GAME_STAT_HIT_POINTS] <= 0) {
                    state->mode = AK_GAME_MODE_DEATH;
                    state->location = AK_GAME_LOCATION_DEATH;
                    push_event(result, AK_GAME_EVENT_LOCATION_CHANGED, state, command, AK_GAME_LOCATION_DEATH);
                    return;
                }
            }
            continue;
        }

        if (monster == 8) {
            continue;
        }

        flee = state->monster_hit_points[monster] < state->dungeon_level * state->level_of_play;
        dx = sign_of(state->dungeon_x - state->monster_x[monster]);
        dy = sign_of(state->dungeon_y - state->monster_y[monster]);
        if (flee) {
            dx = -dx;
            dy = -dy;
        }

        if (dy != 0 && !dungeon_cell_blocks_monster(state, state->monster_x[monster], state->monster_y[monster] + dy)) {
            state->monster_y[monster] += dy;
            moved = 1;
        } else if (dx != 0 && !dungeon_cell_blocks_monster(state, state->monster_x[monster] + dx, state->monster_y[monster])) {
            state->monster_x[monster] += dx;
            moved = 1;
        }

        if (moved) {
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, monster);
        } else if (flee && dungeon_adjacent_to_player(state, monster)) {
            state->monster_hit_points[monster] += monster + state->dungeon_level;
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, monster);
        }
    }
}

static void run_dungeon_post_command(AkGameState *state, AkGameCommandResult *result, AkGameCommandType command) {
    consume_dungeon_food(state);
    if (state->mode == AK_GAME_MODE_DUNGEON) {
        run_dungeon_monster_turns(state, result, command);
    }
}

static int weapon_damage_base(AkGameItem item) {
    switch (item) {
        case AK_GAME_ITEM_RAPIER: return 10;
        case AK_GAME_ITEM_AXE: return 5;
        case AK_GAME_ITEM_SHIELD: return 1;
        case AK_GAME_ITEM_BOW: return 4;
        default: return 0;
    }
}

static int find_attack_target(const AkGameState *state, int ranged) {
    int distance;
    int limit = ranged ? 5 : 1;

    for (distance = 1; distance <= limit; distance++) {
        int x = state->dungeon_x + direction_dx(state->facing) * distance;
        int y = state->dungeon_y + direction_dy(state->facing) * distance;
        int monster;

        if (x < 1 || x > 9 || y < 0 || y > 9) {
            break;
        }

        monster = active_monster_at(state, x, y);
        if (monster > 0) {
            return monster;
        }
    }

    return 0;
}

static void kill_monster(AkGameState *state, int monster) {
    int reward = monster + state->dungeon_level;

    state->stats[AK_GAME_STAT_GOLD] += reward;
    state->dungeon_hit_point_reward += monster * state->dungeon_level / 2;
    state->monster_active[monster] = 0;
    state->monster_hit_points[monster] = 0;
    state->dungeon[state->monster_x[monster]][state->monster_y[monster]] = AK_GAME_DUNGEON_OPEN;
    if (monster == state->quest_target) {
        state->quest_target = -state->quest_target;
    }
}

static AkGameResultCode apply_attack(
    AkGameState *state,
    const AkGameCommand *command,
    AkGameCommandResult *result
) {
    AkGameItem item = command->item;
    int thrown_axe;
    int damage_base;
    int ranged;
    int monster;
    double hit_roll;
    int damage;

    if (!is_weapon_item(item) && is_valid_item((AkGameItem)command->value)) {
        item = (AkGameItem)command->value;
    }
    if (!is_weapon_item(item)) {
        item = AK_GAME_ITEM_FOOD;
    }

    thrown_axe = item == AK_GAME_ITEM_AXE && command->value == AK_GAME_ATTACK_STYLE_THROWN;
    damage_base = weapon_damage_base(item);
    if (is_weapon_item(item)) {
        if (state->inventory[item] < 1) {
            push_event(result, AK_GAME_EVENT_ERROR, state, AK_GAME_COMMAND_ATTACK, item);
            return AK_GAME_RESULT_REJECTED;
        }
        if (!class_can_buy_item(state->player_class, item)) {
            push_event(result, AK_GAME_EVENT_ERROR, state, AK_GAME_COMMAND_ATTACK, item);
            return AK_GAME_RESULT_REJECTED;
        }
    }

    ranged = item == AK_GAME_ITEM_BOW || thrown_axe;
    monster = find_attack_target(state, ranged);
    hit_roll = (double)state->stats[AK_GAME_STAT_DEXTERITY] -
        ak_random_rnd(&state->random, 1.0) * 25.0;

    state->command_count++;
    if (thrown_axe) {
        state->inventory[AK_GAME_ITEM_AXE]--;
    }
    push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, AK_GAME_COMMAND_ATTACK, item);

    if (monster < 1 || hit_roll < (double)(monster + state->dungeon_level)) {
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_ATTACK, 0);
        run_dungeon_post_command(state, result, AK_GAME_COMMAND_ATTACK);
        return AK_GAME_RESULT_OK;
    }

    damage = (int)(ak_random_rnd(&state->random, 1.0) * (double)damage_base +
        (double)state->stats[AK_GAME_STAT_STRENGTH] / 5.0);
    state->monster_hit_points[monster] -= damage;
    push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_ATTACK, monster);
    if (state->monster_hit_points[monster] < 1) {
        kill_monster(state, monster);
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_ATTACK, -monster);
    }

    run_dungeon_post_command(state, result, AK_GAME_COMMAND_ATTACK);
    return AK_GAME_RESULT_OK;
}

static AkGameResultCode apply_magic(
    AkGameState *state,
    const AkGameCommand *command,
    AkGameCommandResult *result
) {
    int choice = command->value;

    if (state->inventory[AK_GAME_ITEM_MAGIC_AMULET] < 1) {
        push_event(result, AK_GAME_EVENT_ERROR, state, AK_GAME_COMMAND_CAST_MAGIC, AK_GAME_ITEM_MAGIC_AMULET);
        return AK_GAME_RESULT_REJECTED;
    }

    if (state->player_class == AK_GAME_CLASS_FIGHTER) {
        choice = ak_random_int(&state->random, 4.0, 1.0);
    }
    if (choice < 1 || choice > 4) {
        push_event(result, AK_GAME_EVENT_ERROR, state, AK_GAME_COMMAND_CAST_MAGIC, choice);
        return AK_GAME_RESULT_INVALID_COMMAND;
    }

    if (ak_random_chance_greater_than(&state->random, 0.75)) {
        state->inventory[AK_GAME_ITEM_MAGIC_AMULET]--;
    }

    state->command_count++;
    push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, AK_GAME_COMMAND_CAST_MAGIC, choice);

    if (choice == 1) {
        state->dungeon[state->dungeon_x][state->dungeon_y] = AK_GAME_DUNGEON_LADDER_UP;
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_CAST_MAGIC, AK_GAME_DUNGEON_LADDER_UP);
        run_dungeon_post_command(state, result, AK_GAME_COMMAND_CAST_MAGIC);
        return AK_GAME_RESULT_OK;
    }
    if (choice == 2) {
        state->dungeon[state->dungeon_x][state->dungeon_y] = AK_GAME_DUNGEON_LADDER_DOWN;
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_CAST_MAGIC, AK_GAME_DUNGEON_LADDER_DOWN);
        run_dungeon_post_command(state, result, AK_GAME_COMMAND_CAST_MAGIC);
        return AK_GAME_RESULT_OK;
    }
    if (choice == 3) {
        int monster = find_attack_target(state, 1);
        if (monster > 0) {
            state->monster_hit_points[monster] -= 10 + state->dungeon_level;
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_CAST_MAGIC, monster);
            if (state->monster_hit_points[monster] < 1) {
                kill_monster(state, monster);
                push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_CAST_MAGIC, -monster);
            }
        } else {
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_CAST_MAGIC, 0);
        }
        run_dungeon_post_command(state, result, AK_GAME_COMMAND_CAST_MAGIC);
        return AK_GAME_RESULT_OK;
    }

    switch (ak_random_int(&state->random, 3.0, 1.0)) {
        case 1:
            state->stats[AK_GAME_STAT_STRENGTH] = 3;
            state->stats[AK_GAME_STAT_DEXTERITY] = 3;
            state->stats[AK_GAME_STAT_STAMINA] = 3;
            state->stats[AK_GAME_STAT_WISDOM] = 3;
            break;
        case 2:
            state->stats[AK_GAME_STAT_HIT_POINTS] = (int)((double)state->stats[AK_GAME_STAT_HIT_POINTS] * 2.5);
            state->stats[AK_GAME_STAT_STRENGTH] = (int)((double)state->stats[AK_GAME_STAT_STRENGTH] * 2.5);
            state->stats[AK_GAME_STAT_DEXTERITY] = (int)((double)state->stats[AK_GAME_STAT_DEXTERITY] * 2.5);
            state->stats[AK_GAME_STAT_STAMINA] = (int)((double)state->stats[AK_GAME_STAT_STAMINA] * 2.5);
            state->stats[AK_GAME_STAT_WISDOM] = (int)((double)state->stats[AK_GAME_STAT_WISDOM] * 2.5);
            break;
        default:
            state->stats[AK_GAME_STAT_HIT_POINTS] /= 2;
            if (state->stats[AK_GAME_STAT_HIT_POINTS] <= 0) {
                state->mode = AK_GAME_MODE_DEATH;
                state->location = AK_GAME_LOCATION_DEATH;
            }
            break;
    }
    push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, AK_GAME_COMMAND_CAST_MAGIC, choice);
    if (state->mode == AK_GAME_MODE_DUNGEON) {
        run_dungeon_post_command(state, result, AK_GAME_COMMAND_CAST_MAGIC);
    }
    return AK_GAME_RESULT_OK;
}

static void apply_quest_acknowledgement(AkGameState *state, AkGameCommandResult *result, AkGameCommandType command) {
    int i;
    int award_stats = 0;

    if (state->quest_target == 0) {
        state->quest_target = state->stats[AK_GAME_STAT_WISDOM] / 3;
        if (state->quest_target < 1) {
            state->quest_target = 1;
        }
        if (state->quest_target > AK_GAME_MONSTER_COUNT) {
            state->quest_target = AK_GAME_MONSTER_COUNT;
        }
        award_stats = 1;
    } else if (state->quest_target < 0) {
        if (-state->quest_target >= AK_GAME_MONSTER_COUNT) {
            set_mode(state, result, AK_GAME_MODE_VICTORY, command);
            state->quest_target = 0;
        } else {
            state->quest_target = -state->quest_target + 1;
        }
        award_stats = 1;
    }

    if (award_stats) {
        for (i = 0; i < AK_GAME_STAT_COUNT; i++) {
            state->stats[i]++;
        }
    }
    push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, command, state->quest_target);
}

void ak_game_init(AkGameState *state) {
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->mode = AK_GAME_MODE_STARTUP_LUCKY_NUMBER;
    state->location = AK_GAME_LOCATION_STARTUP;
    state->facing = AK_GAME_DIRECTION_EAST;
    ak_random_init(&state->random);
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
    sync_food_from_inventory(state);

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
        ak_random_rnd(&state->random, (double)-abs(state->lucky_number));
        generate_character_qualities(state);
        state->command_count++;
        set_mode(state, result, AK_GAME_MODE_CHARACTER_REVIEW, type);
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, command->value);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_SELECT_CLASS) {
        if (state->mode != AK_GAME_MODE_CLASS_SELECT ||
            (command->player_class != AK_GAME_CLASS_FIGHTER && command->player_class != AK_GAME_CLASS_MAGE)) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, command->player_class);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->player_class = command->player_class;
        state->command_count++;
        set_mode(state, result, AK_GAME_MODE_TOWN, type);
        set_location(state, result, AK_GAME_LOCATION_TOWN, type);
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, command->player_class);
        return finish(result, AK_GAME_RESULT_OK);
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
        generate_character_qualities(state);
        state->command_count++;
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, 0);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (is_overworld_move(type)) {
        int dx = 0;
        int dy = 0;

        if (state->mode != AK_GAME_MODE_OVERWORLD) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        if (type == AK_GAME_COMMAND_MOVE_NORTH) {
            dy = -1;
        } else if (type == AK_GAME_COMMAND_MOVE_EAST) {
            dx = 1;
        } else if (type == AK_GAME_COMMAND_MOVE_SOUTH) {
            dy = 1;
        } else {
            dx = -1;
        }
        move_overworld(state, result, type, dx, dy);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_FORWARD ||
        type == AK_GAME_COMMAND_TURN_LEFT ||
        type == AK_GAME_COMMAND_TURN_RIGHT ||
        type == AK_GAME_COMMAND_TURN_AROUND) {
        int next_x;
        int next_y;
        int tile;

        if (state->mode != AK_GAME_MODE_DUNGEON) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }

        if (type == AK_GAME_COMMAND_TURN_LEFT) {
            state->facing = (AkGameDirection)((state->facing + 3) % 4);
            state->command_count++;
            push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, state->facing);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, state->facing);
            run_dungeon_post_command(state, result, type);
            return finish(result, AK_GAME_RESULT_OK);
        }
        if (type == AK_GAME_COMMAND_TURN_RIGHT) {
            state->facing = (AkGameDirection)((state->facing + 1) % 4);
            state->command_count++;
            push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, state->facing);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, state->facing);
            run_dungeon_post_command(state, result, type);
            return finish(result, AK_GAME_RESULT_OK);
        }
        if (type == AK_GAME_COMMAND_TURN_AROUND) {
            state->facing = (AkGameDirection)((state->facing + 2) % 4);
            state->command_count++;
            push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, state->facing);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, state->facing);
            run_dungeon_post_command(state, result, type);
            return finish(result, AK_GAME_RESULT_OK);
        }

        next_x = state->dungeon_x + direction_dx(state->facing);
        next_y = state->dungeon_y + direction_dy(state->facing);
        tile = dungeon_tile_at(state, next_x, next_y);

        if (tile != AK_GAME_DUNGEON_WALL && active_monster_at(state, next_x, next_y) == 0) {
            state->dungeon_x = next_x;
            state->dungeon_y = next_y;
        }

        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
        resolve_dungeon_arrival(state, result, type);
        if (state->mode == AK_GAME_MODE_DUNGEON) {
            run_dungeon_post_command(state, result, type);
        }
        if (result != NULL && result->event_count == 1) {
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, tile);
        }
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_ENTER) {
        int tile;

        if (state->mode != AK_GAME_MODE_OVERWORLD) {
            int dungeon_tile;

            if (state->mode != AK_GAME_MODE_DUNGEON) {
                push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
                return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
            }

            dungeon_tile = dungeon_tile_at(state, state->dungeon_x, state->dungeon_y);
            if (dungeon_tile != AK_GAME_DUNGEON_LADDER_DOWN &&
                dungeon_tile != AK_GAME_DUNGEON_LADDER_DOWN_ALT &&
                dungeon_tile != AK_GAME_DUNGEON_LADDER_UP) {
                push_event(result, AK_GAME_EVENT_ERROR, state, type, dungeon_tile);
                return finish(result, AK_GAME_RESULT_REJECTED);
            }

            state->command_count++;
            if (dungeon_tile == AK_GAME_DUNGEON_LADDER_UP && state->dungeon_level == 1) {
                state->stats[AK_GAME_STAT_HIT_POINTS] += state->dungeon_hit_point_reward;
                state->dungeon_hit_point_reward = 0;
                set_mode(state, result, AK_GAME_MODE_OVERWORLD, type);
                set_location(state, result, AK_GAME_LOCATION_OVERWORLD, type);
                push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, dungeon_tile);
                return finish(result, AK_GAME_RESULT_OK);
            }

            if (dungeon_tile == AK_GAME_DUNGEON_LADDER_UP) {
                state->dungeon_level--;
            } else {
                state->dungeon_level++;
            }
            generate_dungeon(state);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, dungeon_tile);
            return finish(result, AK_GAME_RESULT_OK);
        }

        tile = overworld_tile_at(state, state->overworld_x, state->overworld_y);
        if (tile != AK_GAME_TILE_TOWN &&
            tile != AK_GAME_TILE_DUNGEON &&
            tile != AK_GAME_TILE_CASTLE) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, tile);
            return finish(result, AK_GAME_RESULT_REJECTED);
        }

        state->command_count++;
        consume_overworld_food(state);

        if (state->mode == AK_GAME_MODE_DEATH) {
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, 0);
            return finish(result, AK_GAME_RESULT_OK);
        }

        if (tile == AK_GAME_TILE_TOWN) {
            set_mode(state, result, AK_GAME_MODE_TOWN, type);
            set_location(state, result, AK_GAME_LOCATION_TOWN, type);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, tile);
            return finish(result, AK_GAME_RESULT_OK);
        }
        if (tile == AK_GAME_TILE_DUNGEON) {
            enter_dungeon(state, result, type);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, tile);
            return finish(result, AK_GAME_RESULT_OK);
        }
        if (tile == AK_GAME_TILE_CASTLE) {
            set_mode(state, result, AK_GAME_MODE_QUEST, type);
            set_location(state, result, AK_GAME_LOCATION_CASTLE, type);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, tile);
            return finish(result, AK_GAME_RESULT_OK);
        }
    }

    if (type == AK_GAME_COMMAND_EXIT) {
        if (state->mode != AK_GAME_MODE_TOWN) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        state->command_count++;
        return_to_overworld(state, result, type);
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, 0);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_PASS) {
        if (state->mode == AK_GAME_MODE_OVERWORLD) {
            state->command_count++;
            consume_overworld_food(state);
            push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
            push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, 0);
            return finish(result, AK_GAME_RESULT_OK);
        }
        if (state->mode == AK_GAME_MODE_DUNGEON) {
            state->command_count++;
            push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
            run_dungeon_post_command(state, result, type);
            if (result != NULL && result->event_count == 1) {
                push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, 0);
            }
            return finish(result, AK_GAME_RESULT_OK);
        }
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
        if (!is_valid_item(command->item)) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, command->item);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        if (!class_can_buy_item(state->player_class, command->item) ||
            state->stats[AK_GAME_STAT_GOLD] < AK_GAME_ITEM_PRICES[command->item]) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, command->item);
            return finish(result, AK_GAME_RESULT_REJECTED);
        }
        apply_purchase(state, command->item);
        state->command_count++;
        push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, command->item);
        push_event(result, AK_GAME_EVENT_STATE_CHANGED, state, type, command->item);
        return finish(result, AK_GAME_RESULT_OK);
    }

    if (type == AK_GAME_COMMAND_ATTACK || type == AK_GAME_COMMAND_CAST_MAGIC) {
        if (state->mode != AK_GAME_MODE_COMBAT && state->mode != AK_GAME_MODE_DUNGEON) {
            push_event(result, AK_GAME_EVENT_ERROR, state, type, state->mode);
            return finish(result, AK_GAME_RESULT_INVALID_COMMAND);
        }
        if (type == AK_GAME_COMMAND_ATTACK) {
            return finish(result, apply_attack(state, command, result));
        }
        return finish(result, apply_magic(state, command, result));
    }

    if (type == AK_GAME_COMMAND_ACKNOWLEDGE) {
        if (state->mode == AK_GAME_MODE_QUEST) {
            state->command_count++;
            push_event(result, AK_GAME_EVENT_COMMAND_ACCEPTED, state, type, 0);
            apply_quest_acknowledgement(state, result, type);
            return finish(result, AK_GAME_RESULT_OK);
        }
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
        case AK_GAME_MODE_VICTORY: return "victory";
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

const char *ak_game_monster_name(int monster) {
    if (monster >= 1 && monster <= AK_GAME_MONSTER_COUNT) {
        return AK_GAME_MONSTER_NAMES[monster];
    }
    return "unknown";
}

const char *ak_game_tile_name(AkGameTile tile) {
    switch (tile) {
        case AK_GAME_TILE_OPEN: return "open";
        case AK_GAME_TILE_MOUNTAIN: return "mountain";
        case AK_GAME_TILE_FIELD: return "field";
        case AK_GAME_TILE_TOWN: return "town";
        case AK_GAME_TILE_DUNGEON: return "dungeon";
        case AK_GAME_TILE_CASTLE: return "castle";
    }
    return "unknown";
}

const char *ak_game_dungeon_tile_name(AkGameDungeonTile tile) {
    switch (tile) {
        case AK_GAME_DUNGEON_OPEN: return "open";
        case AK_GAME_DUNGEON_WALL: return "wall";
        case AK_GAME_DUNGEON_TRAP: return "trap";
        case AK_GAME_DUNGEON_SECRET_DOOR: return "secret_door";
        case AK_GAME_DUNGEON_HIDDEN_DOOR: return "hidden_door";
        case AK_GAME_DUNGEON_CHEST: return "chest";
        case AK_GAME_DUNGEON_LADDER_DOWN: return "ladder_down";
        case AK_GAME_DUNGEON_LADDER_UP: return "ladder_up";
        case AK_GAME_DUNGEON_LADDER_DOWN_ALT: return "ladder_down_alt";
    }
    return "unknown";
}
