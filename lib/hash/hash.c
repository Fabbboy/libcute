#include "hash/hash.h"
#include <nostd.h>
#include <stddef.h>
#include <stdint.h>

/** Compute the 32-bit FNV-1a hash of a buffer. */
uint32_t cu_Hash_FNV1a32(const void *data, size_t len) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint32_t hash = CU_FNV1A32_OFFSET_BASIS;
  for (size_t i = 0; i < len; ++i) {
    hash ^= bytes[i];
    hash *= CU_FNV1A32_PRIME;
  }
  return hash;
}

/** Compute the 64-bit FNV-1a hash of a buffer. */
uint64_t cu_Hash_FNV1a64(const void *data, size_t len) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint64_t hash = CU_FNV1A64_OFFSET_BASIS;
  for (size_t i = 0; i < len; ++i) {
    hash ^= bytes[i];
    hash *= CU_FNV1A64_PRIME;
  }
  return hash;
}

/** Compute the 32-bit Murmur3 hash of a buffer. */
uint32_t cu_Hash_Murmur3_32(const void *data, size_t len, uint32_t seed) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint32_t h1 = seed;
  const uint32_t c1 = CU_MURMUR3_C1;
  const uint32_t c2 = CU_MURMUR3_C2;
  size_t i = 0;
  while (len - i >= 4) {
    uint32_t k1;
    cu_Memory_memcpy(&k1, cu_Slice_create((void *)(bytes + i), 4));
    k1 *= c1;
    k1 = (k1 << 15) | (k1 >> 17);
    k1 *= c2;

    h1 ^= k1;
    h1 = (h1 << 13) | (h1 >> 19);
    h1 = h1 * 5 + CU_MURMUR3_N;
    i += 4;
  }

  uint32_t k1 = 0;
  switch (len & 3) {
  case 3:
    k1 ^= bytes[i + 2] << 16;
    /* fallthrough */
  case 2:
    k1 ^= bytes[i + 1] << 8;
    /* fallthrough */
  case 1:
    k1 ^= bytes[i];
    k1 *= c1;
    k1 = (k1 << 15) | (k1 >> 17);
    k1 *= c2;
    h1 ^= k1;
  }

  h1 ^= (uint32_t)len;

  h1 ^= h1 >> 16;
  h1 *= CU_MURMUR3_F1;
  h1 ^= h1 >> 13;
  h1 *= CU_MURMUR3_F2;
  h1 ^= h1 >> 16;

  return h1;
}

static inline uint64_t cu_hash_read64(const uint8_t *p) {
  uint64_t v;
  cu_Memory_memcpy(&v, cu_Slice_create((void *)p, 8));
  return v;
}

#define SIPROUND                                                               \
  do {                                                                         \
    v0 += v1;                                                                  \
    v1 = (v1 << 13) | (v1 >> 51);                                              \
    v1 ^= v0;                                                                  \
    v0 = (v0 << 32) | (v0 >> 32);                                              \
    v2 += v3;                                                                  \
    v3 = (v3 << 16) | (v3 >> 48);                                              \
    v3 ^= v2;                                                                  \
    v0 += v3;                                                                  \
    v3 = (v3 << 21) | (v3 >> 43);                                              \
    v3 ^= v0;                                                                  \
    v2 += v1;                                                                  \
    v1 = (v1 << 17) | (v1 >> 47);                                              \
    v1 ^= v2;                                                                  \
    v2 = (v2 << 32) | (v2 >> 32);                                              \
  } while (0)

/** Compute SipHash-2-4 for the given input. */
uint64_t cu_Hash_SipHash24(
    const uint8_t key[16], const void *data, size_t len) {
  uint64_t k0 = cu_hash_read64(key);
  uint64_t k1 = cu_hash_read64(key + 8);
  uint64_t v0 = CU_SIPHASH_V0_INIT ^ k0;
  uint64_t v1 = CU_SIPHASH_V1_INIT ^ k1;
  uint64_t v2 = CU_SIPHASH_V2_INIT ^ k0;
  uint64_t v3 = CU_SIPHASH_V3_INIT ^ k1;

  const uint8_t *in = (const uint8_t *)data;
  size_t i = 0;
  while (len - i >= 8) {
    uint64_t m = cu_hash_read64(in + i);
    v3 ^= m;
    SIPROUND;
    SIPROUND;
    v0 ^= m;
    i += 8;
  }

  uint64_t b = ((uint64_t)len) << 56;
  switch (len & 7) {
  case 7:
    b |= ((uint64_t)in[i + 6]) << 48;
    /* fallthrough */
  case 6:
    b |= ((uint64_t)in[i + 5]) << 40;
    /* fallthrough */
  case 5:
    b |= ((uint64_t)in[i + 4]) << 32;
    /* fallthrough */
  case 4:
    b |= ((uint64_t)in[i + 3]) << 24;
    /* fallthrough */
  case 3:
    b |= ((uint64_t)in[i + 2]) << 16;
    /* fallthrough */
  case 2:
    b |= ((uint64_t)in[i + 1]) << 8;
    /* fallthrough */
  case 1:
    b |= ((uint64_t)in[i]);
  }

  v3 ^= b;
  SIPROUND;
  SIPROUND;
  v0 ^= b;
  v2 ^= 0xff;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  SIPROUND;
  return v0 ^ v1 ^ v2 ^ v3;
}
