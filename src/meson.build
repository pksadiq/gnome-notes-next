src = []

resources = gnome.compile_resources(
  'resources',
  'resources/notes.gresource.xml',
  source_dir: 'resources',
  c_name: '_notes'
)

trace_conf = configuration_data()
trace_conf.set10('ENABLE_TRACING', get_option('tracing'))

configure_file(
  input: 'gn-trace.h.in',
  output: 'gn-trace.h',
  configuration: trace_conf,
)

src += [
  'main.c',
  'gn-application.c',
  'gn-settings.c',
  'gn-manager.c',
  'gn-utils.c',
  'gn-window.c',
  'gn-action-bar.c',
  'gn-settings-dialog.c',
  'notes/gn-item.c',
  'notes/gn-note.c',
  'notes/gn-note-buffer.c',
  'notes/gn-plain-note.c',
  'notes/gn-xml-note.c',
  'providers/gn-provider.c',
  'providers/gn-goa-provider.c',
  'providers/gn-memo-provider.c',
  'providers/gn-local-provider.c',
  'views/gn-editor.c',
  'views/gn-grid-view.c',
  'views/gn-list-view.c',
  'views/gn-main-view.c',
  'views/gn-grid-view-item.c',
  'views/gn-list-view-item.c',
  'views/gn-item-thumbnail.c',
  resources
]


inc_dir = [
  top_inc,
  include_directories(
    'notes',
    'providers',
    'views'
  )]

libsrc = [
  'notes/gn-item.c',
  'notes/gn-note.c',
  'notes/gn-plain-note.c',
  'notes/gn-note-buffer.c'
]

libgnotes = static_library(
  'gnotes', libsrc,
  install: false,
  include_directories: inc_dir,
  dependencies: pkg_dep
)

executable(
  'gnome-notes',
  src,
  install: true,
  include_directories: inc_dir,
  dependencies: pkg_dep
)
