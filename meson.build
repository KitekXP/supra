project(
  'tsux',
  'c',
  version: '1.0.0',
  default_options: ['c_std=c99', 'warning_level=2', 'werror=false']
)


cc = meson.get_compiler('c')


lib_dir = meson.get_external_property('lib_path', '/usr/local/lib')

pam_dep = cc.find_library(
  'pam',
  dirs: [lib_dir],
  required: true
)

tsux_exec = executable(
	'tsux',
	'src/tsux.c',
	dependencies: pam_dep,
	install: false
)

run_target(
  'package',
  command: [
  	files('packager.sh'),
  	meson.project_name(),
  	tsux_exec.full_path(),
  	meson.get_external_property('strip')
  ],
  depends: tsux_exec
)
