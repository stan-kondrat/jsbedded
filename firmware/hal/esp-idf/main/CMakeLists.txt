include(${CMAKE_CURRENT_LIST_DIR}/../../../core/core.cmake)

idf_component_register(
    SRCS ${JSB_CORE_SOURCES} hal_esp.c
    INCLUDE_DIRS ${JSB_CORE_INCLUDE_DIRS}
)
jsb_apply_engine_options()
target_compile_definitions(${COMPONENT_LIB} PRIVATE ${JSB_CORE_DEFINITIONS})
if(JSB_CORE_LINK_LIBRARIES)
    target_link_libraries(${COMPONENT_LIB} PRIVATE ${JSB_CORE_LINK_LIBRARIES})
endif()
