/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdio.h>

int main(void)
{
    int a, b, c;
    uint8_t inv = 0;
    CHECK(tm_gf_mul(0x57, 0x83) == 0xc1);
    CHECK(tm_gf_mul_rec(0x57, 0x83) == 0xc1);
    CHECK(tm_gf_mul(0x53, 0x01) == 0x53);
    CHECK(tm_gf_mul(0x53, 0x00) == 0x00);
    CHECK(tm_gf_mul(0xff, 0x02) == 0xe5);
    CHECK(tm_gf_mul(0x80, 0x02) == 0x1b);
    CHECK(tm_gf_inv(0, &inv) == TM_ERR_DOMAIN);
    for (a = 0; a < 256; a++) {
        uint8_t ua = (uint8_t)a;
        CHECK(tm_gf_mul(ua, 0) == 0);
        CHECK(tm_gf_mul(ua, 1) == ua);
        CHECK(tm_gf_mul(ua, ua) == tm_gf_mul_rec(ua, ua));
        if (ua == 0)
            continue;
        CHECK(tm_gf_inv(ua, &inv) == TM_OK);
        CHECK(tm_gf_mul(ua, inv) == 1);
        CHECK(inv == tm_gf_pow(ua, 254));
        {
            int hits = 0;
            for (b = 0; b < 256; b++) {
                if (tm_gf_mul(ua, (uint8_t)b) == 1)
                    hits++;
            }
            CHECK(hits == 1);
        }
    }
    for (a = 0; a < 256; a++) {
        for (b = 0; b < 256; b++) {
            uint8_t ua = (uint8_t)a, ub = (uint8_t)b;
            CHECK(tm_gf_mul(ua, ub) == tm_gf_mul(ub, ua));
            CHECK(tm_gf_mul(ua, ub) == tm_gf_mul_rec(ua, ub));
        }
    }
    for (a = 0; a < 256; a++) {
        for (b = 0; b < 256; b++) {
            for (c = 0; c < 256; c++) {
                uint8_t ua = (uint8_t)a, ub = (uint8_t)b, uc = (uint8_t)c;
                uint8_t left = tm_gf_mul(tm_gf_mul(ua, ub), uc);
                uint8_t right = tm_gf_mul(ua, tm_gf_mul(ub, uc));
                uint8_t sum = (uint8_t)(ua ^ ub);
                CHECK(left == right);
                CHECK(tm_gf_mul(sum, uc) == (uint8_t)(tm_gf_mul(ua, uc) ^ tm_gf_mul(ub, uc)));
            }
        }
    }
    return check_report("gf");
}
