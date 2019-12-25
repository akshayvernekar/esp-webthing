set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "web_thing.c"
                   "web_thing_adapter.c"
                   )

set(COMPONENT_REQUIRES 
	json
	nvs_flash
	esp_http_server
	mdns
	)
register_component()
