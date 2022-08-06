# sekhmet

Sekhmet is an open-source C++20 game engine aiming to provide a relatively low level of entry while leaving a lot of
room for skill ceiling.

### Goals

* Open-source and free to use (LGPL license).
* Low reliance on external dependencies. No frameworks, but independent libraries are allowed.
* Expose as much functionality to the user as possible. Allow the user to make light customizations of the engine
  without forking the project.
* Follow modern (as of 2022) C++ practices.
* Easy to follow and understand design.

### Milestones

- [X] Core engine library.
- [X] Template-based math library with `constexpr` and SIMD support.
- [X] Runtime reflection system.
- [X] Plugin system.
- [X] Config management system 
- [X] Asset/resource system.
- [ ] ECS
- [ ] UI framework/toolkit targeting
   - [ ] X11 
   - [ ] DWM
- [ ] Fully-featured editor.

### License

Sekhmet is licensed under a dual license. The engine and all its runtime components are distributed under the MIT
license, the editor and all other development tooling is distributed under the GPL license.
