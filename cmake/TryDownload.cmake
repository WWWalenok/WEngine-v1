
function(TryDownload_F URL DIRRECTORY DOWNLOAD_PATH)
    string(REPLACE "+" "%2B" URL_ESCAPED ${URL})
    
    if(NOT EXISTS "${DIRRECTORY}")
        message(STATUS "Downloading ${DOWNLOAD_PATH} from ${URL_ESCAPED}...")

    file(
            DOWNLOAD "${URL_ESCAPED}" "${DOWNLOAD_PATH}"
        SHOW_PROGRESS
        )
    execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${DOWNLOAD_PATH}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs
        )
        file(REMOVE "${DOWNLOAD_PATH}")
    else()
        message(STATUS "Dont need downloading ${DOWNLOAD_PATH}")
    endif()
endfunction()

function(TryDownload lib URL DIRRECTORY)
    set(${lib}_DIRRECTORY ${DIRRECTORY} CACHE INTERNAL "${lib}_DIRRECTORY" FORCE)
    set(DOWNLOAD_PATH "${DIRRECTORY}.zip")
    TryDownload_F(${URL} ${DIRRECTORY} ${DOWNLOAD_PATH})
    set(${lib}_INCLUDE_DIR "${DIRRECTORY}/include" CACHE INTERNAL "${lib}_INCLUDE_DIR" FORCE)
endfunction()

function(TryFindOrDownload lib URL DIRRECTORY)
    
    find_package(${lib})
    set(${lib}_DIRRECTORY "Not installed" CACHE INTERNAL "${lib}_DIRRECTORY" FORCE)
    if(NOT ${lib}_FOUND)
        TryDownload(${lib} ${URL} ${DIRRECTORY})
    endif()
endfunction()