idf_component_register(
    SRCS
        "main.c"
        "src/dht22.c"
        "src/mq135.c"
        "src/adc.c"
        "src/relay.c"
        "src/webserver.c"
        "src/sensor_data.c"
        "src/display.c"
        "src/bmp280.c"
    INCLUDE_DIRS 
        "include"
    EMBED_FILES
        "data/page.html.gz"
        "data/style.css.gz"
        "data/script.js.gz"
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Создаем список выходных файлов (.gz)
set(GZIP_OUTPUT_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/data/page.html.gz"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/style.css.gz"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/script.js.gz"
)

# Создаем список входных файлов
set(GZIP_INPUT_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/data/page.template.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/style.css"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/script.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/gzip_assets.py"
)

# Добавляем кастомную команду для генерации файлов
add_custom_command(
    OUTPUT ${GZIP_OUTPUT_FILES}
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/gzip_assets.py"
    DEPENDS ${GZIP_INPUT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating compressed web assets"
    VERBATIM
)

# Добавляем кастомный таргет
add_custom_target(gzip_assets ALL DEPENDS ${GZIP_OUTPUT_FILES})

# Добавляем зависимость к основной библиотеке
add_dependencies(${COMPONENT_LIB} gzip_assets)