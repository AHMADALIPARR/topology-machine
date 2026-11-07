/* SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) 2026 Topology Machine contributors
 */
#include "check.h"
#include "tm.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    char path[] = "build/selfcheck-out";
    FILE *fp;
    char buf[4096];
    size_t n;
    CHECK(tm_mkdir("build") == TM_OK);
    fp = fopen("build/selfcheck.txt", "w");
    CHECK(fp != NULL);
    CHECK(tm_selfcheck(fp, path) == TM_OK);
    fclose(fp);
    fp = fopen("build/selfcheck.txt", "r");
    CHECK(fp != NULL);
    n = fread(buf, 1, sizeof buf - 1, fp);
    buf[n] = '\0';
    fclose(fp);
    CHECK(strstr(buf, "PASS: all native checks\n") != NULL);
    CHECK(strstr(buf, "0x57*0x83=0xc1") != NULL);
    CHECK(strstr(buf, "V=(0.400,0.200)") != NULL);
    CHECK(strstr(buf, "0 models") != NULL);
    return check_report("selfcheck");
}
