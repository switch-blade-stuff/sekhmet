### Service locator

Services are global singletons of a particular type. They can either be obtained directly via the service type's
`instance` function, or via the service locator. Service locator maintains a database of services indexed by their
service type. The database can contain both owned (managed lifetime) and non-owned (external lifetime) instances
of service implementation objects.

Service locator can dynamically load and instantiate implementation types either directly via a `type_info` or via a
service id, assigned using the service `implements_service<S>` attribute. In either case, a service implementation type
should have be reflected as child of `S` with the `implements_service<S>` attribute, where `S` is the parent service
type. The attribute can also be supplied with a "debug name" string, which would be used for debugging and/or display
purposes, and an "id" string, which is used to select the service implementation at runtime. If a debug name or id
strings are not manually specified, the type name of the implementation type is used instead.

The user can also manually provide a service implementation instance, in which case the service locator will not check
for the presence of an `implements_service<S>` attribute or parent-child relationship with `S`. The lifetime of such
service instance would not be managed by the service locator and would be the responsibility of the user.

To notify the user of service instance changes, service locator provides the `on_load` and `on_reset` events,
where `on_load` is invoked when a new instance of a service is loaded (either dynamically or externally),
while `on_reset` is invoked then the implementation instance is released. If the user replaces an existing
implementation via `load`, first `on_reset` is invoked for the old instance, then `on_load` is invoked for the newly
loaded instance.

The `instance` static function of individual `service<S>` types accesses the service locator once on first call, after
which it caches the internal instance pointer for fast access. The internal service pointer is guaranteed to be
maintained throughout the lifetime of the service locator.