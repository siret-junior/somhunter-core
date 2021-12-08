
set(HEADERS
  	task-target-helper.h
)

set(SOURCES
	${HEADERS}
	task-target-helper.cpp
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})
