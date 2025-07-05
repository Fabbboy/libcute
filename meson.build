project('libcute', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c11', 'default_library=static'])

utf8proc_dep = dependency('libutf8proc', required: true)

includes = include_directories('include')
 
sources = [
  'lib/object/slice.c',
  'lib/object/optional.c',
  'lib/memory/allocator.c',
  'lib/memory/page.c',
  'lib/hash/hash.c',
]

cute_lib = library(
  'cute',
  sources,
  include_directories : includes,
  dependencies: [utf8proc_dep]
)

cute_dep = declare_dependency(
  include_directories : includes,
  link_with : cute_lib,
)

example_exe = executable(
  'cute_test',
   sources: ['example.c'],
   dependencies: [cute_dep, utf8proc_dep],
)
