# Build tests respecting the freestanding option
unity_dep = dependency('unity')

# Determine whether to compile in freestanding mode
freestanding = get_option('freestanding')
test_args = []
if freestanding.enabled()
  test_args += ['-DCU_FREESTANDING']
endif

# Individual test executables
test_files = [
  'test_optional.c',
  'test_string.c',
  'test_allocator.c',
  'test_bitmap.c',
  'test_gpa.c',
  'test_hash.c',
  'test_hashmap.c',
  'test_page_allocator.c',
  'test_slab_allocator.c',
  'test_ring_buffer.c',
  'test_list.c',
  'test_dlist.c',
  'test_vector.c',
  'test_arena_allocator.c',
  'test_fmt.c',
  'test_fixed_allocator.c',
  'test_wasm_allocator.c',
  'test_skip_list.c',
  'test_skip_list_sst.c',
  'test_rc.c',
  'test_file.c',
  'test_dir.c',
]

foreach test_file : test_files
  test_name = test_file.split('.')[0]
  exe = executable(
    test_name,
    [test_file, 'test_common.c'],
    dependencies: [cute_dep, unity_dep],
    include_directories: includes,
    c_args: test_args,
  )
  test(test_name, exe)
endforeach

