idf_component_register(
    SRCS
        "sensors.c" "i2c/i2c.c"
        "bare-drivers/bh1750/bh1750.c"
    INCLUDE_DIRS
        "." "i2c"
        "bare-drivers/bh1750/include"
    REQUIRES
        driver
)