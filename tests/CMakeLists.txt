
set(HEADERS
	tests.hpp
	config-tests.h
)

set(SOURCES 
	${HEADERS}
)

target_include_directories(somhunter 
	PUBLIC
		.
		../src
)

target_sources(somhunter 
	PUBLIC 
		${SOURCES}
)

target_include_directories(${SOMHUNTER_TARGET_TESTS}
	PUBLIC
		.
		../src
)

target_sources(${SOMHUNTER_TARGET_TESTS}
	PUBLIC 
		${SOURCES}
		main-tests.cpp
		ImageManipulator-tests.cpp
		CanvasQueryRanker-tests.cpp
)