# Roadmap to a ~200–230-page textbook

**Status:** 13-chapter draft plus three appendices and twelve standalone examples; still below the target length. The next milestone is expanded chapters and tested examples, then layout and typeset page count.

The first-edition-scale book should grow through:
1. Expanded explanations and more progressive examples per chapter.
2. A verified suite of standalone C23 sample files for every code listing.
3. A complete set of exercises with selected answers, including boundary/error cases.
4. A detailed appendix covering expression syntax, precedence, declarations, standard headers, C23 changes, and compiler portability.
5. Bibliography, glossary, back-of-book index, and controlled typographic page layout.
6. A reproducible manuscript-to-PDF pipeline and final rendered-page QA.

**Editorial policy:** Original authorship; never present this as K&R's third edition or reproduce their chapters, prose, or exercises. Every complete listing must be compile-tested with real toolchains. Clearly mark illustrative fragments. Correctness and concision outrank arbitrary page inflation.

## Latest continuation

Chapter 13 adds testing methodology, sanitizer guidance, property testing, fuzzing, and host-versus-target verification. The repository-wide GitHub Actions workflow at `.github/workflows/modernc-book.yml` targets GCC 14 and Clang 18. Its execution status must be checked independently; a committed workflow is not evidence of a passing run.
