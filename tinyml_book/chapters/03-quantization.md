# Quantization and Fixed-Point Arithmetic


![The integer path includes the accumulator, a specified rounding step, and output clamping.](figures/quant-new.png){width=4.45in}

## What INT8 represents

A quantized tensor stores integers $q$ that approximate real values $r$ using a positive scale $s$ and integer zero point $z$:

$$r \approx s(q-z),\qquad q=\operatorname{clamp}_{[q_{\min},q_{\max}]}
\left(\operatorname{round}\left(\frac{r}{s}\right)+z\right).$$

This is *affine* quantization. For signed 8-bit tensors, $q\in[-128,127]$. The zero point ensures real zero can be represented exactly when $z$ is within the integer interval. Symmetric quantization is the special case $z=0$ (or an equivalent scheme with implementation-specific range conventions).

The scale is not dimensionless: if $r$ is a feature in volts, $s$ has volts per integer count. At network boundaries, input calibration must match training-time preprocessing. A mismatch between a logarithmic-mel feature's natural logarithm and base-10 logarithm, for example, cannot be repaired by a correct convolution kernel.

## Choosing scale and zero point

Given a calibrated real interval $[r_{\min},r_{\max}]$ and integer interval $[q_{\min},q_{\max}]$, a common mapping is

$$s=\frac{r_{\max}-r_{\min}}{q_{\max}-q_{\min}},\quad
z=\operatorname{round}\left(q_{\min}-\frac{r_{\min}}{s}\right),$$

with $z$ clamped to the representable range. If the calibration interval does not include zero, the chosen scheme should be designed to ensure zero's exact representation when the kernel semantics require it. Calibration is not a mere numerical preprocessing choice; it is an engineering decision based on representative data and the acceptable saturation rate.

**Worked example.** Suppose $[-1,1]$ is mapped to signed INT8. Then $s=2/255\approx0.00784314$, and the rounded zero point is approximately $-1$ or $0$ depending on the prescribed rounding rule and endpoint convention. The attainable positive and negative extremes will not be perfectly symmetric in a full 256-code signed range. Implementations must use the model's stored $(s,z)$ rather than recomputing them with a preferred convention.

## Deriving the integer dot product

Let input and weight tensors obey $x_i\approx s_x(q_{x,i}-z_x)$ and $w_i\approx s_w(q_{w,i}-z_w)$. Then

$$\sum_i x_iw_i \approx s_xs_w\sum_i(q_{x,i}-z_x)(q_{w,i}-z_w).$$

Define an integer accumulator

$$A=\sum_i(q_{x,i}-z_x)(q_{w,i}-z_w)+b_q,$$

where $b_q$ approximates $b/(s_xs_w)$, typically in INT32. If weights are symmetric, $z_w=0$, simplifying the multiply. For an output tensor $(s_y,z_y)$,

$$q_y=\operatorname{clamp}\left(
\operatorname{round}\left(\frac{s_xs_w}{s_y}A\right)+z_y\right).$$

The factor $M=s_xs_w/s_y$ is the **requantization multiplier**. Real implementations avoid per-output floating-point multiplication by approximating $M$ with a fixed-point integer multiplier and shift. The exact rounding and saturation semantics must match the selected model runtime and kernel API.

### The accumulator is not an 8-bit integer

The product of two centered INT8 operands may require more than 8 bits. When $z_x$ is nonzero, the centered input $q_x-z_x$ can be as large as 255 in magnitude. An $N$-term dot product may require roughly

$$|A|\le N\max|q_x-z_x|\max|q_w-z_w|+|b_q|.$$

For $N=1024$, a crude bound with both centered values bounded by 255 is $66{,}585{,}600$ before bias, which fits signed INT32; sufficiently long accumulations need explicit overflow reasoning. Do *not* assume every possible operator shape fits INT32 without examining the actual numerical constraints. Integer overflow in C/C++ signed arithmetic is undefined behavior; production kernel implementations must honor their documented range preconditions.

## Per-tensor and per-channel parameters

For per-tensor quantization, one pair $(s_w,z_w)$ covers an entire weight tensor. For per-output-channel quantization, each output channel $o$ may use its own $s_{w,o}$ (commonly symmetric $z_{w,o}=0$). Then

$$M_o=\frac{s_xs_{w,o}}{s_y},\qquad q_{y,o}
=\operatorname{requantize}(A_o,M_o)+z_y.$$

Per-channel quantization often preserves accuracy when different filters have different ranges, at the cost of storing arrays of multipliers and shifts and selecting the correct element per output channel. The multiplier arrays are frequently negligible relative to convolution weights, but not necessarily negligible in a model containing many tiny layers.

