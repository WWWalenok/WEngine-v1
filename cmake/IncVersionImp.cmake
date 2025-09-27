

IF(EXISTS ${IncVesionCacheFile})
    file(READ ${IncVesionCacheFile} INCREMENTED_VALUE)
    math(EXPR INCREMENTED_VALUE "${INCREMENTED_VALUE}+1")
ELSE()
    set(INCREMENTED_VALUE "0")
ENDIF()
string(TIMESTAMP year  "%Y")
math(EXPR year ${year})
string(TIMESTAMP day   "%d")
math(EXPR day ${day})
string(TIMESTAMP month "%m")
math(EXPR month ${month})
string(TIMESTAMP time  "%H:%M:%S")
string(TIMESTAMP UTC   "%Y-%m-%dT%H:%M:%SZ")
string(TIMESTAMP TS    "%Y%j%H%M%S")
string(SHA256 HASH "${INCREMENTED_VALUE}${TS}${UTC}")
string(SUBSTRING ${HASH} 0 4 BUILD_VERSION_1)
string(SUBSTRING ${HASH} 15 4 BUILD_VERSION_2)
string(SUBSTRING ${HASH} 7 4 BUILD_VERSION_3)
set(BUILD_VERSION "${BUILD_VERSION_1}-${BUILD_VERSION_2}-${BUILD_VERSION_3}")
message("IncVesionCacheFile " ${IncVesionCacheFile})
message("IncVesionFile " ${IncVesionFile})
file(WRITE ${IncVesionCacheFile} "${INCREMENTED_VALUE}")
file(WRITE ${IncVesionFile} 
"#ifndef BUILD_VERSION_H\n\n\
int          BUILD_NUMBER() { return ${INCREMENTED_VALUE}; }\n\
int          BUILD_YEAR() { return ${year}; }\n\
int          BUILD_DAY() { return ${day}; }\n\
int          BUILD_MONTH() { return ${month}; }\n\
const char * BUILD_UTC() { return \"${UTC}\"; }\n\
long long    BUILD_TS() { return ${TS}; }\n\
const char * BUILD_VERSION() { return \"${BUILD_VERSION}\"; }\n\n\
const char * BUILD_TYPE() { return \"${BUILD_TYPE}\"; }\n\n\
#endif BUILD_VERSION_H\n\n\n\
")
