#include <gtest/gtest.h>
extern "C" {
#include "memory/allocator.h"
#include "string/fmt.h"
}

TEST(FmtBuffer, AppendFormatted) {
  cu_Allocator alloc = cu_Allocator_CAllocator();
  cu_FmtBuffer buf = cu_FmtBuffer_init(alloc);
  EXPECT_EQ(CU_STRING_ERROR_NONE, cu_FmtBuffer_appendf(&buf, "number %d", 10));
  EXPECT_EQ(CU_STRING_ERROR_NONE, cu_FmtBuffer_appendf(&buf, " %s", "items"));
  EXPECT_STREQ(buf.string.data, "number 10 items");
  cu_FmtBuffer_destroy(&buf);
}
