
if(NOT DEFINED version_file)
    set(version_file version.h)
endif()

add_custom_target(
    Version ALL
    COMMAND ${CMAKE_COMMAND}
        "\"-DIncVesionCacheFile:STRING=${CMAKE_SOURCE_DIR}/${version_file}cache\""
        "\"-DIncVesionFile:STRING=${CMAKE_SOURCE_DIR}/${version_file}\""
        "\"-DBUILD_TYPE:STRING=${build_type}\""
        -P "\"${CMAKE_CURRENT_LIST_DIR}/IncVersionImp.cmake\""
)