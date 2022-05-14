### Tools

* lefticus json2cpp - Constexpr json resource building. Baking serialized Json configuration into the built binary.

### Component serialization

While the ECS system does not require components to be explicitly declared, in order to be able to serialize
components (for use in prefabs and such) they need to have a component factory that would add them to entities (and
potentially do other initialization). For this reason components accessible at runtime need to be registered as
"prefab components".

### Assets & resources

Every file that the engine uses is an "asset".

Assets are supplied to the engine via asset packages. Asset packages can be either loose (in which case a package is a
directory) or archived (in which case a package is a special archive meant for streaming). Every asset package must
contain configuration metadata which would contain a map of asset ids (name & UUID) to their paths within the package,
as well as other information such as whether the package is a master, list of package's fragments, etc. For "loose"
packages the metadata comes in form of the `.manifest` file inside the package's directory. For archive packages
metadata is stored as the header of the package file.

Asset packages can be combined via asset repositories. When an asset package is added to a repository, every asset is
added to the repository's asset table. If multiple packages contain an asset with the same id, the most recently loaded
package's asset will be used. There is a global asset repository used for global asset lookup. Most users will only
access the global repository. Assets can be searched for via either their UUID or their string names.

Archived packages can be fragmented. Packages that do not have the "master" flag set are considered to be fragments and
are ignored during package load. "Master" packages can specify a list of fragments, which will be loaded as a part of
their "master" package.

Resources are assets that contain serialized structure data. By default, resources are serialized into UBJson (.ubj)
format. If a different format is desired, it should be specified during resource type registration.