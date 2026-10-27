# 11. An Extended Example: A Word Index

A word counter is a small program that demonstrates streaming input, fixed-capacity state, tokenization, searching, and error reporting. We deliberately avoid dynamic allocation so the memory bound is explicit.

## 11.1 Requirements

Read standard input one byte at a time. Treat ASCII letters and digits as part of words, fold uppercase ASCII to lowercase, store up to 1024 distinct words of at most 31 bytes, and print each word with its occurrence count. Reject excess distinct words rather than corrupting the table.

This is an ASCII-oriented utility, not a general Unicode word segmenter. Its fixed-size table and linear search make worst-case costs visible.

## 11.2 Implementation

```c
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

enum { MAX_WORDS = 1024, MAX_LEN = 31 };

struct entry {
    char word[MAX_LEN + 1];
    unsigned long count;
};

static struct entry entries[MAX_WORDS];
static size_t used;

static bool letter_or_digit(int c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9');
}

static char lower_ascii(int c)
{
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return (char)c;
}

static bool add_word(const char *word)
{
    for (size_t i = 0; i < used; ++i) {
        if (strcmp(entries[i].word, word) == 0) {
            if (entries[i].count == ~0UL) return false;
            ++entries[i].count;
            return true;
        }
    }
    if (used == MAX_WORDS) return false;
    strcpy(entries[used].word, word); /* bounded by tokenizer */
    entries[used].count = 1;
    ++used;
    return true;
}

int main(void)
{
    char word[MAX_LEN + 1];
    size_t len = 0;
    int ch;
    bool too_long = false;
    while ((ch = getchar()) != EOF) {
        if (letter_or_digit(ch)) {
            if (len < MAX_LEN)
                word[len++] = lower_ascii(ch);
            else
                too_long = true;
        } else if (len != 0 || too_long) {
            if (too_long) {
                fputs("word exceeds limit\n", stderr);
                return 1;
            }
            word[len] = '\0';
            if (!add_word(word)) {
                fputs("word table full or counter overflow\n", stderr);
                return 1;
            }
            len = 0;
        }
    }
    if (ferror(stdin)) {
        fputs("input error\n", stderr);
        return 1;
    }
    if (too_long) {
        fputs("word exceeds limit\n", stderr);
        return 1;
    }
    if (len != 0) {
        word[len] = '\0';
        if (!add_word(word)) return 1;
    }
    for (size_t i = 0; i < used; ++i)
        printf("%-31s %lu\n", entries[i].word, entries[i].count);
    return ferror(stdout) ? 1 : 0;
}
```

The total table storage is fixed. The program checks every insertion, rejects too-long words, and refuses counter wraparound. The final output is in order of first appearance, not lexicographic order.

## 11.3 Complexity and extensions

Each word may be compared against up to 1024 existing entries, so the lookup time is linear in the number of distinct words. A hash table can improve average lookup, but then needs collision handling, a load-factor policy, and carefully bounded worst-case behavior. Sorting entries before printing provides alphabetical output without changing the tokenizer.

**Exercises.** 11.1 Sort the final table alphabetically. 11.2 Replace linear lookup with open addressing. 11.3 Count overflowed word lengths rather than aborting. 11.4 Implement Unicode-aware tokenization using a deliberately selected library, documenting its normalization rules.

## 11.4 From linear search to a sorted index

The first implementation searches entries in order of first appearance. If there are *u* distinct words, one lookup may require *u* comparisons. We can instead keep the table sorted and use binary search to locate an existing word or the place to insert a new one.

For a sorted array with bounds `lo` and `hi`, the active range is half-open: `[lo, hi)`. At each iteration, compute `mid = lo + (hi - lo) / 2`; this avoids the potential overflow of `(lo + hi) / 2`. Compare the word at `mid` with the query. If it sorts earlier, the search continues after `mid`; otherwise it continues at or before `mid`. The range narrows on every iteration.

When `lo == hi`, the search ends. The same position is the insertion point for a new word. Shifting entries one position to the right preserves sort order. Lookup costs logarithmic comparisons, but insertion still takes linear time in the number of entries. This is a useful reminder that improving one operation does not automatically improve the entire algorithm.

## 11.5 A table with explicit invariants

The sorted-array representation has three important invariants: (1) `used` never exceeds `MAX_WORDS`, (2) entries in `[0, used)` are sorted lexicographically, and (3) every stored word contains at most `MAX_LEN` ASCII bytes followed by a zero terminator. Before incrementing a count, test against `ULONG_MAX`. Before inserting, confirm unused capacity. These checks protect both memory bounds and numerical meaning.

