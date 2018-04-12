desktop = 'org.sadiqpk.notes.desktop'

i18n.merge_file(
  desktop,
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(pkg_datadir, 'applications')
)

install_data(
  'org.sadiqpk.notes.gschema.xml',
  install_dir: pkg_schemadir
)

subdir('icons')
