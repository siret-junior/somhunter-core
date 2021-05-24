
set(HEADERS
	
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
		tests.hpp
		config-tests.h
		main-tests.cpp
)

add_subdirectory(src)