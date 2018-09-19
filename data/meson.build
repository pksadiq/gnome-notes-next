desktop_file = i18n.merge_file(
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

compiled = gnome.compile_schemas(build_by_default: true)

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils,
       args: [desktop_file])
endif

subdir('icons')
