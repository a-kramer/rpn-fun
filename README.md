# Reverse Polish Notation

I always found reverse polish notation quite fun, it's easy to parse,
and one does not need to count parentheses.

There is a program called `dc`, which i like a lot. I never use its
advanced features (e.g. registers) and I am always a bit disappointed
that it doesn't have the functions from the math library (`math.h`).

In addition, there are certain inconvenciences:

```sh
dc -e '2 3 +'
```

has no result, you _have_ to write `p` for print. And for some reason this is the default behavior:

```sh
dc -e '1.2 2.3 + p'
```
```
3.5
```
```sh
dc -e '1.2 2.3 / p'
```
```
0
```

... mysterious.

For some reason, we must fix this by providing a _precision_ (with _k_ for _precision setting_ ), which wasn't necessary for the `+`:

```sh
dc -e '4 k 1.2 2.3 / p'
```
```
.5217
```

On the other hand, `dc` also has fancy operations like `~` which does both integer
division and remainder calculation (`/` and `%`) at the same
time. This is probably cool, I just have never used it.

Another unfortunate thing is that, in basic arithmetic, there is an unary minus: `-1` and a binary minus: `1 - 3` (this is generally true). The `dc` program removes this ambiguity in synax by doing this:

- unary minus: `_` (underscore `0x5f`), e.g. `1 _1 + p`
- binary minus: `-` (ascii-dash `0x2d`), e.g. `1 1 - p`

But, this convention is not carried over to the output of `dc`:

```sh
dc -e '3 _8 + p'
```
```sh
-5                 #  ... not _5
```

And thus:

```sh
x=$(dc -e '3 _8 + p')
dc -e "${x} 3 + p"
```
```sh
# dc: stack empty
8
```

You have to do this:

```bash
dc -e "${x/-/_} 3 + p"
```
```sh
-2
```

# Goals

I want to write a very similar program, that I can use in shell
scripts, whenever the coordination of some bigger job requires
non-integer values as parameters to some program.

I would like to avoid (as much as possible) operators where it is
important to know the order of popped values, because I will need to
check the manual to see which is popped first, the numerator or the
denominator. This concerns `-` (minus), `/` (divide), and `^` (power).

These are the aspirational goals:

- use/invent operations which don't care about order as much as
  possible
- don't ask the user for the _precision_
- Output must be usable as input in at least one form, if several
  outputs are possible
- Have many special functions (e.g. `exp`, or `tanh`).
- When the program is done, it prints the entire stack, no print
  commands
- Avoid stack empty errors

The following sections explain how I address these goals, if I found a way.

# Precision

This program is not _arbitrary precision_, increasing the precision
may become a goal later on.

But, perhaps what people need from a quick command line tool, is a
calculation that is integer-like, when sufficient, and not integer
when needed.


## Rational Numbers $\mathbb{Q}$

This program treats all values as _rational numbers_ (at least
approximately), using this special notation: `1;2;3` (semi-colon separated list). It is
interpreted as:

$$
1\frac{2}{3} := 1 + \frac{2}{3}
$$

All of these components are retained in the internal representation:

```sh
./rpnc '1;1;3 1;2;3 +'
```
```sh
(3)	# 3
```

A leading `-` will negate the entire number:

```sh
./rpnc '-1;1;8 1;1;8'
```
```sh
(-1-1/8)	# -1.125
(1+3/8)	# 1.375
```

The fractional part can have signs as well (please avoid using it).
Since a leading `-` negates the entire number, you may find the result confusing:

```sh
./rpnc '-1;-1;8'  # same as -(1-1/8)  ...bad
```
```sh
(-1+1/8)	# -0.875
```
note that the leading minus in `-(1-1/8)` is outside the parentheses.
A scale can be added with one additional `;` component:

```sh
./rpnc '-1;1;8;3'  # same as -(1+1/8)×10³
```
outputs:
```sh
(-1-1/8)*pow(10,3)	# -1125
```

## Output

The output of `rpnc` looks a bit peculiar perhaps (it's the default
output style). It prints the rational representation in parentheses,
this way it can be copied and pasted into some other formula, or
expression. Aditionally, it also prints a floating point
representation of the result, after a shell comment mark.

Of course, most numbers aren't nice values, close to an integer:

```sh
./rpnc '2 sin' # sin(2)
```
has output that includes a correction $f$:
```sh
(909 + 185/622 -8.271e-07)*pow(10,-3)	# 0.909297
```

From this output, we know that the result is approximately
$909\times10^{-3}$ (but written in C syntax). We also get a fractional
term $185/622$, which is smaller than $1$ (and ultimately smaller than
$10^{-3}$ after multiplication with the _scale_ factor).

Because this _is_ an approximation, we also get a double-precision
floating point correction term `-8.271e-7`, which indicates how well
the rational fraction term has worked out.

Finally, we also see the boring result `0.909297` (rounded).

When reading numbers, the program does the same approximation (when
necessary):

