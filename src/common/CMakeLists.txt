
set(HEADERS
	common.h
	common-types.h
  	utils.hpp
	static-logger.hpp
)

set(SOURCES
	${HEADERS}
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})

add_subdirectory(json)
add_subdirectory(math)
add_subdirectory(network)