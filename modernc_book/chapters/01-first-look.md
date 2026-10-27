# 1. A First Look at C

A C program describes operations on values and storage. It can run with very little runtime support, but it gives the programmer correspondingly little protection from mistakes. We begin with programs small enough to understand without an elaborate vocabulary.

## 1.1 A complete program

```c
#include <stdio.h>

int main(void)
{
    puts("hello, world");
    return 0;
}
```

The `#include` directive makes declarations from a header available. `main` is the function from which a hosted program begins. The braces contain a block. `puts` writes a string followed by a newline. A return value of zero reports success to the execution environment.

Compile with `cc -std=c23 -Wall -Wextra -Wpedantic hello.c -o hello` and run `./hello`. The `cc` command may select a compiler too old for the C23 flag; a modern GCC or Clang may be named explicitly.

The program has a declaration, an expression statement, and a return statement. There is no implicit declaration of `puts`; its prototype comes from `stdio.h`. Unlike historical C, modern C requires function declarations that accurately describe the parameters.

## 1.2 Variables and arithmetic

```c
#include <stdio.h>

int main(void)
{
    for (int celsius = 0; celsius <= 100; celsius += 20) {
        double fahrenheit = 9.0 * celsius / 5.0 + 32.0;
        printf("%3d %6.1f\n", celsius, fahrenheit);
    }
}
```

The loop creates, tests, and increments `celsius`. In each iteration, `fahrenheit` exists within the loop body. A value like `9.0` is floating point; integer division such as `9 / 5` would discard the fractional part. The `printf` format is an agreement between the programmer and the arguments: `%d` expects an `int`; `%f` expects a floating-point argument (a `double` after promotion).

Types are not interchangeable merely because two values can both be printed as numbers. The compile-time type controls arithmetic, conversions, available operations, and storage requirements.

## 1.3 Characters and the input stream

```c
#include <stdio.h>

int main(void)
{
    int ch;
    unsigned long lines = 0;

    while ((ch = getchar()) != EOF) {
        if (ch == '\n')
            ++lines;
    }
    printf("%lu\n", lines);
}
```

`getchar` returns an `int`, not a `char`: it must represent every `unsigned char` value *and* the negative sentinel `EOF`. The parentheses in the loop make the assignment happen before the inequality test. The input is read until end-of-file or an input error; a production program checks `ferror(stdin)` to distinguish the two.

## 1.4 An array and a simple histogram

Suppose a program counts decimal digits in its input:

```c
#include <stdio.h>

int main(void)
{
    unsigned long count[10] = {0};
    int ch;
    while ((ch = getchar()) != EOF) {
        if (ch >= '0' && ch <= '9')
            ++count[ch - '0'];
    }
    for (int i = 0; i < 10; ++i)
        printf("%d: %lu\n", i, count[i]);
    return ferror(stdin) ? 1 : 0;
}
```

A C array is a contiguous sequence of elements. Indexes begin with zero. The array above has exactly ten positions; a value of ten is already out of bounds. Unlike many modern languages, C does not generally check array bounds at runtime.

## 1.5 Functions as named operations

```c
static int maximum(int a, int b)
{
    return a > b ? a : b;
}
```

A function packages an operation behind a name and an interface. `static` at file scope gives this function internal linkage: other source files cannot refer to it by this name. The conditional operator selects one of two expressions.

**Exercises.** 1.1 Change the temperature program to print from 100 down to 0. 1.2 Count tabs and spaces separately. 1.3 Count lines longer than a fixed limit without storing whole lines. 1.4 Explain what happens if the histogram test accidentally admits the character `':'`.


## 1.6 Counting words: states rather than lines

Counting lines required remembering only a number. Counting words requires remembering whether the previous character belonged to a word. A *state* is just the minimum information needed to interpret the next input.

