idf_component_register(SRCS
                            "gui.c"
                            "font.c"
                      INCLUDE_DIRS "."
                      REQUIRES driver freertos
                )
