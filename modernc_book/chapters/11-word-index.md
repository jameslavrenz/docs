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

## 11.14 A search algorithm with a proof obligation

Binary search is brief enough to fit on a few lines, but it should be understood through its invariant. At the start of each iteration, the insertion point lies in the half-open interval `[lo, hi)`. Every entry strictly before `lo` compares less than the requested word. Every entry at or after `hi` compares greater than or equal to it.

Set `mid = lo + (hi - lo) / 2`. If the middle entry compares less than the request, the insertion position must follow `mid`, so set `lo = mid + 1`. Otherwise, it is at or before `mid`, so set `hi = mid`. When `lo == hi`, exactly one insertion position remains.

The implementation can then determine whether the requested word already exists at that position. If so, it increments a counter only after verifying that increment is representable. If not, it checks table capacity before shifting existing records.

The proof is useful when changing the data structure. One off-by-one mistake can produce a program that appears correct for a small vocabulary but corrupts storage when inserting at the beginning or end.

## 11.15 The cost of sorted insertion

Binary search reduces the number of comparisons needed to locate a word, but maintaining a sorted array costs more when inserting new records. If there are already `u` distinct words and the new one belongs at the front, the program shifts `u` entries. The worst-case cost of inserting `u` distinct keys is therefore quadratic in `u`, even though each lookup performs only logarithmically many comparisons.

That is not automatically unacceptable. With a hard limit of 1,024 short keys, the worst-case memory usage is small and predictable. A microcontroller may value a fixed allocation and simple failure policy more than average-case insertion speed. For a large text corpus with millions of terms, a hash table or tree is likely more appropriate.

Measure the actual workload: repeated tokens benefit from quick lookup and do not shift records, while a sorted stream of unique tokens repeatedly inserts near one end. Big-O notation tells us the growth trend, but a real resource bound must still account for maximum keys and fixed string lengths.

## 11.16 Error handling belongs inside the index

Rejecting a long token rather than truncating it prevents two different words from becoming the same key. Rejecting an eleventh-hundredth distinct key rather than writing past the table prevents memory corruption. Rejecting counter overflow prevents the program from silently reporting a smaller count than the truth.

After any rejection, decide whether the program should stop, skip the offending token and continue, or expose a detailed status. The teaching implementation stops at the first error. A long-running service might recover, but only with a documented strategy that avoids partially committing a word.

The internal operation `add_word` returns success or failure; a larger program could use an error enumeration to distinguish table-full from count-overflow. That would improve diagnostics without changing the sorted-array algorithm.

## 11.17 Independently test the expected output

A decoder and encoder written with the same wrong byte order can agree with each other. Likewise, a word index and an expected-result generator copied from the same algorithm can share the same defect. For a handful of text samples, write the expected alphabetical output manually.

Then add generated larger cases with invariants: total counts must equal accepted word occurrences, entries must be strictly sorted and unique, and no count may be zero. For a stable random test, record the seed so failures can be reproduced.

### Exercises

**11.10** Trace the search interval for inserting a word at the beginning, middle, and end of a five-element table. **11.11** Estimate the maximum number of shifts for inserting 1,024 distinct words in reverse order. **11.12** Design an error enumeration that distinguishes a long word, full table, input failure, and counter overflow. **11.13** Write an independent expected output for a file containing repeated words with differing capitalization.


## 11.18 Trace a complete input

The word-index program is small enough to reason about from its first input byte to its final output. Consider the stream:

```text
Pear apple pear BANANA apple
```

The tokenizer recognizes letters and decimal digits and treats the intervening space as a separator. Under its intended ASCII-compatible execution encoding, it converts the uppercase letters to lowercase before updating the index.

The resulting logical sequence of words is:

```text
pear, apple, pear, banana, apple
```

The index maintains entries in sorted order, so its final output is:

```text
apple 2
banana 1
pear 2
```

This output is independently determined by the input, rather than constructed by running the index algorithm again. That distinction is useful in testing: the expected result must not rely on the implementation being correct.

## 11.19 A trace of sorted insertion

Start with an empty table and insert `pear`: the search interval is `[0,0)`, so the insertion position is zero.

Insert `apple`: the search finds position zero because `apple` sorts before `pear`. The program shifts the existing record right and inserts `apple` at the first slot.

Insert `pear` again: the search locates the existing entry at position one and increments its count rather than inserting a duplicate.

Insert `banana`: the new word belongs between `apple` and `pear`, so the program moves `pear` right by one slot. The table now contains three distinct words.

Finally, inserting `apple` again increments the first entry's count. No shifting occurs for repeated words.

At each step, the invariant is that exactly the first `used` entries contain initialized words, those words are strictly increasing according to `strcmp`, and all counts are positive.

The implementation can therefore print the table directly without a final sorting pass.

## 11.20 Why the midpoint is written carefully

The binary search uses the half-open interval `[lo, hi)`. Its midpoint is:

```c
size_t mid = lo + (hi - lo) / 2;
```

Before this expression, the algorithm maintains `lo <= hi <= used`. Therefore `hi - lo` is defined without unsigned underflow, and adding at most half that difference to `lo` yields a result between `lo` and `hi`.