```c
#include <stdio.h>
#include <stdbool.h>

int main(void)
{
    unsigned long words = 0;
    bool inside = false;
    int ch;

    while ((ch = getchar()) != EOF) {
        bool separator = ch == ' ' || ch == '\n' || ch == '\t';
        if (separator) {
            inside = false;
        } else if (!inside) {
            inside = true;
            ++words;
        }
    }
    if (ferror(stdin)) {
        fputs("input error\n", stderr);
        return 1;
    }
    printf("%lu\n", words);
}
```

The loop recognizes a transition from *outside* to *inside* a word. Notice that the definition of a word depends on the definition of separator. Here only three ASCII whitespace characters count. Changing to `isspace` requires `<ctype.h>` and care to pass only an `unsigned char` value or `EOF`.

A state-machine description is valuable beyond text processing. An embedded serial receiver similarly recognizes transitions among waiting, payload, and checksum states, and can process one byte at a time without allocating a complete input stream.

### Trace a program

For input `" one  two\n"`, write down `inside` and `words` after each character. Two adjacent separators must not count an empty word. End-of-file also must not count an extra word.

## 1.7 When the simplest program is not enough

The programs in this chapter fit in small files, but each has an implicit contract. A histogram counter might overflow on a sufficiently large input. An I/O operation may fail before reaching EOF. An unexpected input byte may not be meaningful under an intended encoding. A reliable C program deliberately records which of those conditions it will handle.

Three habits will recur throughout the book:

1. Carry buffer sizes explicitly.
2. Treat resource and I/O failures as ordinary control paths.
3. Use the type system and compiler diagnostics to expose assumptions rather than conceal them.

### Further exercises

**1.5** Modify the word counter to report separately how many words began with an ASCII digit. **1.6** Implement the line counter using a function `count_lines(FILE *stream)` that reports failure separately from a legitimate zero count. **1.7** Determine when `unsigned long` is wide enough for your intended input volume; document a policy for counter overflow. **1.8** Write a program that collapses runs of spaces to one space while preserving newlines.


## 1.8 Reading a program from the outside inward

A complete program has two interfaces. The first is its *source interface*: its declarations, definitions, and required headers. The second is its *execution interface*: how input arrives, what output means, and which exit status reports success. Keeping both visible makes the examples easier to reason about.

In a hosted C environment, execution starts at `main`. Either `int main(void)` or the conventional argument-bearing form `int main(int argc, char *argv[])` may be used. Parameters named `argc` and `argv` hold the command-line argument count and vector. The argument strings are accessible through pointers; they are not fixed-sized arrays embedded in the function. A freestanding embedded implementation may use a different entry point and need not provide all hosted I/O facilities.

An output statement has a return value for a reason. `puts` returns `EOF` on output error. In a quick hello-world demonstration we ignore that possibility; in a program that promises reliable output, we check it:

```c
#include <stdio.h>

int main(void)
{
    if (puts("hello, world") == EOF)
        return 1;
    return fflush(stdout) == EOF ? 1 : 0;
}
```

This detects errors reported by these stream operations. It does not guarantee that bytes have reached nonvolatile storage.

## 1.9 Trace the loop, not the syntax alone

When studying an unfamiliar loop, create a three-column trace: the input just consumed, the current state, and the counters. For the word counter earlier in this chapter, consider the bytes `a`, space, space, `b`, newline. The transition on the first `a` increases the count; the two adjacent spaces do not; the `b` increases it once more. Writing this trace explains the algorithm better than memorizing the loop.

Now change the input to the empty stream. The loop body never executes, and the count remains zero. Change it to a stream containing only separators: the state never enters a word. Change it to a final word with no newline: the last word is still counted, because the increment happens at word entry rather than word exit. Such tests expose the program's true specification.

A byte read from `getchar` is always represented by an `int` return type. This permits every possible `unsigned char` value as well as `EOF`. If the value is assigned first to a plain `char`, some valid byte values may be confused with the sentinel on implementations with signed `char`.

## 1.10 Programs as small transformations

A useful first decomposition separates three questions: What is the input domain? What state must be retained? What result must be produced? The digit histogram's input domain is a byte stream; its state is ten counters; its output is a display of those counters. Its logic makes no assumptions about keeping the whole input in memory.

