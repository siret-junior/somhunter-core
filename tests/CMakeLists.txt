
set(HEADERS
	tests.hpp
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