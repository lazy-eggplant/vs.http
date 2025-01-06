`vs.http` is an HTTP server based on [mongoose](https://github.com/cesanta/mongoose) and [vs.templ](https://github.com/lazy-eggplant/vs.templ).  
It serves as demo project for _vs.templ_, but it works and might have some practical value.

It has a quite small memory footprint, mostly thanks to _mongoose_ and _pugixml_ which are decently optimized.  
If you are targetting embedded devices, you might want to configure some flags in \_pugixml_to further reduce its memory usage.

## Building

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

(but PRs are accepted)

- File streaming
- Cache HTTP headers to avoid reloads
