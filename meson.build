project(
  'website',
  ['cpp'],
  version: '0.1.0',
  #TODO: set to 1.7 once available
  meson_version: '>= 1.1',
  default_options: ['cpp_std=c++20'],
  license: 'GPL-2',
)

cmake = import('cmake')

pugixml_dep = dependency('pugixml', version: '>=1.14', required: false, static: true)
if pugixml_dep.found() == false
  pugixml_proj = subproject(
    'pugixml',
    default_options: ['default_library=static'],
  )
  pugixml_dep = pugixml_proj.get_variable('pugixml_dep')
endif

templ_proj = subproject(
  'vs-templ',
  default_options: ['default_library=static'],
)
templ_dep = templ_proj.get_variable('vs_templ_dep')

mongoose_proj = subproject(
  'mongoose',
  default_options: ['default_library=static'],
)
mongoose_dep = mongoose_proj.get_variable('mongoose_dep')

canfigger_proj = subproject(
  'canfigger',
  default_options: ['default_library=static'],
)
canfigger_dep = canfigger_proj.get_variable('canfigger_dep')

server = executable(
  'vs.http',
  ['src/main.cpp'],
  dependencies: [pugixml_dep, templ_dep, mongoose_dep, canfigger_dep],
  include_directories: ['include'],
  install: not meson.is_subproject(),
)

if get_option('tests')
  #subdir(['./test/'])
endif