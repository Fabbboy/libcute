gtest_dep = dependency('gtest')
gtest_main = dependency('gtest_main')

sources = [
  'test_optional.cpp',
  'test_string.cpp',
  'test_allocator.cpp',
  'test_hash.cpp',
  'test_page_allocator.cpp',
]

exe = executable('libcute_tests', sources,
  dependencies : [cute_dep, gtest_dep, gtest_main],
  include_directories : includes,
)

test('libcute', exe)
