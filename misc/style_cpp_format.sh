#!/bin/sh
cd `dirname $0` #lol
echo `dirname $0`
clang-format -style=file -verbose -i ../src/*.hpp ../src/*.h ../src/*.cpp ../*.h ../*.cpp