The superficially simpler expression `(lo + hi) / 2` may overflow a sufficiently narrow unsigned type even when both endpoints are valid indexes. In this particular fixed-capacity example, `used` never exceeds 1,024 and such overflow is not realistic on its supported hosts. The subtraction-based midpoint is nevertheless a clear reusable pattern.

When the middle entry is lexically less than the requested word, the insertion position must follow it; the search assigns `lo = mid + 1`. Otherwise it assigns `hi = mid`. The interval shrinks until the endpoints meet.

The final position identifies either an existing word or the location where a new word should be inserted.

## 11.21 Moving entries without losing one

Suppose there are `used` initialized entries and a new word belongs at index `lo`. Before insertion, confirm that `used < MAX_WORDS`. The program moves entries from right to left:

```c
for (size_t j = used; j > lo; --j)
    entries[j] = entries[j - 1];
```

On the first iteration, the destination is `entries[used]`, which is available because the capacity check established that one extra slot exists. The source is `entries[used - 1]`, the final initialized entry.

Moving right to left is necessary. Moving left to right would overwrite an entry before its old contents had been copied to the next slot.

When `lo == used`, the loop executes zero times and the word is appended. When `lo == 0`, every existing entry shifts one position. This creates a linear worst-case cost for insertion.

Each `struct entry` includes its word bytes directly, so structure assignment copies the complete inline representation. There is no owning pointer that would require a deep-copy operation. This is a deliberate simplification of the data structure.

## 11.22 Fixed resource bounds

The example defines:

```c
enum { MAX_WORDS = 1024, MAX_LEN = 31 };
```

Each entry stores a word buffer of 32 `char` elements and an `unsigned long` count. Thus the table uses the storage required by 1,024 such structures, including any implementation-selected padding or alignment. Its exact RAM footprint is `sizeof entries` on the selected target.

The tokenizer uses a single 32-element buffer plus a length and an overlong-word flag. No storage grows with the input file size.

The application therefore has predictable memory consumption, but input processing time depends on the number of bytes and words. A new distinct word near the beginning may shift many entries; repeated words require a binary search and count increment. Once 1,024 distinct words exist, a new distinct word is rejected.

A production application might decide to evict old entries, use a fixed-size hash table, or retain approximate counts. Those choices change the observable output contract. The teaching program instead reports exhaustion explicitly.

## 11.23 What 'ASCII' means in this program

The source uses numerical range tests such as `c >= 'A' && c <= 'Z'`, and it converts uppercase letters by adding the difference between the lowercase and uppercase initial letter codes.

This assumes an execution character encoding with contiguous alphabetic ranges and uniform uppercase/lowercase offsets, as is true on common ASCII-compatible systems. The C language guarantees that decimal digits are consecutively encoded, but does not require every alphabetic character range to be contiguous on every conforming implementation.

The program's intended scope is ASCII-oriented text processing on a compatible hosted target. It should not be described as a universally portable Unicode tokenizer.

For a locale-sensitive C application, character classification functions in `<ctype.h>` may be appropriate, with their argument-domain restrictions respected. For UTF-8, byte-range classification alone does not identify Unicode letters. A Unicode tokenizer would need decoding and higher-level character-property decisions before indexing.

Explicitly stating this limitation is more accurate than claiming that a small ASCII example automatically supports every text encoding.

## 11.24 The three end-of-input cases

The tokenizer recognizes a word when a separator is encountered after word bytes. But a stream can end immediately after the final word without a separator.

The program therefore has a final step after the input loop: if the current word length is positive, terminate the buffer and insert that last word. Without this step, a file ending in an ordinary letter or digit would silently omit its final token.

There is another case: an overlong word reaches EOF without a separator. The program must still reject it. It does not become legal merely because a delimiter never appeared.

Finally, `getchar` returning `EOF` does not alone establish clean end of input. The program checks `ferror(stdin)` before reporting success. These cases illustrate why stream termination must be part of the algorithm, not an afterthought.

## 11.25 Improve the regression tests

The original test feeds a small mixed-case sentence, checks sorted output and counts, verifies empty input, and expects a word longer than 31 characters to be rejected.

A stronger boundary suite should also test:
- Exactly 31 valid word bytes.
- A 32-byte word rejected without truncated insertion.
- A final valid word with no trailing newline.
- Multiple separators in a row.
- A repeated word at the beginning and end of the stream.
- Output order when inserting distinct words in reverse alphabetical order.

The program exposes failure through a nonzero exit status and a diagnostic written to `stderr`. Tests must inspect the exit status, not infer success from the presence of output alone.

### Further exercises

**11.14** Produce an insertion trace for five words in reverse alphabetical order and count the total number of shifted entries. **11.15** Derive the exact worst-case number of shifts for inserting 1,024 distinct words at the front of the table. **11.16** Add boundary tests for exactly 31 and 32 word bytes. **11.17** Explain what would need to change before the tokenizer could correctly process UTF-8 letters outside ASCII. **11.18** Replace the fixed inline word buffers with dynamically owned strings, then describe the additional ownership and failure cases created by that design.
