### Prefab/Level/Scene system

* Prefab - resource containing serialized data of a portion of an ECS world (entities & components), as well as
  serialized systems.
* Level - "extended" prefab resource, containing per-level information, such as the background color/skybox
* Scene - runtime instance of one or multiple levels. Contains the actual ECS world and system instances used for
  execution of the scene.
* Scene view - UI element used to display the current scene and handle signals such as user input.
    * Editor view - scene view used for the current editor scene.
    * Player view - scene view used for the current player scene.

### Editor play mode

When the play mode is entered in editor, editor scene serializes it's world & systems (saving modified levels and
prefabs if required), after which it creates a duplicate scene from the serialized data. This is necessary in order to
ensure correct initialization of components and systems, since a regular copy is likely to leave stale and/or invalid
references into the old scene, as such a deep copy via serialization is preformed. In addition, if the user enables
autosave on play, the serialized data would be used to save level and prefab assets.