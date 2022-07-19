## Engine loop (runtime):

### Pre-init:

* Parse CMD
* Load config
* Load & enable plugins specified in project config

### Main init:

* Select & initialize render server from config (render server factories can be registered from plugins)
* Load & initialize renderer resource
* Select & load scenes specified in project config
