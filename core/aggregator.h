#ifndef AGGREGATOR_H
#define AGGREGATOR_H

#include <stddef.h>

long aggregator_sum(const long *partials, size_t count);

#endif /* AGGREGATOR_H */
