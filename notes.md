### Tools

* lefticus json2cpp - Constexpr json resource building. Baking serialized Json configuration into the built binary.

### Component serialization

While the ECS system does not require components to be explicitly declared, in order to be able to serialize
components (for use in prefabs and such) they need to have a component factory that would add them to entities (and
potentially do other initialization). In order to do this, a special attribute `component_factory_attribute` would be
needed.

### Assets & resources

Every file that the engine uses is an "asset".

Assets are supplied to the engine via asset packages. Asset packages can be either loose (in which case a package is a
directory) or archived (in which case a package is a special archive meant for streaming). Every asset package must
contain configuration metadata which would contain a map of asset UUIDs to their paths within the package, as well as
other information such as whether the package is a master, list of package's fragments, etc. For "loose" packages the
metadata comes in form of the `.manifest` file inside the package's directory. For archive packages metadata is stored
as the header of the package file.

When an asset package is loaded, first it's manifest is read and every asset is added to the internal asset table. If
multiple packages contain an asset with the same UUID, the most recently loaded package's asset will be used. There is
an asset table for each individual package, as well as the global asset table. Assets can be searched for via either
their UUID or their string tags (ex. all "image" assets).

Archived packages can be fragmented. Packages that do not have the "master" flag set are considered to be fragments and
are ignored during package load. "Master" packages can specify a list of fragments, which will be loaded as a part of
their "master" package.

Resources are just assets of UBJson format (.ubj) that contain serialized structure data.