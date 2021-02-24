
target_include_directories(somhunter 
	PUBLIC
		stb
		json11
		cereal/include
)

set(HEADERS
	json11/json11.hpp
	json11/json11.hpp
)

set(SOURCES
	${HEADERS}
	json11/json11.cpp
)

target_sources(somhunter 
	PUBLIC
		${SOURCES}
)