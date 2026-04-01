project('libstrophe', 'c',
  meson_version : '>=0.60.3',
  version : run_command('git', 'describe',  '--tags', check : true).stdout().strip(),
)

mod = import('unstable-external_project')

run_command('bootstrap.sh', check : true)

p = mod.add_project('configure',
  configure_options : ['--prefix=@PREFIX@',
                       '--libdir=@PREFIX@/@LIBDIR@',
                       '--includedir=@PREFIX@/@INCLUDEDIR@',
                      ],
  verbose : true
)

libstrophe_dep = p.dependency('strophe')
meson.override_dependency('libstrophe', libstrophe_dep)
