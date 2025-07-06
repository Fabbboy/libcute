gtest_dep = dependency('gtest')
gtest_main = dependency('gtest_main')

# Individual test executables
test_files = [
  'test_optional.cpp',
  'test_string.cpp',
  'test_allocator.cpp',
  'test_hash.cpp',
  'test_page_allocator.cpp',
]

foreach test_file : test_files
  test_name = test_file.split('.')[0]
  exe = executable(test_name, test_file,
    dependencies : [cute_dep, gtest_dep, gtest_main],
    include_directories : includes,
  )
  test(test_name, exe)
endforeach
