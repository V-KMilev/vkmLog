// Compiled, never linked, by format.cmake: with VKM_TEST_MISFIT_LOG or VKM_TEST_MISFIT_ASSERT, one call
// whose arguments do not fit its format.

#include "vkm_log/assert.h"
#include "vkm_log/logger.h"

void logs(int count, const char* name) {
#if defined(VKM_TEST_MISFIT_LOG)
    LOG_INFO("%d of %s", name, count);
#elif defined(VKM_TEST_MISFIT_ASSERT)
    VKM_ASSERT(count > 0, "%s", count);
#else
    LOG_INFO("%d of %s", count, name);
    VKM_ASSERT(count > 0, "%s", name);
    VKM_ASSERT(count > 0);
#endif
}
