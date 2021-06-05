
set(HEADERS
	dataset-frames.h
	dataset-features.h
)

set(SOURCES
	${HEADERS}
	dataset-frames.cpp
	dataset-features.cpp
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})
