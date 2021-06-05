
set(HEADERS
  	scores.h
	search-context.h
	user-context.h
)

set(SOURCES
	${HEADERS}
	scores.cpp
	search-context.cpp
	user-context.cpp
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})
