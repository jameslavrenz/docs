# Preface {-}

This book is for a practicing embedded software engineer who already knows C or C++, interrupts, memory maps, and how to debug a board, but who has not necessarily built a neural-network inference engine. It is *not* a survey of cloud AI. Its central question is: **what must an engineer understand to place a useful, auditable, bounded-latency learned function inside a microcontroller product?**

The emphasis is on Arm Cortex-M devices, in particular integer inference, deterministic memory allocation, real-time integration, DSP preprocessing, and proof by measurement. The numerical methods are portable to other MCU families. Training normally takes place on a development computer; the deployed firmware owns preprocessing, model storage, inference, decision logic, faults, and telemetry.

An original reference kernel is valuable pedagogically even when an optimized production library ultimately replaces it. We will first derive the arithmetic, specify tensor layouts and lifetimes, and write the simplest correct operator. Only then will we discuss optimized backends such as CMSIS-NN. Several case studies refer to the author's public projects, [KWSKit](https://github.com/NetKit-Labs/kwskit), [NetKit](https://github.com/NetKit-Labs/netkit), and [MemKit](https://github.com/NetKit-Labs/memkit). Those projects are independent implementations; the small code snippets in this book are explanatory examples, not drop-in copies of their internals.

**Evidence conventions.** Equations and illustrative calculations are distinguished from measured device observations. Any observed numbers are attributed to a specified repository's recorded test session. A benchmark from one build is never called a worst-case execution-time guarantee, and an offline training accuracy is never called deployed field accuracy. The manuscript is a technical draft, not a safety certification or validated product design.

**Reading path.** Chapters 1--3 establish requirements, mathematical semantics, and quantization. Chapters 4--5 address an original integer runtime and static memory planning. Chapters 6--8 cover audio DSP, system scheduling, and performance optimization. Chapters 9--10 explain the training-to-firmware workflow and complete system design. Chapters 11--13 address verification, a tested integer implementation, and board integration. Chapter 14 provides a reproducible end-to-end classifier; Chapter 15 discusses measured portfolio case studies and the limits of the evidence. Appendices provide a bring-up checklist, glossary, and a compact design worksheet.

**Notation.** Unless qualified, tensor indices are zero-based, convolution is cross-correlation as implemented in contemporary ML libraries, and an INT8 inference tensor uses signed integers in $[-128,127]$. 'Latency' refers to elapsed wall-clock execution time; 'cycles' refers to counted core cycles, which must be qualified by the clock and measurement boundary.

\newpage
