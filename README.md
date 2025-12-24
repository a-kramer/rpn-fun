# Reverse Polish Notation

I always found reverse polish notation quite fun, it's easy to parse,
and one does not need to count parentheses.

There is program called `dc`, which i like a lot. I never use its
advanced features (e.g. registers) and I am always a bit disappointed
that it doesn't have the functions from the math library (`math.h`).

In addition, there are certain inconvenciences:

```sh
dc -e '2 3 +'
```

has no result, you _have_ to write `p` for print. And for some reason this is the default behavior:

```sh
dc -e '1.2 2.3 + p'
# 3.5
dc -e '1.2 2.3 / p'
# 0
```

... mysterious.

For some reason, we must fix this by providing a _precision_ (with _k_ for _precision setting_ ), which wasn't necessary for the `+`:

```sh
dc -e '4 k 1.2 2.3 / p'
# .5217
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
# -5                   ... not _5
```

And thus:

```bash
x=$(dc -e '3 _8 + p')
dc -e "${x} 3 + p"
# dc: stack empty
# 8
```

You have to do this:

```bash
dc -e "${x/-/_} 3 + p"
# -2
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

This program will treat numbers as _rational numbers_, using this special notation: `1;2;3`, which is interpreted as:

$$
1\frac{2}{3} := 1 + \frac{2}{3}
$$

This means that:

```sh
./rpnc '1;1;3 1;2;3 +'
# (3)	# 3
```

A leading `-` will negate the entire number:

```sh
./rpnc '-1;1;8 1;1;8'
# (-1-1/8)	# -1.125
# (1+3/8)	# 1.375
```

The fractional part can have signs as well. But, since a leading `-` negates the entire number, you may find the result confusing:

```sh
./rpnc '-1;-1;8'  # same as -(1-1/8)
# (-1+1/8)	# -0.875
```
note that one unary minus is outside the parentheses.
A scale can be added with one additional `;` component:

```sh
./rpnc '-1;1;8;3'  # same as -(1+1/8)×10³
```
outputs:
```
(-1-1/8)*pow(10,3)	# -1125
```

## Output

The output of `rpnc` looks a bit weird perhaps (it's the default
output style). It prints the rational representation in parentheses,
this way it can be copied and pasted into some other formula, or
expression. Aditionally, it also prints a floating point
representation of the result, after a shell comment mark.

But, of course, most numbers aren't nice values close to an integer:

```sh
./rpnc '2 sin' # sin(2)
```
has the output:
```sh
(909 + 185/622 -8.271e-07)*pow(10,-3)	# 0.909297
```

From this output, you know that the result is approximately
$909\times10^{-3}$ (but written in C syntax). You also get a
fractional correction term, which is smaller than $10^{-3}$.
But, because this is an approximation, you also get a double-precision floating point correction term `-8.271e-7`, which indicates how well the rational fraction term has worked.
Finally, you also see the boring result `0.909297`, rounded.

When reading in numbers, the program does the same approximation when
necessary:

```bash
./rpnc '1 1.e-3 1.e4 3.141592653589' | column -t -s $'\t'
```
which contains no operators or functions, so it just prints what it read:
```
(1)                      # 1
(1)*pow(10,-3)           # 0.001
(10)*pow(10,3)           # 10000
(3 + 16/113 -2.668e-07)  # 3.14159
```

So, to summarize, each number has 4 meaningful components:

$$
x := \left(A+\frac{n}{d}+f\right)\times 10^{E}
$$

where $f$ is an optional floating point (`double`) correction term (if
needed), and $E$ a _scale_ (exponent to the base 10). When converting
to this format, we try to create E in multiples of three.

# Stack Empty Problem

The last value on the stack, cannot be removed. Thus,

```sh
./rpnc '2 * * *' # pops 2 and then 2 again, multiplies them, pushes onto stack
```
will print:
```
(256)   # 256
```
that is: `((2*2)*(2*2))*((2*2)*(2*2)) = 2^8`
or in C code:

```c
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
```
(1)     # 1
```

# Operators

These is an incomplete list of operators, more will be added when
implemented. In every case the result is pushed onto the stack.

`+`
: pops two values from the stack and adds them (as rationals)

`-`
: pops one value from the stack and negates it

`*`
: pops two values from the stack and multiplies them

`^`
: this pops two values, breaking the rules of _order should not matter_ (no solution found so far); pops an exponent `b`, then pops a base `a`, returns `pow(a,b)`

## Powers

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

But, these are both much longer than `'2 3 ^'`.

# Functions

In the end, most of the functions in `math.h`, currently, a small subset. WIP
