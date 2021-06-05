
set(HEADERS
	query-types.h
	somhunter.h
	settings.h
)

set(SOURCES
	${HEADERS}
	query-types.cpp
	somhunter.cpp
	settings.cpp
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})

add_subdirectory(datasets)
add_subdirectory(evaluation-server)
add_subdirectory(logs)
add_subdirectory(rankers)
add_subdirectory(searches)
add_subdirectory(soms)
