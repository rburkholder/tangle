# web.boost

## introduction

* Use Boost Beast Advanced server, flex (plain + SSL) as template for handling web operations.
* Currently developed on Debian Linux platform

## sample configuration file (www.cfg)
```
thread_count = 2
port_http = 80
port_https = 443
listen_address = 0.0.0.0
content_directory = web/content
static_host = static.example.com
static_directory = web/static
static_extension = jpg
static_extension = jpeg
static_extension = png
static_extension = css
static_extension = txt
static_extension = ico
static_extension = gif
certificate_path_fullchain = certs/fullchain.pem
certificate_path_privkey = certs/privkey.pem
```
## build outline
* compile environment: C++20
* requires libssl-dev, libboost-dev (json, log, program_options, serialization, url)
* add luajit and sol2

### build lua modules: luajit, sol2
```bash
# acquire, build & install luajit
git clone --depth=1 https://github.com/LuaJIT/LuaJIT.git
pushd LuaJIT
sed -i 's/#XCFLAGS+= -DLUAJIT_ENABLE_LUA52COMPAT/XCFLAGS+= -DLUAJIT_ENABLE_LUA52COMPAT/' src/Makefile
make
sudo make install
popd

# acquire & install sol2 (lua helpers for c++)
git clone --depth=1 https://github.com/ThePhD/sol2.git
sudo mv -n sol2/include/sol /usr/local/include/
```

### build fmt
```bash
git clone --depth=1 https://github.com/fmtlib/fmt.git
pushd fmt
mkdir build
cd build
cmake -D FMT_TEST=FALSE ..
make
sudo make install
popd
```

### build project
```bash
mkdir build
cd build
cmake ..
make
```

## security, run
To run on a port under 1024, requires something like:
```bash
sudo setcap CAP_NET_BIND_SERVICE=+eip ~/projects/web.boost/build/src/boost.web
~/projects/web.boost/build/src/boost.web
```

## current features
* 2026/08/09
  * use [sol2](https://sol2.readthedocs.io/en/latest/)/[luajit](https://luajit.org/luajit.html) to load and run [lua](https://www.lua.org/manual/5.1/) files
  * integrates [lua based html generator by riki moe (りき萌)](https://riki.house/lua-html#The-part-where-I-do-the-thing)
* 2026/08/02
  * GET static html and support files from a directory
  * supports HTTP and HTTPS

## sample test page (test.lua)
* influenced by riki's demo code
* copy src/lua/html.lua to web/lib
* create test.lua in web/content
  ```lua
  package.path="web/lib/?.lua"

  local h = require( 'html' )

  local output = h.Document{
    lang = "en",

    h.head{
      h.meta{charset = "UTF-8"},
      h.title{"Hello, world!"},
    },

    h.body{
      h.h1{"Hello, world!"},
      h.p{
        "This is an example of the little HTML templating framework."
      },
      h.p{
        "As you can see, it is fully capable of generating any kind of markup ",
        "you'd ever want.", h.br(),
        "It can even do things like ", h.b"bold text", "!"
      },
      h.p"This is some embedded HTML <p></p>"
    }
  }
  Render( 'text/html', tostring( output ) )
  ```

## alternatives
* [wt web toolkit](https://www.webtoolkit.eu/wt/) - designed for single page applications with total control over page generation - not recommeded for internet centric applications - no lua
* [drogon web framework](https://drogon.org/) - excellent functionality but comes up short on SSL capability and reliability - proprietary web template, no lua
* [openresty](https://github.com/openresty/openresty) - Web Platform Based on Nginx and LuaJIT - may come back to this
