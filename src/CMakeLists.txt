
set(HEADERS
)

set(SOURCES
	${HEADERS}
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS} main.cpp)

# No main.cpp for the tests target
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})

add_subdirectory(config)
add_subdirectory(extractor)
add_subdirectory(network-api)
add_subdirectory(somhunter)
add_subdirectory(common)
