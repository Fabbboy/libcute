project(
  'libcute',
  ['c', 'cpp'],
  version: '0.1',
  default_options: ['warning_level=3', 'c_std=c11', 'default_library=static'],
  meson_version: '>=1.4.0',
)

includes = include_directories('include')

sources = [
  'lib/object/optional.c',
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
  'lib/collection/ring_buffer.c',
  'lib/collection/list.c',
  'lib/collection/dlist.c',
  'lib/collection/vector.c',
  'lib/collection/hashmap.c',
  'lib/io/error.c',
]

nostd = get_option('nostd')
c_args = []
if nostd.enabled()
  c_args += ['-DCU_NO_STD']
  message(
    'Tests are deactivated for now but we will adjust the builds and conditionally add new and remove stdlib depended',
  )
endif

cute_lib = library(
  'cute',
  sources,
  include_directories: includes,
  c_args: c_args,
)

cute_dep = declare_dependency(
  include_directories: includes,
  link_with: cute_lib,
)

if get_option('tests').enabled() and not nostd.enabled()
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
  doxy_conf.set('CONFIG_H', config_h.full_path())
  doxyfile = configure_file(input: 'docs/Doxyfile.in', output: 'Doxyfile', configuration: doxy_conf)

  run_target('docs', command: [doxygen, doxyfile])
endif