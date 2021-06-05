
set(HEADERS
  	som.h
	async-som.h
)

set(SOURCES
	${HEADERS}
	som.cpp
	async-som.cpp
)

target_include_directories(${SOMHUNTER_TARGET} PRIVATE .)
target_include_directories(${SOMHUNTER_TARGET_TESTS} PRIVATE .)

target_sources(${SOMHUNTER_TARGET} PRIVATE ${SOURCES} ${HEADERS})
target_sources(${SOMHUNTER_TARGET_TESTS} PRIVATE ${SOURCES} ${HEADERS})
