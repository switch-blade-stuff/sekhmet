### Plugin compilation & hot-reload

Bue to the quirks of certain shared library linking implementations (such as GNU's `STB_GNU_UNIQUE` and musl),
dynamic hot-reload cannot be guaranteed (i.e. a shared library may never be unloaded). To mitigate these issues,
shared plugin libraries can be compiled with `-wno-gnu-unique` for GCC, however this is a platform-specific hack.
In addition, while on Win32 DLLs should be unloaded once the reference count reaches 0, it may change in the future
and break core. Issues may also arise when a plugin shared library is linked by another plugin library, as such
even if the first one is `dlclose`d, it's reference count would not reach 0 and it will never be unloaded.

The only platform-independant future-proof solution is to completely re-load editor state, where unpon
re-compilation of a plugin library, the editor would request a "deep" restart, completely unloading itself and all
dependant libraries and starting an entirely new process from scratch (with the same commandline arguments).
While this is a subpar solution, native plugins rely on OS- and platform-specific functionality, and in order
to guarantee compatibility and correct functionality sacrifices need to be made. Additionally, non-native
scripting (ex. via a JIT-compiled scripting language) is preferable for project-specific logic that requires
quick iteration, with native scripting mainly being used to implement large-scope or performance-critical
systems (ex. a physics server or an AI processing system).

This is not an issue outside of the editor, as plugin hot-reload is only supported within the editor runtime.
