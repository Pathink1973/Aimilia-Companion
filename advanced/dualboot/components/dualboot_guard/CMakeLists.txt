idf_component_register(SRCS "guard.c" INCLUDE_DIRS "."
    REQUIRES app_update nvs_flash esp_partition)
target_link_libraries(${COMPONENT_LIB} INTERFACE
    "-Wl,--wrap=app_main" "-Wl,--wrap=esp_ota_begin"
    "-Wl,--wrap=esp_ota_resume")
if(CONFIG_DUALBOOT_XIAO_NVS)
    foreach(api nvs_open nvs_open_from_partition nvs_flash_init
            nvs_flash_init_partition nvs_flash_erase nvs_flash_erase_partition
            nvs_flash_deinit nvs_flash_deinit_partition)
        target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=${api}")
    endforeach()
endif()
