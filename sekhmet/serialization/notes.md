### Output

Writing to output archives is done via `write` & `operator<<` member functions. `write` writes an entry to the archive
and throws `archive_error` exception on failure. `write` may fail due to any implementation-defined error. `operator<<`
simply calls `write`.

In addition to `write` & `operator<<`, output archives should provide `flush` member function, which can be used to
commit any buffered changes (be it the internal archive buffer or the destination stream buffer) to the destination
stream or buffer.

### Input

Reading from input archives is done via `read` & `operator>>` member functions. `read` reads the current entry of the
archive & advances to the next entry on success. On failure, it throws `archive_error` exception. `read` may fail due to
the archive being exhausted (no more entries left), entry type mismatch, and any other implementation-defined
error. `operator>>` simply calls `read`.

Read has 2 overloads: `void read(T &)` uses the passed reference, while `T read()` uses a default-constructed instance &
returns it.

To attempt a read without causing an exception, `try_read` function should be provided by all input archives. This
function attempts to read the next entry and returns `true` if read successfully. If the read has failed, `try_read`
returns false and the archive state is left unmodified. `try_read` may throw exceptions not caused by deserialization (
such as stream IO errors, memory allocation errors and other fatal errors).

Input archives may be treated as containers, who's `begin` & `end` member functions return iterators to entries of the
archive. Entry iterators must implement the `forward_iterator` concept and point to an implementation-defined entries.
Entries should have `read`, `operator>>` & `try_read` member functions, which would preform the corresponding operations
on the handle. They should also have a perfect explicit cast operator, which preforms a read on the entry.

Archives may implement container functionality themselves, or through some kind-of intermediate frame/view sub-archive.

Note that entry iterators are not required to preserve the actual order of entries and should not be relied upon for
ordered entry access. It is, however, guaranteed that entries can be read in the same order as they were written.

If a container-like archive supports keyed entries, entry iterators should provide an ability to get keys of the
pointed-to entries.

Input archives implementing container functionality must implement the `Container` standard keyed requirement
with `value_type` being the implementation-defined entry type.

### Manipulators

Manipulators preform defined operations on the archives and can be applied to both input & output archives. Support for
manipulators is defined by the archive's policy. Reading/writing a manipulator to/from a stream applies it to the said
steam. Archives may ignore unsupported manipulators.

### Thread-safety

While serialization does not utilize any global state, serialization operations preformed on the same archive are not
synchronized. Thus, if an archive must be read by multiple threads, an external synchronization mechanism is required.