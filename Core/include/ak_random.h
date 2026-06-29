#ifndef AK_RANDOM_H
#define AK_RANDOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state;
    double last_value;
} AkRandom;

void ak_random_init(AkRandom *random);
double ak_random_rnd(AkRandom *random, double argument);
int ak_random_int(AkRandom *random, double multiplier, double offset);
int ak_random_chance_greater_than(AkRandom *random, double threshold);
uint32_t ak_random_seed_from_applesoft(double argument);

#ifdef __cplusplus
}
#endif

#endif
