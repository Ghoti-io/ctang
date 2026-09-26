# CTang

Tang is a new <b>T</b>emplate l<b>ANG</b>uage intended to be embedded into a host program and specifically targeted to generate HTML.  It is loosely typed and garbage collected (under development).  A program is compiled at run time to x86-64 machine code, but can fall back to a bytecode interpreter when the JIT is unavailable or fails.  The two backends are required to agree.

The language is specified in [documentation/language-reference.md](documentation/language-reference.md), which also lists what is not implemented yet. The language is under active development and the feature set is unstable.

## History

I wrote the first incarnation in C++, implemented using an AST tree-walking interpreter, and relying on C++ `shared_ptr` for memory management (i.e., only reference counting).

The second incarnation was written in C++ with a bytecode interpreter, but still relied on the `shared_ptr` for memory management.  It was done as part of a 100+ episode [YouTube series](https://www.youtube.com/playlist?list=PLZqirAnnqaCZ8lT8w7p2PUB7tqrId7d89).  It is boring.  It is live coding.  Nothing is scripted.  Watch it at your own risk.

This is the third incarnation, written in C, with a JIT, bytecode VM fallback, proper garbage collection, etc.

## Before you call it

- Strings are graphemes. Indexing, slicing and `length` count graphemes (user-perceived characters). For example, the flag of Scotland is 28 bytes of UTF-8 (`\xF0\x9F\x8F\xB4\xF3\xA0\x81\xA7\xF3\xA0\x81\xA2\xF3\xA0\x81\xB3\xF3\xA0\x81\xA3\xF3\xA0\x81\xB4\xF3\xA0\x81\xBF`), which represents 7 Unicode code points, but they all combine to be exactly one grapheme; `length` is 1 and `byte_length` is 28.
- The host decides what the template can touch. There is no file system, network or clock unless the host provides one through a library specifically written for Tang, resolved by name when the template runs. Limits can be set on memory and on execution time.
- Output carries an encoding. HTML escaping is a property of the string the host applies when it renders, not a call the template has to remember.  
- Code is interspersed with the literal template text. `{` and `}` mark a code block, `<% %>` the same, and `<%= %>` prints a value. Statements end with a semicolon. Tang has arrays, maps, and a slice operator.

## Examples

### A value

Suppose `user` is the string `"Alice"`.

```
Welcome, <%= user %>!
```

```
Welcome, Alice!
```

### A loop

Suppose `users` is `"Alice"`, `"Bob"`, `"Carol"`, `"Frank"`.

```
<ul>
<%
for (name : users) {
  print("  <li>");
  print(name);
  print("</li>\n");
}
%>
</ul>
```

```
<ul>
  <li>Alice</li>
  <li>Bob</li>
  <li>Carol</li>
  <li>Frank</li>
</ul>
```

The same loop can sit in the markup. That form keeps the line breaks of the
template:

```
<ul>
<% for (name : users) { %>
  <li><%= name %></li>
<% } %>
</ul>
```

```
<ul>

  <li>Alice</li>

  <li>Bob</li>

  <li>Carol</li>

  <li>Frank</li>

</ul>
```

## Running it

`make` builds the shared library and a `tang` binary.

```bash
tang -s -e 'print(1 + 2);'
```

```
3
```

```bash
tang template.tang
```

`-s` / `--script` treats the source as a script. Without it, the source is a
template. `-e` / `--evaluate` takes the program on the command line; otherwise
`tang` reads a file, or stdin.

## Compile and link

Once the library is installed, pkg-config carries the include path, the
library, and its dependencies:

```bash
cc -o host host.c $(pkg-config --cflags --libs ghoti.io-tang-0)
```

The module name ends in the major version, `-0` for this release, so two
majors can be installed side by side. A build made with `make BRANCH=-dev`
installs `ghoti.io-tang-dev` instead.

## Building the library

[cutil](https://github.com/Ghoti-io/cutil) and
[unicode](https://github.com/Ghoti-io/unicode) must already be installed where
pkg-config can see them. Grapheme boundaries come from unicode. A dependency
pkg-config cannot find is a hard error naming the fix.

```bash
sudo apt install g++ make bison flex build-essential pkgconf libgtest-dev \
    doxygen graphviz
```

`bison` and `flex` generate the parser. Google Test builds the tests. Doxygen
and Graphviz build the manual.

```bash
make
make test
sudo make install
```

From the parent of a suite checkout:

```bash
./suite/install.sh
export PKG_CONFIG_PATH="$PWD/.local/share/pkgconfig"
make -C libs/ctang test PREFIX="$PWD/.local"
```

`make test` is the suite, under both backends. `make help` lists the rest.

| Target | What it does |
| --- | --- |
| `make` | The library and the `tang` binary |
| `make test-asan` | Rebuild with ASan and UBSan and run the tests |
| `make jit-alignment-check` | Rebuild with the JIT stack-alignment check and run the tests |
| `make docs` | The Doxygen manual, into `./docs` |
| `make test-watch` | Recompile and run the tests when a file is saved |

## The API

`<ghoti.io/tang/tang.h>` is the umbrella: a program, a computed value, and
the libraries a host installs for a template to call. The language reference
describes what the template can see.

The JIT is x86-64. On any other architecture the bytecode interpreter is the
execution path.

[Before you call it](#before-you-call-it) is what changes about a template
before it runs.

## Dependencies

Found through pkg-config, and the installed `.pc` file names them, so a
program that links `ghoti.io-tang-0` links these too.

- [ghoti.io-cutil](https://github.com/Ghoti-io/cutil) — the allocator.
- [ghoti.io-unicode](https://github.com/Ghoti-io/unicode) — grapheme boundaries.

## Documentation

[documentation/language-reference.md](documentation/language-reference.md)
is the language, including what is not implemented yet. `make docs` builds the manual.

## Status

Usable for the language the reference describes, and unstable past that: a feature can still change.

## License

LGPL-3.0-only. See [COPYING.LESSER](COPYING.LESSER) for the license, and [COPYING](COPYING) for the GPL text it is written as additional permissions on top of.

Contributions are not being accepted at this time; see
[CONTRIBUTING.md](CONTRIBUTING.md) for what is useful instead.
