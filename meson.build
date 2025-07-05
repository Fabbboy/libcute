project('libcute', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c11', 'default_library=static'])


includes = include_directories('include')
 
sources = [
  'lib/slice.c',
  'lib/optional.c',
  'lib/memory/allocator.c',
  'lib/memory/page.c',
]

cute_lib = library(
  'cute',
  sources,
  include_directories : includes,
)

cute_dep = declare_dependency(
  include_directories : includes,
  link_with : cute_lib,
)

example_exe = executable(
  'cute_test',
   sources: ['example.c'],
   dependencies: cute_dep,
)
