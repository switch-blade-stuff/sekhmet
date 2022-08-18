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
    * Implementation details are to be placed in the `detail` subdirectory, with top-level headers only containing relevant `#include` directives.
    * Logical API partitions (such as namespaces and sources related to a single system) can be placed in sub-directories (ex. `sekhmet/math`, `engine/detail/ecs`).
    * Third-party dependencies are to be placed in the `third-party` directory outside of the project source root.
 * Use the `.cpp`/`.hpp` extensions for C++ sources & headers and `.c`/`.h` extensions for C sources & headers respectively.
 * Prefer wither avoiding getter & setter functions altogether or use one of the following alternatives:
    * `T &some_value()` getter, `void some_value(T &)` setter.
    * `property_t some_value()` where `property_t` is an instance of the `sek::property_wrapper` template,
      used to simplify implementation of non-trivial and potentially overloaded accessors pairs.
 * While not strictly a requirement, prefer to use the `m_` prefix for `private` and `protected` member variables and no prefix for public member variables and member functions.

### Milestones

- [X] Core engine library
- [X] Template-based math library with `constexpr` and SIMD support
- [X] Runtime reflection system
- [X] Plugin system
- [ ] Compile-time configurable serialization library    (currently under rework)
    - [ ] Intermediate structured archive base           (currently under rework)
    - [ ] Json input & output archives                   (currently under rework)
    - [ ] UBJson input & output archives                 (currently under rework)
    - [ ] Raw binary input & output archives             (currently under rework)
- [ ] Asset/resource system. (currently under rework)
    - [ ] "local" asset loading                          (currently under rework)
    - [ ] Network config loading (via URI)
- [ ] Config management system
    - [X] "local" config saving/loading
    - [ ] Network config saving/loading (via URI)
- [ ] ECS
    - [X] Core ECS - world, component & entity sets, views & collections
    - [ ] Prefab & scene resource system
- [ ] Debuging client & server
    - [ ] Logger service
    - [ ] Profiler service
- [ ] UI framework/toolkit targeting
    - [ ] X11
    - [ ] DWM
- [ ] Fully-featured customizeable editor

### License

Sekhmet is licensed under a dual license. The engine and all its runtime components are distributed under the MIT
license, the editor and all other development tooling is distributed under the GPL license.
