sources_6502 = [
    '6502/6502.c',
    '6502/main.c',
]

executable('6502emu', sources_6502, dependencies : cute_dep,
    include_directories : includes, )

libuv_dep = dependency('libuv', default_options: [
  'build_tests=false',
  'build_benchmarks=false',
])

sources_http = ['http_server/http_server.c']

executable(
  'http_server',
  sources_http,
  dependencies: [cute_dep, libuv_dep],
  include_directories: includes,
)
