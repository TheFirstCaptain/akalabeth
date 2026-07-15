#include "ak_random.h"

#include <assert.h>

static int close_enough(double actual, double expected) {
    double delta = actual - expected;
    if (delta < 0.0) {
        delta = -delta;
    }
    return delta < 0.000000001;
}

static void test_default_sequence_is_deterministic(void) {
    AkRandom first;
    AkRandom second;
    double first_values[4];
    double second_values[4];
    int i;

    ak_random_init(&first);
    ak_random_init(&second);

    for (i = 0; i < 4; i++) {
        first_values[i] = ak_random_rnd(&first, 1.0);
        second_values[i] = ak_random_rnd(&second, 1.0);
    }

    assert(close_enough(first_values[0], 0.513870078139));
    assert(close_enough(first_values[1], 0.175741303247));
    assert(close_enough(first_values[2], 0.308651516214));
    assert(close_enough(first_values[3], 0.534533886705));

    for (i = 0; i < 4; i++) {
        assert(close_enough(first_values[i], second_values[i]));
    }
}

static void test_negative_argument_reseeds_sequence(void) {
    AkRandom first;
    AkRandom second;
    double first_value;
    double second_value;

    ak_random_init(&first);
    ak_random_init(&second);

    first_value = ak_random_rnd(&first, -1234.0);
    second_value = ak_random_rnd(&second, -1234.0);

    assert(close_enough(first_value, second_value));
    assert(close_enough(first_value, 0.255045922007));
    assert(ak_random_rnd(&first, 1.0) != first_value);

    ak_random_rnd(&first, 1.0);
    first_value = ak_random_rnd(&first, -1234.0);
    assert(close_enough(first_value, second_value));
}

static void test_zero_argument_repeats_last_value(void) {
    AkRandom random;
    double value;

    ak_random_init(&random);
    value = ak_random_rnd(&random, 1.0);

    assert(close_enough(ak_random_rnd(&random, 0.0), value));
    assert(close_enough(ak_random_rnd(&random, 0.0), value));
}

static void test_integer_range_helpers_cover_source_shapes(void) {
    AkRandom random;
    int dungeon_x;
    int dungeon_y;
    int amulet_choice;

    ak_random_init(&random);
    ak_random_rnd(&random, -7.0);

    dungeon_x = ak_random_int(&random, 19.0, 1.0);
    dungeon_y = ak_random_int(&random, 19.0, 1.0);
    amulet_choice = ak_random_int(&random, 4.0, 1.0);

    assert(dungeon_x >= 1 && dungeon_x <= 19);
    assert(dungeon_y >= 1 && dungeon_y <= 19);
    assert(amulet_choice >= 1 && amulet_choice <= 4);
}

static void test_chance_helper_matches_rnd_comparison_shape(void) {
    AkRandom random;

    ak_random_init(&random);
    ak_random_rnd(&random, -1.0);

    assert(ak_random_chance_greater_than(&random, 0.95) == 0);
    assert(ak_random_chance_greater_than(&random, 0.04) == 1);
}

static void test_fractional_negative_seed_supports_dungeon_seed_shape(void) {
    AkRandom first;
    AkRandom second;
    double seed_argument = -1234.0 - 9.0 * 10.0 - 12.0 * 1000.0 + 3.0 * 31.4;
    double first_value;

    ak_random_init(&first);
    ak_random_init(&second);

    first_value = ak_random_rnd(&first, seed_argument);
    assert(close_enough(first_value, ak_random_rnd(&second, seed_argument)));
    assert(close_enough(first_value, 0.533016623463));
    assert(first.state == 1144644483u);
    assert(first.state == second.state);
}

int main(void) {
    test_default_sequence_is_deterministic();
    test_negative_argument_reseeds_sequence();
    test_zero_argument_repeats_last_value();
    test_integer_range_helpers_cover_source_shapes();
    test_chance_helper_matches_rnd_comparison_shape();
    test_fractional_negative_seed_supports_dungeon_seed_shape();
    return 0;
}
