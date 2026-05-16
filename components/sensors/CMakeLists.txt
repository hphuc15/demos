idf_component_register(
    SRCS "sensors.c" "i2c/i2c.c"
    INCLUDE_DIRS "." "i2c"
    REQUIRES
        driver
        bare-drivers
)