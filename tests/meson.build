# Build tests respecting the freestanding option
gtest_dep = dependency('gtest')
gtest_main = dependency('gtest_main')

# Determine whether to compile in freestanding mode
freestanding = get_option('freestanding')
test_args = []
if freestanding.enabled()
  test_args += ['-DCU_FREESTANDING']
endif

# Individual test executables
test_files = [
  'test_optional.cpp',
  'test_string.cpp',
  'test_allocator.cpp',
  'test_bitmap.cpp',
  'test_gpa.cpp',
  'test_hash.cpp',
  'test_hashmap.cpp',
  'test_page_allocator.cpp',
  'test_slab_allocator.cpp',
  'test_ring_buffer.cpp',
  'test_list.cpp',
  'test_dlist.cpp',
  'test_vector.cpp',
  'test_arena_allocator.cpp',
  'test_fmt.cpp',
  'test_fixed_allocator.cpp',
  'test_wasm_allocator.cpp',
  'test_skip_list.cpp',
  'test_skip_list_sst.cpp',
]

foreach test_file : test_files
  test_name = test_file.split('.')[0]
  exe = executable(
    test_name,
    test_file,
    dependencies: [cute_dep, gtest_dep, gtest_main],
    include_directories: includes,
    c_args: test_args,
    cpp_args: test_args,
  )
  test(test_name, exe)
endforeach