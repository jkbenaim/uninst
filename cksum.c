#include <inttypes.h>
#include "cksum.h"

uint16_t cksum(const uint8_t *s, size_t n)
{
	uint16_t sum = 0;

	for (size_t i = 0; i < n; i++) {
		sum = (sum >> 1) + ((sum & 1) << 15);
		sum += s[i];
		sum &= 0xffff;
	}

	return sum;
}
