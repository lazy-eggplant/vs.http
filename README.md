`vs.http` is an HTTP server based on [mongoose](https://github.com/cesanta/mongoose) and [vs.templ](https://github.com/lazy-eggplant/vs.templ).  
It serves as demo project for _vs.templ_, but it works and might have some practical value.

It has a quite small memory footprint, mostly thanks to _mongoose_ and _pugixml_ which are decently optimized.  
If you are targetting embedded devices, you might want to configure some flags in _pugixml_ to further reduce its memory usage.

## Building

> [!IMPORTANT]  
> You will not be able to build it with normal meson for an [unrelated issue](https://github.com/mesonbuild/meson/pull/14073) whose PR is in waiting list.  
> For now, you will need to patch meson manually if you really want to build this example.

The current build is explicitly using static deps for portability.

```bash
meson setup build
meson install --skip-subprojects -C build
```

## Usage

After building it, run this to see the default example.

```bash
cd example
../build/vs.http
```

By default, it will load the `server.cfg` in that location. Else its default internal profile will be used.  
A profile can also be passed as the first and only argument.

## Not in scope

But PRs are accepted:

- File streaming
- Cache HTTP headers to avoid reloads
- A configurable cache of pre-generated files

PRs will not be accepted:

- Support for methods other than GET. There is no way to support actions able to mutate the state, or externally get data without introducing other scripting capabilities or dynamic library loading.

## Licence

It is forced to GPL-2.0 by _mongoose_.  
Clearly this does not cover your own files and configuration, only the engine itself.
