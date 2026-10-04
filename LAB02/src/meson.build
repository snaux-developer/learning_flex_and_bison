libpebble = static_library(
    'pebble',
    'parser.c',
    'scanner.c',
    dependencies: glib_dep,
)

libpebble_dep = declare_dependency(
    link_with: libpebble,
    include_directories: include_directories('.'),
    dependencies: glib_dep,
)

executable(
    'pebble',
    'main.c',
    dependencies: libpebble_dep,
)
