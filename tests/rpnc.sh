#!/bin/sh
echo "TPA version 14"
echo "1..42"

i=$((1))

printf "%g" $(./rpnc -d '1.234') > /dev/null && echo "ok $i - the '-d' flag produces something that printf accepts" || echo "not ok $i - something is wrong with the '-d' flag"

i=$((i+1))

x="$(./rpnc -d ';1;3 3 *')"
[ "$x" -eq 1 ] && echo "ok $i - a third times three is exactly one" || echo "not ok $i - a third times three is not exactly one"

i=$((i+1))

x=$(./rpnc -r '1;2;3' | awk -F '#' '{print $1}' | tr -d '\t')
[ "$x" = "1;2;3;0" ] && echo "ok $i - the '-r' flag works as expected" || echo "not ok $i - the '-r' flag doesn't work right: «$x» =/= 1;2;3;0"


for f in acos acosh asin asinh atan atanh cbrt ceil cos cosh erf erfc exp exp2 expm1 fabs floor lgamma log log10 log1p log2 logb nearbyint rint round sin sinh sqrt tan tanh tgamma trunc j0 j1 y0 y1 significand exp10
do
	i=$((i+1))
	x=$(./rpnc -d "1.0 $f")
	q=$?
	if [ $q -eq 0 ]; then
		echo "ok $i - function $f exists, $f(1.0) returns some value ($x);"
	else
		echo "not ok $i - function $f should exist but doesn't ($x)"
	fi
done
