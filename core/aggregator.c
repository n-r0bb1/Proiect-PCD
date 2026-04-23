/* aggregator.c – aduna rezultatele partiale de la workers */
#include "aggregator.h"

long aggregator_sum(const long *partials, size_t count)
{
    if (!partials || count == 0) {
        return 0L;
    }
    long total = 0L;
    for (size_t i = 0; i < count; ++i) {
        total += partials[i];
    }
    return total;
}
