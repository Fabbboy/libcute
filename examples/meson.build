sources_6502 = [
    '6502/6502.c',
    '6502/main.c',
]

executable('6502emu', sources_6502, dependencies : cute_dep,
    include_directories : includes, )

sources_http = [
    'http/http.c',
    'http/main.c',
]

executable('httpserver', sources_http, dependencies : cute_dep,
    include_directories : includes, )
