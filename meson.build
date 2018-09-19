project(
  'gnome-notes', 'c',
  version: '0.0.1',
  meson_version: '>= 0.44.0',
  license: 'GPL3+',
  default_options: [
    'c_std=gnu11',
    'warning_level=1',
    'buildtype=debugoptimized'
  ])

pkg_name = meson.project_name()
pkg_id        = 'org.sadiqpk.notes'
pkg_version = meson.project_version()
pkg_is_debug  = get_option('buildtype').startswith('debug')

pkg_prefix = get_option('prefix')
pkg_srcdir = meson.source_root()
pkg_builddir = meson.build_root()
pkg_libdir = join_paths(pkg_prefix, get_option('libdir'))
pkg_localedir = join_paths(pkg_prefix, get_option('localedir'))
pkg_datadir = join_paths(pkg_prefix, get_option('datadir'))
pkg_schemadir = join_paths(pkg_datadir, 'glib-2.0', 'schemas')
pkg_po_dir = join_paths(meson.source_root(), 'po')
top_inc       = include_directories('.')
src_inc       = include_directories('src')

cc = meson.get_compiler('c')
conf = configuration_data()

i18n = import('i18n')
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())

# debug options
conf.set('NDEBUG', get_option('buildtype').startswith('debug'))

# package
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_VERSION', pkg_version)
conf.set_quoted('PACKAGE_BUGREPORT', 'https://sadiqpk.org/bugs')
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(meson.project_name(),
                                                   pkg_version))
conf.set_quoted('PACKAGE_URL', '')

# functions
conf.set('HAVE_DCGETTEXT', cc.has_function('dcgettext'))
conf.set('HAVE_GETTEXT', cc.has_function('gettext'))
conf.set('HAVE_ICONV', cc.has_function('iconv'))
conf.set_quoted('PACKAGE_LOCALE_DIR', join_paths([ get_option('prefix'),
                                                   get_option('datadir'),
                                                   'locale' ]))

common_flags = [
  '-DHAVE_CONFIG_H',
  '-D_FORTIFY_SOURCE=2'
]

add_global_arguments(common_flags, language: 'c')
c_link_args = cc.get_supported_arguments(
  ['-Wall',
   '-Werror=format-security',
   '-Werror=implicit-function-declaration',
   '-fasynchronous-unwind-tables',
   '-pipe',
  ])

pkg_dep = [
  dependency('glib-2.0', version: '>= 2.44.0'),
  dependency('gtk+-4.0', version: '>= 3.93.0'),
  dependency('goa-1.0',),
  dependency('libecal-1.2', version: '>= 3.18.0'),
  dependency('libedataserver-1.2', version: '>= 3.18.0'),
]

configure_file(
  output : 'config.h',
  configuration : conf,
)

gnome = import('gnome')
pkg = import('pkgconfig')

subdir('src')
subdir('data')
subdir('docs')
subdir('po')
subdir('tests')

if get_option('bash_completion')
  subdir('completion')
endif

meson.add_install_script('build-aux/meson_post_install.py')

run_target('make-po',
           command: [
             find_program('build-aux/make_po.sh'),
             meson.project_name(),
             meson.current_build_dir(),
             join_paths(pkg_srcdir, 'po')
           ])

run_target('make-help-po',
           command: [
             find_program('build-aux/make_po.sh'),
             pkg_id,
             meson.current_build_dir(),
             join_paths(pkg_srcdir, 'docs/help'),
             'help-',
             'true'
           ])

output = '\n        ' + pkg_name + ' ' + pkg_version + '\n'
output += '        ==============================\n\n'
output += '        prefix:          ' + pkg_prefix + '\n'
output += '        compiler:        ' + cc.get_id() + ' ' + cc.version()+ '\n'
output += '        tracing:         ' + get_option('tracing').to_string() + '\n'
output += '        manpage:         ' + get_option('man').to_string() + '\n'
output += '        bash-completion: ' + get_option('bash_completion').to_string() + '\n'
output += '        Now type \'make\' to build ' + pkg_name + '\n'
message(output)
