tests_inc = [src_inc, top_inc, inc_dir]

env = environment()
env.set('G_TEST_SRCDIR', meson.current_source_dir())
env.set('G_TEST_BUILDDIR', meson.current_build_dir())
env.set('G_DEBUG', 'gc-friendly')
env.set('MALLOC_CHECK_', '2')
env.set('MALLOC_PERTURB_', '4') # Guaranteed to be random

test_plain_note = executable(
  'test-plain-note',
  'test-plain-note.c',
  include_directories: tests_inc,
  link_with: libgnotes,
  dependencies: pkg_dep
)
test('test-plain-note', test_plain_note, env: env)
