#include "ak_random.h"

#include <stddef.h>

#define AK_RANDOM_DEFAULT_SEED 1u
#define AK_RANDOM_MASK 0x7fffffffu
#define AK_RANDOM_SCALE 2147483648.0

static uint32_t normalize_seed(uint32_t seed) {
    seed &= AK_RANDOM_MASK;
    if (seed == 0u) {
        seed = AK_RANDOM_DEFAULT_SEED;
    }
    return seed;
}

uint32_t ak_random_seed_from_applesoft(double argument) {
    double value = argument < 0.0 ? -argument : argument;
    uint32_t whole = (uint32_t)value;
    uint32_t fraction = (uint32_t)((value - (double)whole) * 10000.0 + 0.5);
    uint32_t seed = whole ^ (fraction << 8) ^ 0x4a6b5c3du;

    seed ^= seed >> 16;
    seed *= 0x7feb352du;
    seed ^= seed >> 15;
    seed *= 0x846ca68bu;
    seed ^= seed >> 16;

    return normalize_seed(seed);
}

void ak_random_init(AkRandom *random) {
    if (random == NULL) {
        return;
    }

    random->state = AK_RANDOM_DEFAULT_SEED;
    random->last_value = 0.0;
}

double ak_random_rnd(AkRandom *random, double argument) {
    uint32_t next;

    if (random == NULL) {
        return 0.0;
    }

    if (argument < 0.0) {
        random->state = ak_random_seed_from_applesoft(argument);
    } else if (argument == 0.0) {
        return random->last_value;
    }

    next = (uint32_t)(((uint64_t)random->state * 1103515245u + 12345u) & AK_RANDOM_MASK);
    random->state = normalize_seed(next);
    random->last_value = (double)random->state / AK_RANDOM_SCALE;
    return random->last_value;
}

int ak_random_int(AkRandom *random, double multiplier, double offset) {
    double value = ak_random_rnd(random, 1.0) * multiplier + offset;

    if (value < 0.0) {
        return (int)value - ((double)((int)value) != value);
    }

    return (int)value;
}

int ak_random_chance_greater_than(AkRandom *random, double threshold) {
    return ak_random_rnd(random, 1.0) > threshold;
}
