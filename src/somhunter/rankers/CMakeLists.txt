
set(HEADERS
	canvas-query-ranker.h
	embedding-ranker.h
	keyword-ranker.h
	keyword-clip-ranker.h
	relocation-ranker.h
)

set(SOURCES
	${HEADERS}
	canvas-query-ranker.cpp
	embedding-ranker.cpp
	keyword-ranker.cpp
	keyword-clip-ranker.cpp
	relocation-ranker.cpp
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})
