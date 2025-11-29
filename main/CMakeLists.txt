idf_component_register(
    SRCS
        "main.c"
        "src/dht22.c"
        "src/mq135.c"
        "src/relay.c"
        "src/webserver.c"
    INCLUDE_DIRS 
        "include"
    EMBED_FILES
        "data/page.html.gz"
        "data/style.css.gz"
        "data/script.js.gz"
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_target(gzip_assets ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Building web assets (gzip + hashes)"
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/gzip_assets.Python3"
    DEPENDS
        "$CMAKE_CURRENT_SOURCE_DIR/data/page.template.html"
        "$CMAKE_CURRENT_SOURCE_DIR/data/style.css"
        "$CMAKE_CURRENT_SOURCE_DIR/data/script.js"
        "$CMAKE_CURRENT_SOURCE_DIR/gzip_assets.py"
    VERBATIM
)

idf_component_register(MAIN_LIB main COMPONENT_LIB)
add_dependencies(${MAIN_LIB} gzip_assets)