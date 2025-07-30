sources_6502 = [
  '6502/6502.c',
  '6502/main.c',
]

sources_http = [
  'http/http.c',
  'http/main.c',
]

executable(
  '6502emu',
  sources_6502,
  dependencies: libcute_dep,y§
  include_directories: includes,
)

executable(
  'httpserver',
  sources_http,
  dependencies: libcute_dep,
  include_directories: includes,
)
