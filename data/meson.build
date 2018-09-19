i18n.merge_file(
  input: pkg_id + '.desktop.in',
  output: pkg_id + '.desktop',
  type: 'desktop',
  po_dir: pkg_po_dir,
  install: true,
  install_dir: join_paths(pkg_datadir, 'applications')
)

install_data(
  pkg_id + '.gschema.xml',
  install_dir: pkg_schemadir
)

subdir('icons')