## Fixed-point multiplication and rounding

For a positive real multiplier $M$, write

$$M \approx m 2^e,\qquad m\in[1/2,1),$$

and quantize $m$ to a signed high-resolution integer $Q$ such that $m\approx Q/2^{31}$. Then an integer approximation to $MA$ is formed using high-multiply, an appropriately signed shift, and a specified rounding rule. Details such as whether ties round away from zero, to even, or by a specialized saturating-doubling-high-multiply primitive affect bit-exact parity. They must not be improvised in an MCU port.

The naive operation

```cpp
int32_t result = (acc * multiplier) >> shift;
```

is dangerous: `acc * multiplier` can overflow a 32-bit temporary before shifting, signed right shifts and negative rounding need explicit semantics, and multiplication may require 64-bit or DSP instructions. CMSIS-NN provides its own arithmetic helpers and defines quantized kernel arguments; when using them, derive expected values from the same contract [CMSIS-NN].

## Integer activation clamping

For ReLU, real output below zero is clamped to zero, represented in the quantized tensor by $z_y$. Therefore:

$$q_{\rm relu}=\max(q_y,z_y).$$

For ReLU6, the upper endpoint is $\operatorname{round}(6/s_y)+z_y$, clipped to the signed range. A correct fused kernel adds output zero point, then clamps to the *output quantization domain*. Clamping before adding $z_y$ is wrong for asymmetric output.

## The quantized convolution loop

For each output $(h,w,o)$, the conceptual reference calculation is:

```text
acc ← bias[o]  // already scaled to input × weight units
for kh, kw, cin in receptive field:
    x ← input_or_padding(h, w, cin)
    w ← weight(kh, kw, cin, o)
    acc ← acc + (x - input_zero_point) * (w - weight_zero_point)
acc ← requantize(acc, multiplier[o], shift[o])
acc ← acc + output_zero_point
output ← clamp(acc, activation_min, activation_max)
```

Every index must be bounds-checked during development. The padding value is `input_zero_point`; since this gives zero after centering, it has no contribution. This also makes the distinction between mathematical convolution and integer storage concrete.

## End-to-end error accumulation

A single scalar has absolute quantization error at most about $s/2$ when not saturated, under round-to-nearest. For a dot product $y=\sum_i x_i w_i$, writing $\hat x_i=x_i+\delta x_i$ and $\hat w_i=w_i+\delta w_i$, the error is

$$\hat y-y=\sum_i(w_i\delta x_i+x_i\delta w_i+\delta x_i\delta w_i).$$

Consequently,

$$|\hat y-y|\le\sum_i (|w_i||\delta x_i|+|x_i||\delta w_i|
+|\delta x_i||\delta w_i|),$$

before additional output requantization error. Saturation destroys this small-error bound, because clipped values may be arbitrarily far from the true real value. Therefore histograms of clipping and per-layer comparison to float or high-precision reference are more informative than merely observing that the final classifier agrees on a handful of examples.

## A deterministic quantization test suite

Use a reference implementation with explicitly wide intermediates and a specified rounding mode to test each kernel:

- Zero-point identity: padding and real zero must contribute zero to centered accumulation.
- Endpoints: $-128$, $127$, negative bias, and saturated output values.
- Shape diversity: odd widths, strides, multiple output channels and asymmetric pads.
- Per-channel scales: an example where two channels require different right shifts.
- Randomized parity: thousands of seeded tensors compared elementwise with a chosen runtime reference.
- Full graph: compare each intermediate tensor with a known quantized runtime, not just the final argmax.

Do not tune quantization solely to the recognition score of a small convenience dataset. Hold out representative devices, operators, backgrounds, and operating conditions; record the exact model hash and preprocessing version.

## Chapter design review

The device receives a *quantized numerical contract*, not just a collection of INT8 weights. Before firmware integration, freeze tensor layout, operator version, scale and zero-point arrays, rounding behavior, bias scaling, activation limits, signedness, and reference vectors. A one-count disagreement might be harmless in a demonstration, but explaining it precisely is essential if this is to be a reproducible embedded component.

**Exercises**

1. Given $s_x=0.02$, $s_w=0.015$, and $s_y=0.01$, compute $M$ and the ideal unrounded output for $A=-37$, $z_y=-4$.
2. Derive the integer padding value for a tensor with $z_x=-12$, and explain why a zero byte would be incorrect.
3. Bound a length-4096 accumulation given $|q_x-z_x|\le255$ and $|q_w|\le127$. Does the bound exceed signed INT32?
