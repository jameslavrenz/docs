# 16. Atomics, Threads, and Communication

Two threads executing the same valid sequential code do not necessarily produce a valid concurrent program. In C, unsynchronized conflicting accesses to a non-atomic object can form a data race and yield undefined behavior. The standard atomic library gives us a way to coordinate access, but it does not eliminate the need to design ownership and progress.

## 16.1 A race on a counter

Suppose two threads each execute `++counter` one thousand times. The operation is a read, computation, and write; it is not necessarily one indivisible action. Without suitable synchronization, concurrent increments of an ordinary `int` are a data race. A result of two thousand is not guaranteed, and the program has undefined behavior rather than merely a risk of losing a few increments.

An atomic object makes individual accesses atomic:

```c compile-check
#include <stdatomic.h>

atomic_uint counter = 0;

/* Called by either worker: */
void count_event(void)
{
    atomic_fetch_add_explicit(&counter, 1u, memory_order_relaxed);
}
```

Relaxed ordering suffices when all that matters is a single independent count, not the visibility of some other data. It does not order ordinary memory operations in a way that can publish a complex object.

## 16.2 Publication with release and acquire

Imagine a producer fills a payload and then signals that it is ready. The consumer must not read the payload before the producer's writes have been made visible through synchronization.

```c compile-check
#include <stdatomic.h>
#include <stdbool.h>

static int payload;
static atomic_bool ready = false;

void publish(int value)
{
    payload = value;
    atomic_store_explicit(&ready, true, memory_order_release);
}

bool try_consume(int *out)
{
    if (out == nullptr)
        return false;
    if (!atomic_load_explicit(&ready, memory_order_acquire))
        return false;
    *out = payload;
    return true;
}
```

This is a **one-shot publication example**. It is valid only under a protocol in which one producer publishes once, after which `payload` is never modified, and consumers do not access it before an acquire load observes `true`. Reusing the flag for repeated publications without an additional state transition and synchronization would be incorrect.

A release store synchronizes with an acquire load that reads from it, establishing a happens-before relationship for earlier writes. The atomic flag does not itself turn every access to `payload` into an atomic access.

## 16.3 A bounded single-producer/single-consumer queue

For repeated communication we need a real protocol. A single-producer/single-consumer (SPSC) ring buffer can keep separate atomic positions. Only the producer modifies the write position, and only the consumer modifies the read position.

Reserve one slot to distinguish full from empty. With capacity eight, the queue can hold at most seven items. The producer writes a non-atomic data element, then publishes its advanced write position with release semantics. The consumer acquires the write position before reading the element. The consumer releases its advanced read position after finishing; the producer acquires it before reusing the slot.

The example in `examples/spsc_queue.c` implements precisely this one-producer/one-consumer algorithm. It is **not** a multiple-producer or multiple-consumer queue. It also does not directly establish that its operations are safe to invoke from an interrupt handler on every implementation: that is a separate target-specific question involving lock-free guarantees, runtime support, and interrupt semantics.


**C23 initialization note.** Older atomic examples may use `ATOMIC_VAR_INIT`, but that macro was removed in C23. The standalone SPSC example uses ordinary `0` initialization for its atomic index members before either participant starts. Once concurrent operation begins, the participants access those positions only through atomic operations with the documented ordering.

## 16.4 Why index wraparound is not numeric overflow

An SPSC ring normally advances an index with `(index + 1) % CAPACITY`. If index values are always less than capacity, and capacity is suitably small, the addition stays in range. The modulo computes a position within the array. It should not be confused with allowing arbitrary integer overflow.

An invariant for the chosen queue is `0 <= read_index < CAPACITY` and `0 <= write_index < CAPACITY`. Equal positions signify empty; advancing the writer to the reader's current position would signify full and must be rejected.

## 16.5 Lifetime and shutdown

A queue's memory must remain alive until both participants stop accessing it. Atomic operations do not rescue a buffer freed while a worker still runs. A threaded application therefore needs a termination protocol and a join or equivalent completion mechanism.

