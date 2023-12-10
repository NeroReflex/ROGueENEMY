#include "rogue_enemy.h"

int32_t div_round_closest(int32_t x, int32_t divisor) {
    const int32_t __x = x;
    const int32_t __d = divisor;
    return ((__x) > 0) == ((__d) > 0) ? (((__x) + ((__d) / 2)) / (__d)) : (((__x) - ((__d) / 2)) / (__d));
}