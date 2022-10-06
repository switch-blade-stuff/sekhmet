# sekhmet

Sekhmet is an open-source C++20 game engine aiming to provide a relatively low level of entry while leaving a lot of
room for skill ceiling.

### Goals

* Open-source and free to use.
* Low reliance on external dependencies & tools.
* Straight-forward and extensible design.
* Highly customizable design via plugins.

### Contribution & style guidelines

* Follow modern (as of 2022) C++ practices.
* Use `shake_case` for all symbols except for `enum` values and `#define` macros.
* Use `clang-format` for formatting, `.clang-format` configuration is provided.
* Follow a mostly flat project structure with the following exceptions:
    * Implementation details are to be placed in the `detail` subdirectory, with top-level headers only containing
      relevant `#include` directives.
    * Logical API partitions can be placed in sub-directories (ex. `engine/detail/ecs`).
* Use the `.cpp`/`.hpp` extensions for C++ sources & headers and `.c`/`.h` extensions for C sources & headers
  respectively.
* Prefer either avoiding getter & setter functions altogether or use one of the following alternatives:
    * `T &some_value()` getter, `void some_value(T &)` setter.
    * `property_t some_value()` where `property_t` is an instance of the `sek::property_wrapper` template,
      used to simplify implementation of non-trivial and potentially overloaded accessors pairs.
* While not strictly a requirement, prefer to use the `m_` prefix for `private` and `protected` member variables and no
  prefix for public member variables and member functions.

### License

Sekhmet is licensed under the MIT license.
