# Memory Planning, Arenas, and Tensor Lifetimes

## Why counting weights is insufficient

In a small MCU system the RAM peak is often set by temporary intermediate tensors rather than persistent model weights. A model may have only a few thousand INT8 parameters yet require much more SRAM for a feature window, im2col scratch, audio buffers, inference activations, and task stacks. A disciplined estimate partitions memory by ownership and lifetime:

$$R_{\rm peak}=R_{\rm static}+R_{\rm stacks}+R_{\rm DMA}
+R_{\rm runtime,persistent}+R_{\rm activation,peak}+R_{\rm safety}.$$

This expression is a budget identity, not a claim that every category is known before link-time and target verification. Alignment introduces internal padding. External SRAM, CCM memory and DMA-accessible SRAM may have different eligibility constraints on an STM32 part; the linker script must follow the actual device's memory map.

## Static, stack, pool, and arena storage

A fixed inference graph is an excellent candidate for static allocation. Static storage has a fixed address and lifetime, but not necessarily minimal memory cost. Stack storage is attractive for small bounded scratch objects; it is a poor place for a large tensor in a deeply nested or preemptible call path unless stack capacity is explicitly verified.

An arena is a preallocated region from which objects are placed by offset. A monotonic arena bumps a pointer and releases all allocations together. A planned activation arena reuses overlapping regions only when tensor lifetimes do not overlap. The latter is more sophisticated because allocation decisions can be made once for a fixed model rather than at each inference.

A simple bump arena illustrates the address arithmetic:

```cpp
struct BumpArena {
  std::byte* base;
  std::size_t capacity;
  std::size_t used{0};

  void* allocate(std::size_t bytes, std::size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)))
      return nullptr;
    // base itself must satisfy the required alignment.
    const std::size_t mask = alignment - 1;
    if (used > capacity || used > SIZE_MAX - mask) return nullptr;
    const auto offset = (used + mask) & ~mask;
    if (offset > capacity || bytes > capacity - offset)
      return nullptr;
    used = offset + bytes;
    return base + offset;
  }
};
```

The example relies on the backing `base` being aligned sufficiently for every allocation requested; merely aligning the offset does not correct a misaligned base pointer. On a target, reserve the backing array with `alignas(...)` and verify the strongest required alignment. For portable code, enforce the contract at arena construction. `SIZE_MAX` requires the relevant header or an equivalent `std::numeric_limits<std::size_t>::max()`.

## Lifetime intervals

Suppose a sequence of operations produces tensors $T_i$. Define for each activation an interval

$$I_i=[p_i,c_i],$$

where $p_i$ is the operation that produces it and $c_i$ is the last operation that consumes it. Two tensors may share physical memory only if their live intervals do not overlap under the selected execution convention. In-place operators are a special case: they require both graph semantics and kernel implementation to allow overwrite before all reads complete.

The memory planner seeks offsets $o_i$ minimizing

$$A_{\max}=\max_i(o_i+|T_i|)$$

subject to

$$I_i\cap I_j\ne\varnothing\ \implies\
[o_i,o_i+|T_i|)\cap[o_j,o_j+|T_j|)=\varnothing,$$

and alignment constraints $o_i\equiv0\pmod{a_i}$. This is related to interval allocation and packing. Finding the global optimum for arbitrary graph constraints is not necessary for a useful embedded implementation; a deterministic greedy allocator often provides a good bound. Verify it against the actual graph.

## A small lifetime example

Consider the following graph:

```text
Input A --(op 0)--> B --(op 1)--> C --(op 2)--> D
            \___________________________/
              A/B skip dependency
```

A residual edge that uses $B$ at op 2 means $B$ cannot be overwritten as soon as $C$ exists. If $|B|=6$ KiB, $|C|=8$ KiB, and $|D|=6$ KiB, naive thinking might predict that 8 KiB is enough because the largest tensor is 8 KiB. In fact, the kernel producing $D$ may need $B$ and $C$ concurrently, so the activation peak could be at least 14 KiB plus the destination (depending on in-place support). The last-use graph determines memory; raw layer order alone does not.

For a simple chain without fan-out, ping-pong buffers can often hold alternating activations. But if dimensions vary substantially, a general planner can reuse fragmented space that two fixed equal-sized buffers waste.

## Memory planning with graph intervals

An illustrative offline allocator:

```text
for tensor in tensors sorted by producer operation:
    expire active tensors whose last use precedes producer
    find lowest aligned offset not conflicting with live intervals
    assign offset[tensor]
    add tensor to active set
peak = maximum(offset + size)
```

This is not the only strategy. A worst-fit approach, greedy by tensor size, and globally optimized packing can yield different peaks. A practical build should store the generated offsets alongside an independently checked constraint list, then test the plan with canary patterns and graph execution. An out-of-bounds DMA write can corrupt a perfectly correct tensor plan, so MPU and memory-region protections matter where available.

## An important real-world runtime architecture

TensorFlow Lite Micro documents a shared tensor arena partitioned conceptually into head (nonpersistent data), temporary allocations, and tail (persistent allocations). Its model allocator and memory planner assign lifetimes during preparation; a recording interpreter can report consumed arena space. It is a useful reference architecture, not the only possible one [TFLM memory management].

One especially important distinction is between **reserved arena capacity** and **reported used bytes**. Reserving 64 KiB establishes a static memory commitment even when the allocator reports only 12.5 KiB used for a particular graph. A product cannot reclaim those unused reserved bytes unless its memory architecture explicitly permits it.

## SRAM placement and hardware access

An embedded design may contain multiple SRAM domains with different DMA accessibility and bus performance. On STM32, consult the *specific* STM32F446 reference manual and datasheet rather than assuming SRAM banks or caches behave like a different STM32 family. DMA rings must reside in DMA-addressable memory and respect alignment and peripheral transfer width.

The tensor arena can often be placed separately from DMA buffers via linker sections. However, a well-designed linker script should also make visible the stack reserve, `.data`, `.bss`, read-only model placement, and overflow checks. A successful link is not evidence that worst-case stack usage is safe; collect stack high-water marks and create a margin policy.

## Memory measurement methodology

A report should identify:

- target board and MCU revision;
- compiler version, optimization flags, build SHA;
- linked flash (`text`, `rodata`, `data`) and SRAM (`data`, `bss`, stacks, heaps);
- tensor arena *capacity* versus allocator *used* and peak scratch;
- contiguous free space, alignment and memory-bank restrictions;
- measured stack high-water and whether ISR/driver stacks were included;
- whether results refer to cold start, warm inference, or worst-case input.

A single number named `RAM usage` without a method obscures more than it reveals.

## Failure containment

The allocator must refuse out-of-range requests and propagate an explicit failure status. It must not silently fall back to unbounded heap allocation in the inference path. After initialization, the graph's memory map should be immutable for deterministic inference. Reset/recovery must reinitialize stateful operators, rolling feature windows, and detection filters consistently, not just clear a global fault variable.

**Exercises**

1. Given activation sizes $[4,8,6,5]$ KiB in a straight chain, derive the minimum peak for a ping-pong schedule under an out-of-place kernel contract.
2. Extend the interval allocator to handle a residual connection and prove that two overlapping live tensors cannot alias.
3. Estimate the real SRAM obligation of a 24-KiB used arena inside a statically reserved 64-KiB array.
