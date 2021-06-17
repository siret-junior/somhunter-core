
set(HEADERS
	tests.h
	config-tests.h
	test-utils.hpp
)

set(SOURCES 
	${HEADERS}
	tests.cpp
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
)

add_subdirectory(src)