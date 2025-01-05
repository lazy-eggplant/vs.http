project(
    'mongoose',
    ['c'],
    meson_version: '>= 1.1',
    #  default_options: ['cpp_std=c++20'],
)

mongoose_lib = static_library(
    'mongoose',
    [
        'mongoose.c',
    ],
    dependencies: [],
    include_directories: ['meson-include'],
    install: not meson.is_subproject(),
)

mongoose_dep = declare_dependency(
    link_with: mongoose_lib,
    include_directories: ['meson-include'],
)

pconf = import('pkgconfig')
pconf.generate(
    mongoose_lib,
    description: 'Embedded TCP/IP network stack with embedded web server, Websocket, and MQTT client',
    url: 'https://github.com/cesanta/mongoose',
    version: meson.project_version(),
)