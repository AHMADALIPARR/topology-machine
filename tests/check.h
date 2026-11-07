/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#ifndef TM_CHECK_H
#define TM_CHECK_H

#include <stdio.h>

static int g_checks;
static int g_fails;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_checks++;                                                          \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            g_fails++;                                                       \
        }                                                                    \
    } while (0)

static int check_report(const char *name)
{
    if (g_fails) {
        fprintf(stderr, "%s: %d/%d failed\n", name, g_fails, g_checks);
        return 1;
    }
    printf("%s: %d checks passed\n", name, g_checks);
    return 0;
}

#endif
