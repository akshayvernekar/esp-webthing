set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_SRCS "web_thing.c"
                   "web_thing_adapter.c"
                   )

set(COMPONENT_REQUIRES json)
register_component()
