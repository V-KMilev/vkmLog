// VKM_ASSERT as a build that defines NDEBUG sees it, whatever this suite's build type defines.
#ifndef NDEBUG
    #define NDEBUG
#endif

#include "vkm_log/assert.h"

#include "harness/check.h"

TEST("under NDEBUG a VKM_ASSERT is compiled out: its condition is not evaluated and it never stops") {
    int evaluated = 0;
    VKM_ASSERT(++evaluated < 0, "never %d", evaluated);
    VKM_ASSERT(false);
    CHECK(evaluated == 0);
}
