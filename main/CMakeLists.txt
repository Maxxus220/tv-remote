idf_component_register(
    SRCS "ir_sensor.cpp" "real_time.cpp" "button.cpp" "main.cpp"
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/external/etl/include" "${CMAKE_CURRENT_LIST_DIR}/external/GSL/include"
)