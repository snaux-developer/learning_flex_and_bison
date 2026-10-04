project(
    'pebble',
    'c',
    version: '1.2.0',
    license: 'MIT',
    default_options: ['c_std=gnu17', 'warning_level=2'],
)

glib_dep = dependency('glib-2.0', version: '>= 2.90')

add_project_arguments(
    '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_90',
    '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_90',
    language: 'c',
)

subdir('src')
subdir('tests')
