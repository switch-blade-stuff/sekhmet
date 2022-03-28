### Component serialization

While the ECS system does not require components to be explicitly declared, in order to be able to serialize
components (for use in prefabs and such) they need to have a component factory that would add them to entities (and
potentially do other initialization). In order to do this, a special attribute `component_factory_attribute` would be
needed.

### Assets & resources

Every file that the engine uses is an "asset". Assets can be loaded from either loose files or asset packs. Within the
engine, assets are represented using asset references. Asset references are unique IDs (UUIDs) that are mapped to an
asset path. This is done in order to allow for asset overwriting, as well as simplify editor asset management - updating
an asset's path will not require a change to all other assets that depend on it, only the central asset table will need
to be updated.

Assets are supplied to the engine via asset packages. Asset packages can be either loose (in which case a package is a
directory) or archived (in which case a package is a special archive meant for streaming). Every asset package must
contain a ".manifest" configuration file which would contain a map of asset UUIDs to their paths within the package.
When an asset package is loaded, first it's manifest is read and every asset is added to the internal asset table. If
multiple packages contain an asset with the same UUID, the most recently loaded package's asset will be used.

Assets can be loaded via either their UUID or an asset path (in this case, id indirection is impossible). Internally,
the asset database is stored as a hash map of UUIDs and asset paths.

Resources are just assets of UBJson format (.ubj) that contain serializable information.