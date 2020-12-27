
set(HEADERS
	tests.hpp
)

set(SOURCES 
	${HEADERS}
)

target_include_directories(somhunter 
	PUBLIC
		.
)

target_sources(somhunter 
	PUBLIC 
		${SOURCES}
)