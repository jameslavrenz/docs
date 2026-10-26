# Mathematics of Inference on an MCU

## The inference graph as a composition

A trained model is a parameterized, deterministic function composed of tensor operators. For a feedforward graph with layers $F_1,\dots,F_L$, evaluation is

$$a^{(0)}=x,\qquad a^{(\ell)}=F_\ell(a^{(\ell-1)};\theta_\ell),\qquad y=a^{(L)}.$$

For inference, $\theta_\ell$ is fixed. There is no gradient calculation, optimizer state, or training dataset resident in firmware. Unlike training, the primary computational problems are: implement $F_\ell$ correctly, schedule buffer lifetimes, minimize expensive transfers, and preserve numerical behavior through quantization.

A tensor is more than a pointer. At minimum it has an element type, rank, dimensions, stride order, layout, quantization metadata, and an ownership/lifetime contract. A tensor $X\in\mathbb{R}^{N\times H\times W\times C}$ in NHWC order typically stores its elements at

$$\operatorname{addr}(n,h,w,c)=\operatorname{base}+
[((nH+h)W+w)C+c]\cdot b,$$

where $b$ is bytes per element. NCHW is equally valid but differs in locality and therefore affects kernel design. A silent layout mismatch often produces plausible-looking yet incorrect outputs.

## Affine layers and nonlinearities

The elementary dense layer is

$$z_j=b_j+\sum_{i=0}^{m-1}W_{ji}x_i, \qquad a_j=\phi(z_j).$$

The cost is $mn$ MAC operations, $mn$ weights, $n$ biases, and (without in-place tricks) $m+n$ activation values. A ReLU activation is $\phi(z)=\max(z,0)$; ReLU6 clips to $[0,6]$. An activation can be fused with its producer, avoiding a separate memory pass.

For a piecewise-linear network, a great deal of the apparent mathematical sophistication reduces to repeated integer dot products, clipping, and carefully specified rescaling. The difficulty is not the abstract formula but exact integer ranges, tensor indexing, and performance under constrained memory.

### Worked example: a two-output layer

Let

$$W=\begin{bmatrix}1&-2&1\\-1&1&2\end{bmatrix},\qquad
x=\begin{bmatrix}2\\3\\-1\end{bmatrix},\qquad
b=\begin{bmatrix}1\\-2\end{bmatrix}.$$

Then $Wx+b=(-4,\,-3)^T$ and applying ReLU yields $(0,0)^T$. The example is intentionally small enough to calculate by hand; it should become a reference test before optimized kernels are introduced. In a deployed integer network, the operands and output may have different scales, so Chapter 3 adds the requantization step.

## Two-dimensional convolution

In machine-learning libraries, the operator usually called *convolution* is spatial cross-correlation: the kernel is not reversed. For NHWC input $X$, weights $K\in\mathbb{R}^{K_h\times K_w\times C_{\rm in}\times C_{\rm out}}$, output $Y$, padding $(P_h,P_w)$ and stride $(S_h,S_w)$,

$$Y[n,h,w,o]=b_o+\sum_{r=0}^{K_h-1}\sum_{s=0}^{K_w-1}
\sum_{c=0}^{C_{\rm in}-1}X[n,hS_h+r-P_h,wS_w+s-P_w,c]K[r,s,c,o],$$

with out-of-range $X$ treated as zero in real-number semantics. The output dimensions, for dilation $D=1$, are

$$H_{\rm out}=\left\lfloor\frac{H+2P_h-K_h}{S_h}\right\rfloor+1,\qquad
W_{\rm out}=\left\lfloor\frac{W+2P_w-K_w}{S_w}\right\rfloor+1.$$

In a quantized implementation, *real zero* is represented by the input tensor's zero point, not necessarily by integer 0. Padding with literal integer zero in an asymmetric tensor is a correctness bug.

The approximate MAC count is

$$N H_{\rm out}W_{\rm out}C_{\rm out}K_hK_wC_{\rm in}.$$

For illustration, a $1\times16\times16\times8$ feature tensor convolved with $3\times3$ kernels to 16 output channels at stride one and 'same' padding requires $16\cdot16\cdot16\cdot3\cdot3\cdot8=294{,}912$ MACs; the output has $4{,}096$ values. An INT8 output occupies 4 KiB; an FP32 output occupies 16 KiB. Those bytes matter as much as the MAC count.

