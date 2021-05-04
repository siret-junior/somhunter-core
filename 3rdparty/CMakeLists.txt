
target_include_directories(somhunter 
	PUBLIC
		stb
		json11
		cereal/include
		hash-library
)

set(HEADERS
	json11/json11.hpp
	json11/json11.hpp
	hash-library/sha256.h
)

set(SOURCES
	${HEADERS}
	json11/json11.cpp
	hash-library/sha256.cpp
)

target_sources(somhunter 
	PUBLIC
		${SOURCES}
)