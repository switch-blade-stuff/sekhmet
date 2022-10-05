### TODO:

1. Remove the `read`/`write` API for serialization, instead use `get`/`set`/`as`.
2. Refactor type name generation to strip out cv-qualifiers, struct, class & union keywords and stray spaces. This would
   allow for cross-compiler type names.
3. Inject type factory as an optional argument to in-place constructed attributes, to allow the attribute attach
   dependency metadata.