This often has greater practical importance than micro-optimizing a memory order. A formally correct queue implementation is still unsafe if its containing object is destroyed too early.

## 16.6 Mutexes remain valuable

Lock-free algorithms are not necessarily faster, simpler, or more predictable. A mutex protects a group of related operations and is often the right design for moderate contention. In an RTOS, priority inheritance and interrupt-safe API restrictions matter; do not assume that a host C mutex can be used inside an interrupt handler.

Atomicity, mutual exclusion, wait-freedom, and real-time boundedness are different properties. Assess them independently.

## 16.7 C23 and implementation support

The C atomic facilities are specified by the language and standard library, but target availability and lock-free properties depend on the implementation. Check `atomic_is_lock_free` when relevant and verify the supported interrupt and threading model on the actual hardware. Freestanding environments may provide only a subset of hosted facilities.

On common hosted implementations, compiling threaded examples may require additional compiler and linker options. Keep those options separate from programs that do not require threading.

## 16.8 Exercises

**16.1** Explain why `volatile unsigned counter` does not fix a data race.

**16.2** Draw the release/acquire relationships for one successful SPSC enqueue and dequeue.

**16.3** Extend the SPSC example with a clear shutdown protocol. Explain object lifetime requirements.

**16.4** Determine whether a capacity-eight ring that leaves one slot empty can hold eight elements. Provide tests for full and empty transitions.

**16.5** Explain why changing two producer threads to call the same SPSC enqueue operation breaks the algorithm.

**16.6** Compare an SPSC queue with a mutex-protected queue for latency, correctness, maintainability, and portability.


## 16.9 Identify the exact shared objects

Concurrency reasoning starts by listing which objects are accessed by more than one participant. In the SPSC queue, the producer publishes `tail` and observes `head`; the consumer publishes `head` and observes `tail`. Both positions are atomic. Data slots are ordinary objects, but the publication protocol ensures the producer and consumer do not access the same occupied slot without the appropriate ordering.

Merely declaring the indices atomic is not sufficient if the buffer is accessed outside that protocol. Every operation must respect the rule that only the producer enqueues, only the consumer dequeues, and the queue object remains alive until both stop.

## 16.10 Understanding the release/acquire pair

A producer writes a slot, then uses a release store to publish the new tail. A consumer that reads the published tail with an acquire load can safely observe the preceding slot write. After consuming the element, the consumer releases its new head position. The producer uses an acquire load of head before deciding that a slot can be reused.

The two directions solve distinct problems: publication of newly written data and reclamation of space after consumption. Using only one direction would not establish all of the required cross-thread ordering in the intended design.

## 16.11 Sequential tests do not demonstrate concurrency correctness

A single-threaded test that pushes seven items, observes full, pops three, and wraps around is useful for verifying index arithmetic and FIFO ordering. It does not establish absence of a data race during simultaneous operations.

A concurrency test can run one producer and one consumer with a known sequence and verify that every item is transferred exactly once and in order. A stress test with variable delays may expose mistakes, but even millions of successful transfers cannot prove the memory-order argument. Reasoning about the C memory model remains necessary.

## 16.12 Interrupts are outside a generic hosted-thread proof

An interrupt handler is not automatically equivalent to a C standard thread. The compiler, runtime, architecture, and RTOS specify which atomic operations are safe in that context, whether they are lock-free, and how memory barriers interact with devices.

An atomic operation implemented through a library helper or lock may be unsuitable in an interrupt. Even lock-free machine instructions can require cache coherency or architectural barriers beyond what a generic application-level description conveys. Verify the target before advertising an SPSC queue as interrupt-safe.

## 16.13 Lifetime and cancellation

Stopping a producer is not the same as ensuring it has stopped. A queue owner must establish that neither participant will access the queue before releasing its memory. A shutdown protocol might signal cancellation, wake blocked consumers, join worker threads, and only then destroy the queue.

The choice between draining and discarding pending items should be explicit. Either can be correct depending on the application, but implicit destruction while work remains creates lifetime hazards.

