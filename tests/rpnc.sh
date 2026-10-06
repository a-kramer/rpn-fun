#!/bin/sh
echo "TAP version 14"
echo "1..45"

i=0

# Test exit status
assert_cmd() {
	i=$((i + 1))
	name="$1"
	shift
	if "$@" > /dev/null 2>&1; then
		echo "ok $i - $name"
	else
		echo "not ok $i - $name"
	fi
}

# Test string equality
assert_eq() {
	i=$((i + 1))
	expected="$1"
	actual="$2"
	name="$3"
	if [ "$expected" = "$actual" ]; then
		echo "ok $i - $name"
	else
		echo "not ok $i - $name (expected '$expected', got '$actual')"
	fi
}

# Test integer equality
assert_int_eq() {
	i=$((i + 1))
	expected="$1"
	actual="$2"
	name="$3"
	if [ "$expected" -eq "$actual" ]; then
		echo "ok $i - $name"
	else
		echo "not ok $i - $name (expected '$expected', got '$actual')"
	fi
}


# --- Test Suite ---

assert_cmd "the '-d' flag produces something that printf accepts" \
	sh -c 'printf "%g" "$(./rpnc -d "1.234")"'

x=$(./rpnc -d ';1;3 3 *')
assert_eq "1" "$x" "a third times three is exactly one"

x=$(./rpnc -r '1;2;3' | awk -F '#' '{print $1}' | tr -d '\t')
assert_eq "1;2;3;0" "$x" "the '-r' flag works as expected"

# Math functions test loop
FUNCS="acos acosh asin asinh atan atanh cbrt ceil cos cosh erf erfc exp exp2 expm1 fabs floor lgamma log log10 log1p log2 logb nearbyint rint round sin sinh sqrt tan tanh tgamma trunc j0 j1 y0 y1 significand exp10"

for f in $FUNCS; do
	assert_cmd "function $f exists and evaluates" ./rpnc -d "1.0 $f"
done

# Address resolution tests
EXPECTED=$(printf "x\ty\tz\n1\t2\t3\n5\t9\t14\n10\t1\t11\n") #

x=$(./rpnc -H z 'A B +' < tests/test.tsv)
assert_eq "$EXPECTED" "$x" 'A-Z addressing'

x=$(./rpnc -H z '$1 $2 +' < tests/test.tsv)
assert_eq "$EXPECTED" "$x" '$n addressing'

x=$(./rpnc -H z 'x y +' < tests/test.tsv)
assert_eq "$EXPECTED" "$x" 'named addressing'