The same shape reappears in firmware: read a sample or message, update bounded state, and report or act on the result. The difference is not primarily a new language construct but the stronger need to bound time, memory, and error handling. Keeping simple algorithms streaming from the outset makes the later examples easier to adapt.

**Further problems:** Describe precisely what the word counter considers a separator. Add overflow checks to the histogram counters using `ULONG_MAX`. Change the line counter to distinguish a newline count from a count of human-readable lines that includes a final unterminated line.


## 1.11 The execution path from source file to program

The small examples hide several steps that are worth separating. A preprocessor handles directives such as \`#include\`; a compiler translates a translation unit into object code; a linker combines objects and libraries into an executable. On a hosted implementation, the runtime arranges for \`main\` to execute. None of those stages guarantees that an input file exists or that a terminal will accept output.

Consider a program split between \`count.c\` and \`main.c\`. The shared function declaration belongs in a header. If \`main.c\` is compiled without seeing the declaration, current C does not permit inventing an implicit function declaration. That old convenience was dangerous: the compiler could assume an incompatible calling convention or return type. A correct C program states its interfaces.

A useful first experiment is to compile with \`-c\` and inspect the resulting object files without linking them. Then link those objects into an executable. Deliberately omit the object containing one function definition: the unresolved external symbol illustrates the boundary between compilation and linkage.

## 1.12 A state machine for byte-oriented input

The line and word counters suggest a general pattern. Each input byte causes a state transition, and the program emits results based on the resulting state. The same pattern recognizes a serial protocol whose messages begin with a marker, contain a length, and end with a checksum.

For a toy ASCII protocol, suppose lines are terminated by \`'\n'\` and may contain at most 16 ordinary bytes. A parser could maintain a buffer, a length, and one status flag. Every ordinary byte is accepted only if capacity remains. A newline terminates a record, and an overlong line is rejected rather than truncated into something that looks valid.

The key engineering step is to write the invariants before the loop: \`0 <= length <= capacity\`; no bytes are written beyond \`buffer[length-1]\`; failure has a documented outcome; and after rejecting a long line the parser must either drain to the next newline or stop processing. That final choice is a *resynchronization policy*, not a syntax issue.

Why not read the entire input file first? Streaming input requires bounded storage regardless of total input size, and it can process an indefinitely long stream. Its tradeoff is that the algorithm must remember enough state to handle records split across reads.

## 1.13 Three kinds of correctness

A counting program can fail in at least three distinct ways.

First, the algorithm can be logically wrong: perhaps two consecutive spaces are counted as a word. Second, it can invoke an operation the language does not define, such as accessing an array at index ten when its valid indexes are zero through nine. Third, it can encounter an ordinary external error: the input stream may fail, or the output may not be writable.

A test for one class does not necessarily reveal the others. Trace the state machine to reason about the first. Make bounds explicit and use compiler diagnostics and sanitizers to help investigate the second. Check I/O results to address the third.

## 1.14 Choosing types from contracts

For \`getchar\`, \`int\` is the correct return type because the function must represent \`EOF\` separately from ordinary character values. For an array length, \`size_t\` is the natural type because the standard library uses it to represent object sizes. For a fixed-width protocol field, use a suitable type such as \`uint16_t\` if the target provides it.

The preferred type depends on meaning rather than an aesthetic preference for unsigned or wide integers. A counter of unbounded input events can still overflow; using \`unsigned long\` does not make the data unlimited. Choose a maximum supported volume and decide what to do if it is exceeded.

### Guided exercises

**1.9** Write a table of the input states for the word counter: separator, word byte, and EOF. For each, state which variables change. **1.10** Propose a record parser with a 16-byte buffer and show the sequence of state changes for a 17-byte record. **1.11** Modify the histogram to report an error before incrementing a counter already at \`ULONG_MAX\`. **1.12** Build a two-translation-unit program and explain exactly which step fails when a required object file is omitted.