**Further problems:** Create a two-thread FIFO stress test recording item sequence numbers. Explain the effect of replacing a release store with a relaxed store. Sketch a shutdown protocol for a queue shared between a worker thread and an RTOS task.


## 16.14 Memory ordering is a protocol, not a performance switch

The SPSC queue works because it establishes two publication relationships. The producer writes an ordinary slot and then publishes an updated tail through a release operation. The consumer acquires that tail before reading the slot. In the opposite direction, the consumer finishes reading the slot and releases its updated head; the producer acquires head before reusing that storage.

It is tempting to label every atomic access `memory_order_relaxed` for speed. But relaxed atomics guarantee the atomicity and modification order of the atomic object, not the required visibility of ordinary slot writes to another thread. The release/acquire relationships are part of this algorithm's proof.

Conversely, using `memory_order_seq_cst` everywhere does not magically repair a design that lets two producers concurrently modify a slot intended for one producer. Memory ordering and permitted roles are separate requirements.

## 16.15 Read the single-producer guarantee literally

An SPSC queue has one producer and one consumer *at a time under the documented protocol*. Calling `push` from a second producer can make both read the same tail, write the same slot, and publish conflicting updates. Atomic tail storage does not turn the remaining sequence into an atomic transaction.

A multiple-producer queue usually needs additional synchronization or a different algorithm. If a single interrupt is the producer and one task is the consumer, you must still verify that the compiler, target architecture and RTOS support the chosen atomic operations in that interrupt context.

A callback that sometimes runs on the producer thread and sometimes runs in an interrupt is not automatically a single producer under a safe schedule. Identify the actual execution contexts.

## 16.16 Progress guarantees are different

A nonblocking operation returns without sleeping for a mutex, but the terms *lock-free* and *wait-free* have precise algorithmic meanings. Lock-free generally concerns system-wide progress; wait-free concerns per-operation completion in a bounded number of steps. Neither necessarily means low latency under every cache, interrupt or scheduling condition.

The small SPSC queue has bounded local index work, but whether its atomic primitives are implemented without locks depends on the target. On a small MCU, a supposedly atomic operation may be lowered to a helper routine that is inappropriate in an interrupt handler.

Measure real target execution and verify the atomic implementation before making a real-time guarantee.

## 16.17 Shutdown and ownership

Memory ordering does not establish object lifetime by itself. Imagine a queue owner frees the queue while a consumer is about to load its head. Even though the member is atomic, the storage is gone and the access is invalid.

A shutdown protocol can announce cancellation, stop the producer, let the consumer drain or abandon remaining items by policy, wait for both to complete, and only then reclaim storage. A good API states who owns the queue object and which operation establishes that no in-flight access remains.

For embedded systems, task deletion and ISR unregistering can each require target-specific synchronization. Test the shutdown path, not only steady-state traffic.

### Exercises

**16.7** Draw the two release/acquire edges on an eight-slot queue. **16.8** Explain why a second producer is unsafe despite atomic index variables. **16.9** Compare mutex-based and atomic-based designs for a real-time telemetry queue. **16.10** Describe a shutdown sequence that makes destruction safe.


## 16.18 A complete two-thread atomic counter

The repository's `examples/thread_counter.c` creates two hosted C threads through the facilities in `<threads.h>`. Each worker performs 20,000 increments on one shared `atomic_uint`. The main thread joins both workers and verifies that the final result is 40,000.

The worker's essential operation is:

```c
atomic_fetch_add_explicit(&ctx->count, 1u, memory_order_relaxed);
```

This read-modify-write operation is atomic. Two workers cannot accidentally overwrite one another's increments through the classic load/increment/store race on an ordinary integer. The example deliberately chooses a total that fits even in the standard minimum range of `unsigned int`.

Why is relaxed ordering sufficient? The program cares only about the counter's value. It does not use that counter to publish a separate ordinary data structure. The atomic object has an atomic modification order, and successful thread joins establish that the workers have completed before the main thread checks the result.

If a producer filled an ordinary object and then incremented the counter to announce that the object was ready, the design would be different. Relaxed operations on the counter alone would not establish the necessary ordering of writes to the accompanying non-atomic object.

