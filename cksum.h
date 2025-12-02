#pragma once
#include <inttypes.h>
#include <stddef.h>

extern uint16_t cksum_update(const uint8_t *s, size_t n, uint16_t seed);
extern uint16_t cksum(const uint8_t *s, size_t n);