## Depthwise and pointwise factorization

A conventional convolution jointly mixes spatial neighborhoods and input channels. A depthwise convolution applies one $K_h\times K_w$ spatial filter to each input channel (or a small multiplier per channel). A pointwise $1\times1$ convolution then mixes channels.

For $C_{\rm in}=C$, $C_{\rm out}=O$, output area $HW$, kernel size $K=K_hK_w$:

$$\operatorname{MAC}_{\rm standard}=HWKCO,$$

$$\operatorname{MAC}_{\rm depthwise+pointwise}=HW(KC+CO).$$

Their ratio is

$$\rho=\frac{KC+CO}{KCO}=\frac{1}{O}+\frac{1}{K}.$$

With $K=9$ and $O=32$, $\rho\approx0.142$. This is a 7-fold reduction in nominal MAC count, **not** a promise of a 7-fold speedup. Overheads, scratch buffers, cache locality, and support for each operator can alter the outcome. Tiny models with very narrow channels may not amortize optimized-kernel setup costs.

## Pooling, reduction, and classification

Average pooling over a region $\Omega$ is

$$y_c=\frac{1}{|\Omega|}\sum_{(h,w)\in\Omega}x_{h,w,c}.$$

The division and rounding policy must be specified for integer arithmetic. Maximum pooling selects the largest value; if input and output quantization are identical, comparisons can remain entirely in the integer domain.

For logits $z_i$, softmax is

$$p_i=\frac{\exp(z_i-m)}{\sum_j\exp(z_j-m)},\qquad m=\max_j z_j.$$

Subtracting $m$ improves numerical stability. For simple argmax classification, softmax is unnecessary because $\arg\max_i z_i=\arg\max_i p_i$. But if the downstream detection logic uses calibrated confidence thresholds, dropping softmax without revisiting the policy changes system semantics.

## Parameter counting and model metadata

A model with $n$ INT8 weights has $n$ bytes of raw weight values. But the binary artifact includes bias arrays (often INT32), tensor shapes, offsets and scales, operator definitions, scheduling metadata, and serialization overhead. The flash cost can exceed the parameter payload by a large factor for tiny graphs.

Example: a dense layer with $128$ inputs and $16$ outputs has $2048$ INT8 weight bytes and $64$ bytes of INT32 bias. That is $2112$ bytes of parameter payload, excluding alignment and metadata. A layer with $5{,}000$ parameters need not produce a $5{,}000$-byte model file.

## Graph structure determines buffer reuse

A chain $A\to B\to C$ allows the storage of $A$ to be reused when $B$ has consumed it. A skip connection $A\to B\to C$ with $A$ also used by $C$ lengthens $A$'s lifetime. The minimum peak activation memory is thus a graph-liveness problem, not a simple sum of model dimensions. Treating all model tensors as simultaneously live can grossly over-allocate; treating them as sequential without analyzing fan-out can corrupt live values.

## Precision is part of correctness

A floating-point reference normally defines the conceptual graph. Integer inference approximates it, but the reference that matters for regression tests is the *specified quantized graph*, including signed ranges, zero points, rounding, accumulation and clamping. Two mathematically reasonable requantization routines may differ by one output least-significant bit. That discrepancy can be important near a threshold or on a long sequence of operators.

For testing, separate three assertions: (1) structural correctness of shape and indexing, (2) integer agreement against a known reference for each operator, and (3) application-level accuracy against representative sensor data. Passing one does not imply the other two.

## Chapter design review

A good model handoff for MCU deployment includes input dimensions and physical units, layout, normalization, tensor scales and zero points, complete operator set and versions, arena estimate, flash estimate, reference test vectors, dataset provenance, and failure/unknown-class behavior. A model artifact without these contracts is an ambiguous binary dependency.

**Exercises**

1. Compute the MACs and parameter payload for a $3\times3$ convolution with eight input channels, 24 output channels and a $20\times20$ output plane.
2. Show precisely how the output changes if one mistakenly uses NCHW addresses for an NHWC tensor.
3. Prove that argmax is invariant under softmax, assuming exact arithmetic and finite logits. Explain why output calibration is not invariant.
