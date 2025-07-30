project(
  'libcute',
  ['c'],
  version: '0.1',
  default_options: ['warning_level=3', 'c_std=c11', 'default_library=static'],
  meson_version: '>=1.4.0',
)

includes = include_directories('include')

sources = [
  'lib/object/optional.c',
  'lib/object/destructor.c',
  'lib/memory/allocator.c',
  'lib/memory/page.c',
  'lib/memory/arenaallocator.c',
  'lib/memory/gpallocator.c',
  'lib/memory/slab.c',
  'lib/memory/fixedallocator.c',
  'lib/memory/wasmallocator.c',
  'lib/collection/bitmap.c',
  'lib/hash/hash.c',
  'lib/string/string.c',
  'lib/string/fmt.c',
  'lib/nostd.c',
  'lib/utility.c',
  'lib/collection/ring_buffer.c',
  'lib/collection/list.c',
  'lib/collection/dlist.c',
  'lib/collection/skip_list.c',
  'lib/collection/vector.c',
  'lib/collection/hashmap.c',
  'lib/state.c',
  'lib/io/error.c',
  'lib/io/file.c',
  'lib/io/dir.c',
  'lib/io/fstream.c',
  'lib/io/fdfile.c',
  'lib/io/memstream.c',
]

freestanding = get_option('freestanding')
examples = get_option('examples')

c_args = []
if freestanding.enabled()
  c_args += ['-DCU_FREESTANDING']
endif

cute_lib = library(
  'cute',
  sources,
  include_directories: includes,
  c_args: c_args,
)

libcute_dep = declare_dependency(
  include_directories: includes,
  link_with: cute_lib,
)

meson.override_dependency('cute', libcute_dep)

if get_option('tests').enabled()
  subdir('tests')
endif

if not freestanding.enabled() and examples.enabled()
  subdir('examples')
endif

doxygen = find_program('doxygen', required: false)
if doxygen.found()
  conf = configuration_data()
  conf.set('PROJECT_VERSION', meson.project_version())
  config_h = configure_file(output: 'config.h', configuration: conf)

  doxy_conf = configuration_data()
  doxy_conf.set('PROJECT_VERSION', meson.project_version())
  doxy_conf.set('CONFIG_H', config_h.full_path())
  doxyfile = configure_file(input: 'docs/Doxyfile.in', output: 'Doxyfile', configuration: doxy_conf)

  run_target('docs', command: [doxygen, doxyfile])
endif