The companion complete program, `examples/word_index.c`, demonstrates sorted indexing, ASCII normalization, fixed storage, and failure reporting. It deliberately treats words exceeding the maximum length as an error instead of silently truncating them into potentially ambiguous keys.

## 11.6 Complexity, measurement, and design

For a text with *n* input bytes and *u* unique words, lexical scanning is linear in *n*. Binary lookup is *O(log u)* string comparisons per completed word; moving records for insertion is *O(u)* in the worst case. Since each string is at most 31 bytes, comparison work has a fixed bound. The static table capacity also provides a fixed memory bound.

A hash table could give expected near-constant lookup, with tradeoffs for collision policy, load factor, hashing cost, and predictability. A balanced tree could avoid moving array entries but would require more bookkeeping. Neither is categorically superior: requirements determine the data structure.

## 11.7 Additional exercises

**11.5** Prove that the binary-search interval strictly shrinks on every iteration. **11.6** Add a configurable maximum word count, without introducing dynamic allocation. **11.7** Implement a frequency-first ordering at print time, preserving alphabetical order on ties. **11.8** Test the empty input, a final word without newline, repeated separators, a full table, and an overlong word. **11.9** Benchmark insertion workloads with vocabulary ordered forward, reverse, and randomly.


## 11.8 Designing the input grammar

The index accepts ASCII letters and decimal digits and treats everything else as a separator. That simple grammar has two important consequences: punctuation splits words, and any byte outside the specified ASCII set is not recognized as part of a word. A UTF-8 text file may therefore produce surprising tokens unless the program is explicitly extended to decode Unicode.

The rule for overlong words is equally important. Truncating a word after 31 bytes is not merely losing display information; two different long words can collapse into the same key. Rejecting overlong tokens prevents an incorrect count from looking authoritative.

The program also chooses to count only complete words and to accept a final word that has no trailing separator. That behavior is tested separately because streaming programs often get end-of-file boundaries wrong.

## 11.9 Proving the sorted insertion

The maintained invariant is that `entries[0]` through `entries[used-1]` are sorted and distinct. Binary search finds the first position whose word is not less than the input. There are two cases:

* If an entry at that position compares equal, increment its count after checking for overflow.
* Otherwise, if capacity permits, move the suffix one element right, copy the new word into the gap, set its count to one, and increment `used`.

The move proceeds from the right edge toward the insertion point. Moving in the opposite direction with ordinary assignments could overwrite entries before they were copied. The special bound `used < MAX_WORDS` ensures one free slot exists before any shift.

The search interval `[lo, hi)` shrinks because each iteration either increases `lo` beyond the midpoint or lowers `hi` to the midpoint. Since its endpoints are integers in a finite range, the loop terminates. The midpoint calculation `lo + (hi-lo)/2` avoids adding two potentially large bounds.

## 11.10 Complexity with meaningful constants

Let `w` be the number of word occurrences, `u` the number of distinct words, and `L` the maximum key length. Tokenization scans input bytes once. A binary search performs at most logarithmically many comparisons in `u`, and each comparison can inspect up to `L` bytes. A new entry may require copying `O(u)` records. Thus workloads dominated by repeated words may be substantially cheaper than workloads dominated by inserting new distinct words.

For a fixed table of 1024 entries and 31-byte keys, the resource ceiling is explicit. That bounded design may matter more in firmware than the asymptotic advantage of a dynamic hash table. A hash implementation is a useful improvement exercise, not a universally better choice.

## 11.11 Test cases that reveal design errors

Use independent expected outputs for: no input; one word; a final word without newline; repeated separators; uppercase and lowercase variants; words exactly at the maximum length; a word one byte too long; the maximum distinct-word count; and an attempted extra distinct word.

A property test can also compare the sum of all printed counts with the number of successfully recognized word occurrences. This is not a complete proof, since two errors could cancel, but it detects many accidental drops and duplicates.

## 11.12 Extending the implementation responsibly

An open-addressed hash table needs a defined hash function, collision resolution, load limit, and behavior when no free slot exists. Returning failure is better than silently overwriting an unrelated entry. If the application must preserve sorted output, sort the final records or maintain a separate ordering; hashing by itself does not provide alphabetical order.

A Unicode-aware version is a different product. It must define normalization and word-boundary rules, decide how case folding works, and bound its memory for multibyte text. Simply applying `tolower` to each UTF-8 byte is not enough.

**Further problems:** Provide a proof that the insertion loop cannot access index `MAX_WORDS`. Write a test generating 1024 distinct terms. Compare sorted-array and hash-table lookup under high repetition. Design a stable frequency ordering for ties.
