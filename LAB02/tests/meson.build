test_scanner = executable(
    'test-scanner',
    'test-scanner.c',
    dependencies: libpebble_dep,
)

test('scanner', test_scanner, protocol: 'tap')

test_parser = executable(
    'test-parser',
    'test-parser.c',
    dependencies: libpebble_dep,
)

test('parser', test_parser, protocol: 'tap')
