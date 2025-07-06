#include <gtest/gtest.h>
extern "C" {
#include "hash/hash.h"
}

TEST(Hash, FNV1a32) {
  const char *data = "abcd";
  uint32_t h = cu_Hash_FNV1a32(data, 4);
  EXPECT_NE(0u, h);
}

TEST(Hash, Murmur3) {
  const char *data = "abcd";
  uint32_t h = cu_Hash_Murmur3_32(data, 4, 1234);
  EXPECT_NE(0u, h);
}
