#!/bin/sh
echo "TPA version 14"
echo "1..3"

i=$((1))

printf "%g" $(./rpnc -d '1.234') > /dev/null && echo "ok $i - the '-d' flag produces something that printf accepts" || echo "not ok $i - something is wrong with the '-d' flag"

i=$((i+1))

x="$(./rpnc -d ';1;3 3 *')"
[ "$x" -eq 1 ] && echo "ok $i - a third times three is exactly one" || echo "not ok $i - a third times three is not exactly one"

i=$((i+1))

x=$(./rpnc -r '1;2;3' | awk -F '#' '{print $1}' | tr -d '\t')
[ "$x" = "1;2;3;0" ] && echo "ok $i - the '-r' flag works as expected" || echo "not ok $i - the '-r' flag doesn't work right: «$x» =/= 1;2;3;0"

i=$((i+1))


