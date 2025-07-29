#include "hash/hash.h"
#include "unity.h"
#include <unity_internals.h>

static void Hash_FNV1a32(void) {
  const char *data = "abcd";
  uint32_t h = cu_Hash_FNV1a32(data, 4);
  TEST_ASSERT_TRUE((0u) != (h));
}

static void Hash_Murmur3(void) {
  const char *data = "abcd";
  uint32_t h = cu_Hash_Murmur3_32(data, 4, 1234);
  TEST_ASSERT_TRUE((0u) != (h));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(Hash_FNV1a32);
  RUN_TEST(Hash_Murmur3);
  return UNITY_END();
}
