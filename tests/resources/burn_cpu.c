#include "burn_cpu.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

size_t burn_cpu(const size_t count) {
  bool *prime = malloc(count);
  assert(prime);
  prime[0] = prime[1] = false;
  for (size_t i = 2; i < count; ++i) prime[i] = true;
  for (size_t i = 2; i * i < count; ++i)
    if (prime[i])
      for (size_t j = i * i; j < count; j += i) prime[j] = false;
  size_t ret = 0;
  for (size_t i = 0; i < count; ++i)
    if (prime[i]) ++ret;
  free(prime);
  return ret;
}
