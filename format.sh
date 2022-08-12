#!/bin/sh
find ./src -iname *.mm -o -iname *.h* -o -iname *.c* | xargs clang-format -style=File -i -verbose