## 16.19 Partial thread creation is a resource problem

Suppose creating the first worker succeeds and creating the second fails. The first worker may still be executing and using a pointer to the caller's stack-local context.

The correct cleanup principle is to remember how many workers started and join them all before allowing that context's lifetime to end. Returning immediately after the second creation error could leave the first thread with a dangling pointer.

This is the same ownership discipline used for partially initialized files, allocations, and device resources: acquire incrementally, track what succeeded, and clean up what was acquired on every failure path.

The example checks each creation and join result, although a failed join in a production thread framework may require a more comprehensive recovery policy. A passing test of ordinary creation and completion does not prove all exceptional thread-system conditions are recoverable.

## 16.20 One-shot release/acquire publication

An atomic counter is not the same as publishing a payload. Consider a shared ordinary `int payload` and an atomic readiness flag. The intended one-shot protocol is:

```text
Producer                              Consumer
--------                              --------
payload = value;
store ready=true (release)            load ready (acquire)
                                      if true, read payload
```

When the consumer's acquire load reads from the producer's release store, the release/acquire synchronization establishes that the preceding payload write happens-before the later read.

This explanation depends on a **one-shot** lifetime rule: the producer writes payload once and does not overwrite it after publication. Reusing the flag and publishing a second value without coordinating with the consumer is not automatically safe. A later write could race with the earlier consumer's read.

Using `volatile` instead of an atomic flag would not establish this general thread synchronization guarantee.

## 16.21 Two distinct directions in an SPSC ring

In the book's single-producer/single-consumer queue, the producer writes a slot and release-stores the updated tail. The consumer acquire-loads the tail before reading the newly available slot.

The opposite direction matters too: after the consumer finishes reading a slot, it release-stores an advanced head; the producer acquire-loads head before deciding that the old slot is safe to reuse. One direction publishes data; the other publishes reclaimed capacity.

The queue assumes exactly one producer and one consumer under this protocol. Adding a second producer is not a harmless increase in workload. Two producers may observe the same tail, write the same slot, and publish conflicting states. Atomic positions alone do not turn a sequence of operations into a multiple-producer transaction.

## 16.22 Different tests answer different questions

A sequential queue test verifies index arithmetic, FIFO behavior, full/empty conditions, and wraparound for chosen operations. A concurrent producer-consumer test may expose interleaving failures. The two-thread counter demonstrates that the test environment supports concurrent atomic read-modify-write operations.

None of those tests proves all possible memory-ordering arguments for an unrelated lock-free algorithm. Correctness also requires reviewing which participant owns each slot, which objects are shared, what establishes happens-before relationships, and when storage may be destroyed.

A host ThreadSanitizer run can detect certain data races on executed paths, but its instrumentation changes timing and may not model MCU interrupt or DMA semantics.

## 16.23 What the C thread library does not promise for a microcontroller

The hosted threads facility is not universally provided by freestanding C implementations. A desktop build may need a compiler/linker flag such as `-pthread`; an embedded project instead uses RTOS task facilities or target-specific interfaces.

The C memory model provides a useful conceptual foundation, but whether a particular atomic operation is lock-free or safe inside an interrupt handler is an implementation-specific matter. An atomic operation lowered to a runtime library helper may be inappropriate in an ISR. DMA buffers also introduce coherency requirements outside the ordinary two-thread example.

Do not equate lock-free with deadline-safe. System-wide progress is not the same as a proven maximum completion time for every caller. A real-time design must account for preemption, cache, scheduling, blocking and priority behavior.

### Additional exercises

**16.11** Explain why `memory_order_relaxed` is sufficient for a standalone atomic count but not for publication of an ordinary payload. **16.12** Draw all resources after one thread creation succeeds and the second fails. **16.13** Label the two release/acquire handoff directions of an SPSC queue. **16.14** Describe the additional requirements for making the queue safely usable by an MCU interrupt handler. **16.15** Distinguish lock-free, wait-free, and bounded real-time execution.
