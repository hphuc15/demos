idf_component_register(
    SRCS
        "main.c"
        "app/app.c" "app/wifi_button.c"
    INCLUDE_DIRS
        "." "app"
    REQUIRES
        sensors network
)
