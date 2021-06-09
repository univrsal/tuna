#!/bin/bash
#grep -rl 'Copyright 2020' ./src | xargs sed -i 's/Copyright 2020/Copyright 2021/g'
#grep -rl 'uni@vrsal.cf' ./src | xargs sed -i 's/uni@vrsal.cf/uni@vrsal.de/g'
find ./src -iname *.h* -o -iname *.c* | xargs clang-format -style=File -i -verbose