```bash
./rpnc '1 1.e-3 1.e4 3.141592653589' | column -t -s $'\t'
```
which contains no operators or functions, so it just prints what it read:
```sh
(1)                      # 1
(1)*pow(10,-3)           # 0.001
(10)*pow(10,3)           # 10000
(3 + 16/113 -2.668e-07)  # 3.14159
```

To summarize, each number has 5 meaningful components:

$$
x := \left(A+\frac{n}{d}+f\right)\times 10^{E}
$$

where $f$ is an optional _floating point_ (`double`) correction term,
and $E$ a _scale_ (exponent to the base 10). When converting to this
format, we try to create $E$ in multiples of three. But users may
enter any combination of `A;n;d;E` (e.g. `0;355;113` as an approximation of $\pi$).

# Stack Empty Problem

The last value on the stack, cannot be removed. Thus,

```sh
./rpnc '2 * * *' # pops 2 and then 2 again, multiplies them, pushes onto stack, etc.
```
will print:
```sh
(256)   # 256
```
that is: `((2*2)*(2*2))*((2*2)*(2*2)) = 2^8`
or in C code:

```C
x=2;  // 2
x*=x; // 4
x*=x; // 16
x*=x; // 256
```

The initial top of the stack is the constant `0`. So, this works:

```sh
./rpnc 'cos'      # should be 1
```
returns the cosine of 0:
```sh
(1)     # 1
```

Pushing a value onto the stack replaces the initial `0` as the top value.

# Operators

This is an incomplete list of operators, more will be added when
implemented. In every case the result is pushed onto the stack.

`+`
: pops two values from the stack and adds them (as rationals)

`-`
: pops one value from the stack and negates it

`*`
: pops two values from the stack and multiplies them

`@`
: inverse, pops a value, then pushes the inverse of that number to the stack

An example of `@` inversion:

```sh
./rpnc '3 @'
```
```sh
(0 +1/3)  # 0.333333
```

And also for the purposes of division:

```sh
./rpnc '3 @ 3 *'
```
```sh
(1)	# 1
```

## Binary Operators

These operators break the rules about order (_order should not matter_).

`^`
: pops an exponent `b`, then pops a base `a`, returns `pow(a,b)`

`**`
: pops an exponent `b`, then pops a base `a`, returns the integer power of `a*…*a` or $\prod_{i=1}^b a$. This operator uses only the whole integer component of any number

`/`
: division, short-hand for `@` and `*` together: `${x} ${y} @ *` is `x y /`  (x/y in infix notation)

`\`
: division in reverse order: `${x} ${y} \` is the same as `${y} ${x} /`; short-hand for `${x} @ ${y} *`

```sh
./rpnc '2 3.1 ^ 2 3.1 **' | column -t -s $'\t'
```
```
(8 +565/984 +7.084e-07)  # 8.57419
(8)                      # 8
```

Division:

```sh
./rpnc '2 3 / 2 3 \'
```
```
(0 +2/3)	# 0.666667
(1 +1/2)	# 1.5
```


### Powers

A cool way to calculate powers without remembering an order (is the first number popped a base or an exponent?) is to use `log` and `exp`:

$$
x^y = \exp(\log(x^y)) = \exp(y \log(x))\,,
$$

Therefore:

```sh
./rpnc '2 log 3 * exp' # should be 8=2^3
# (8)	# 8
```

Or, equivalently:

```sh
./rpnc '3 2 log * exp'
# (8)	# 8
```

This, to me, is a much nicer order, with the numbers grouped apart
from the operations; I also finc the order of operations easy to
remeber. In this pattern, you can also deduce from the operations,
which number represents what: it is clear to me that the `log` is
applied to the base. This solves the order problem form exponents,
in my opinion.

On the other hand, these are both much longer than `'2 3 ^'`, or `'2 3 **'`.

# Functions

All functions with one argument in `math.h` are available (where the
types are always `double`), except `pow10` (but `exp10` is, it does
the same thing).

These are not made available (yet? who knows):

```c
double      atan2(double, double);
double      copysign(double, double);
double      fdim(double, double);
double      fma(double, double, double);
double      fmax(double, double);
double      fmin(double, double);
double      fmod(double, double);
double      frexp(double, int *);
double      hypot(double, double);
double      ldexp(double, int);
double      modf(double, double *);
double      nextafter(double, double);
double      nexttoward(double, long double);
double      pow(double, double);
double      remainder(double, double);
double      remquo(double, double, int *);
double      scalbln(double, long);
double      scalbn(double, int);
double      drem(double, double);
double      scalb(double, double);
```

# Constants

Mathematical constants from `math.h` such as `M_PI` can be used as well:

```C
M_E
M_LOG2E
M_LOG10E
M_LN2
M_LN10
M_PI
M_PI_2
M_PI_4
M_1_PI
M_2_PI
M_2_SQRTPI
M_SQRT2
M_SQRT1_2
```
