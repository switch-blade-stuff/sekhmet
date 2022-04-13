### Output

Writing to output archives is done via `write` & `operator<<` member functions. `write` writes an entry to the archive
and throws `archive_error` exception on failure. `write` may fail due to any implementation-defined error. `operator<<`
simply calls `write`.

### Input

Reading from input archives is done via `read` & `operator>>` member functions. `read` reads the current entry of the
archive & advances to the next entry on success. On failure, it throws `archive_error` exception. `read` may fail due to
the archive being exhausted (no more entries left), entry type mismatch, and any other implementation-defined
error. `operator>>` simply calls `read`.

Read has 2 overloads: `void read(T &)` uses the passed reference, while `T read()` uses a default-constructed instance &
returns it.

To attempt a read without causing an exception, `try_read` function is provided. This function attempts to read the next
entry and returns `true` if read successfully. If the read has failed, `try_read` returns false and the archive state is
left unmodified.

Input archives can be treated as containers, who's `begin` & `end` member functions return iterators to entries of the
archive. Entry iterators must implement the `forward_iterator` concept and their `operator*` must return an
implementation-defined handle to the entry. Entry handles should have `read`, `operator>>` & `try_read` member
functions, which would preform the corresponding operations on the handle. They should also have a perfect explicit cast
operator, which preform a read on the entry. It should also be possible to directly call `read`, `operator>>`
& `try_read` on the entry iterator, which would forward the call to the entry.

### Modifiers

Modifiers preform defined operations on the archives and can be applied to both input & output archives. Support for
modifiers is defined by the archive's policy.

Following modifiers can be supported by output archives:

* `named_entry(name, value)` - Creates or updates an entry with a given explicit name. Supported only if the archive
  has `named_entry_policy` entry policy.
* `sequence(size)` - Starts a sequence (array of entries) with a fixed size. Modifier contains size of the sequence.
  Supported only if the archive has `fixed_sequence_policy` sequence policy.
* `sequence()` - Starts a sequence (array of entries) with unspecified size. Must be supported by all output archives.

Following modifiers can be supported by input archives:

* `named_entry(name, out_value)` - Seeks & reads an entry with a given explicit name. Fails if such entry is not
  present. Supported only if the archive has `named_entry_policy` entry policy.
* `sequence(out_size)` - Reads size of the sequence (array of entries). Fails if the current entry does not have a
  size (either the entry is not a sequence or it does not have a fixed size). Supported only if the archive
  has `fixed_sequence_policy` sequence policy.

Archives may ignore unsupported modifiers.