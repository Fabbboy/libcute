project(
  'libcute',
  ['c', 'cpp'],
  version: '0.1',
  default_options: ['warning_level=3', 'c_std=c11', 'default_library=static'],
)

includes = include_directories('include')

sources = [
  'lib/object/slice.c',
  'lib/object/optional.c',
  'lib/memory/allocator.c',
  'lib/memory/page.c',
  'lib/memory/arenaallocator.c',
  'lib/memory/gpallocator.c',
  'lib/memory/slab.c',
  'lib/collection/bitmap.c',
  'lib/hash/hash.c',
  'lib/string/string.c',
  'lib/string/fmt.c',
  'lib/collection/ring_buffer.c',
  'lib/collection/list.c',
  'lib/collection/dlist.c',
  'lib/collection/vector.c',
  'lib/collection/hashmap.c',
]

deps = []
if get_option('worker_pool').enabled()
  uv_dep = dependency('libuv', fallback: ['libuv', 'libuv_dep'])
  sources += ['lib/concurrency/worker_pool.c']
  deps += uv_dep
  add_project_arguments('-DCU_USE_LIBUV', '-D_GNU_SOURCE', language: ['c', 'cpp'])
endif

cute_lib = library(
  'cute',
  sources,
  include_directories: includes,
  dependencies: deps,
)

cute_dep = declare_dependency(
  include_directories: includes,
  link_with: cute_lib,
  dependencies: deps,
)

if get_option('tests').enabled()
  subdir('tests')
endif

subdir('examples')

doxygen = find_program('doxygen', required: false)
if doxygen.found()
  conf = configuration_data()
  conf.set('PROJECT_VERSION', meson.project_version())
  config_h = configure_file(output: 'config.h', configuration: conf)

  doxy_conf = configuration_data()
  doxy_conf.set('PROJECT_VERSION', meson.project_version())
  doxy_conf.set('CONFIG_H', 'config.h')
  doxyfile = configure_file(input: 'docs/Doxyfile.in', output: 'Doxyfile', configuration: doxy_conf)

  run_target('docs', command: [doxygen, doxyfile])
endif
