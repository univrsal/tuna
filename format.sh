#!/bin/sh
find ./src -iname *.h* -o -iname *.c* | xargs clang-format -style=File -i -verbose
