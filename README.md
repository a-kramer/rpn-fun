# Reverse Polish Notation

I always found reverse polish notation quite fun, it's easy to parse,
and one does not need to count parentheses.

There is program called `dc`, which i like, but I never use its
advanced features (e.g. registers) and I am always a bit disappointed
that it doesn't have the functions from the math library.

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

... mysterious. And for some reason, we must fic this by providing a precision, which wasn't necessary for the `+`:

```sh
dc -e '4 k 1.2 2.3 / p'
# .5217
```
