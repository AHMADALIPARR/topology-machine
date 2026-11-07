/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 *
 * GF(2^8) with the AES polynomial x^8 + x^4 + x^3 + x + 1.
 * Reduction constant 0x1b. This is field arithmetic, not a MAC.
 */
#include "tm.h"

uint8_t tm_gf_mul(uint8_t a, uint8_t b)
{
    uint8_t out = 0;
    int i;
    for (i = 0; i < 8; i++) {
        if (b & 1u)
            out ^= a;
        {
            uint8_t carry = (uint8_t)(a & 0x80u);
            a = (uint8_t)(a << 1);
            if (carry)
                a ^= TM_GF_REDUCE;
        }
        b = (uint8_t)(b >> 1);
    }
    return out;
}

static uint8_t mul_rec(uint8_t a, uint8_t b, uint8_t acc, int i)
{
    if (i == 8)
        return acc;
    return mul_rec((uint8_t)((a << 1) ^ ((a & 0x80u) ? TM_GF_REDUCE : 0)),
                   (uint8_t)(b >> 1),
                   (uint8_t)(acc ^ ((b & 1u) ? a : 0)),
                   i + 1);
}

uint8_t tm_gf_mul_rec(uint8_t a, uint8_t b)
{
    return mul_rec(a, b, 0, 0);
}

uint8_t tm_gf_pow(uint8_t a, unsigned exp)
{
    uint8_t r = 1;
    while (exp) {
        if (exp & 1u)
            r = tm_gf_mul(r, a);
        a = tm_gf_mul(a, a);
        exp >>= 1;
    }
    return r;
}

int tm_gf_inv(uint8_t a, uint8_t *inv)
{
    if (!inv)
        return TM_ERR_RANGE;
    if (a == 0)
        return TM_ERR_DOMAIN;
    /* a^(2^8 - 2) = a^254 is the inverse in a field of 256 elements. */
    *inv = tm_gf_pow(a, 254);
    return TM_OK;
}
