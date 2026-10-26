# DSP Front Ends for Embedded Audio

## Why preprocessing is part of the model

A keyword-spotting network usually does not receive unscaled raw microphone bytes. Its learned parameters assume a specific preprocessing function: sample rate, channel handling, framing, window coefficients, FFT normalization, frequency scale, logarithm, clipping, and final quantization. The *same model file* can fail badly if the firmware implements even a seemingly innocuous variant of these steps. Hence preprocessing has a versioned numerical contract and needs reference vectors like any neural operator.

Consider $x[n]$, a 16-kHz sampled mono sequence. A frame length of $N=512$ samples corresponds to 32 ms; a hop $H=256$ corresponds to 16 ms and 50% overlap. A fixed-size circular buffer holds the most recent $N$ samples. Every hop produces one feature row, and a sliding history of rows forms the classifier input.

## Continuous-time acquisition and discrete frames

If a signal has appreciable energy above Nyquist, sampling aliases that energy into the band of interest. Anti-alias filtering, microphone decimation, and clock accuracy therefore affect model features. Audio sensor interfaces such as PDM may have a worker or DMA subsystem that generates PCM; do not assume PDM callback invocation always occurs at the final PCM sample rate or that its work may be performed safely inside the highest-urgency ISR.

Framing uses

$$x_m[n]=x[n+mH],\qquad 0\le n<N.$$

The $m$th windowed frame is

$$u_m[n]=w[n]x_m[n].$$

A periodic Hann window is

$$w[n]=\frac12\left(1-\cos\frac{2\pi n}{N}\right), \quad 0\le n<N.$$

This differs slightly from the symmetric Hann expression with denominator $N-1$. The implementation and training pipeline must select the same convention. The distinction may be small acoustically yet significant in bit-exact feature tests.

## FFT and power spectrum

The discrete Fourier transform is

$$X_m[k]=\sum_{n=0}^{N-1}u_m[n]e^{-j2\pi kn/N},$$

and its raw power spectrum is

$$P_m[k]=|X_m[k]|^2=(\Re X_m[k])^2+(\Im X_m[k])^2.$$

For a real input signal, $X_m[N-k]=\overline{X_m[k]}$, so only $N/2+1$ nonredundant bins are needed: 257 bins for $N=512$. Real FFT libraries sometimes use packed formats that do not match a natural array of complex values. In particular, the DC and Nyquist bins may occupy special positions. A correct DSP pipeline must unpack the library's documented format rather than reinterpret memory as ordinary interleaved complex pairs.

FFT normalization differs by library: some use an unscaled forward transform, others divide by $N$ or distribute normalization across stages. Changing that convention shifts the log-energy features. An embedded implementation should explicitly note the convention and apply matching scale compensation, if required.

## Mel filter bank

The mel scale commonly maps frequency $f$ (Hz) to

$$m(f)=2595\log_{10}\left(1+\frac{f}{700}\right).$$

A bank of $B$ triangular filters $H_b[k]\ge0$ maps frequency-bin powers into bands:

$$E_m[b]=\sum_{k=0}^{N/2}H_b[k]P_m[k],\qquad b=0,\dots,B-1.$$

The triangles may be constructed by uniformly spacing vertices on the mel scale and mapping back to FFT frequencies. Choices about endpoint rounding, filter normalization, frequency range, and whether powers or magnitudes are accumulated all change the feature distribution. These are architectural parameters that must be kept in lockstep with model training.

A logarithmic feature is

$$F_m[b]=\ln(\max(E_m[b],\epsilon)),$$

where $\epsilon>0$ prevents $\ln(0)$. Other pipelines use $\log_{10}$, dB scaling, or normalized energy. One is not intrinsically the correct standard; the correct choice is the one specified by the trained model and validated by reference vectors.

### Why logs change the signal

The logarithm compresses the large dynamic range of spectral energy. Multiplying the underlying signal amplitude by $a$ approximately adds $2\ln|a|$ to every band with significant energy, before clipping and normalization. This makes amplitude behavior interpretable, but it does not eliminate microphone gain or background noise shifts. Training augmentation and empirical thresholds must account for those effects.

## Sliding context and timing

If a feature matrix contains $R$ rows and each row advances by $H$ samples, the *covered audio duration* is

$$T_{\rm context}=\frac{N+(R-1)H}{f_s}.$$

With $N=512$, $H=256$, $R=61$, and $f_s=16$ kHz, this gives $(512+60\cdot256)/16000=0.992$ s. Note that the feature rows are not 61 independent 32-ms clips: they overlap. A full context cannot be inferred from 61 times 32 ms.

For $B=20$ INT8 bands, the $61\times20$ feature matrix holds 1,220 bytes. That does not include PCM ring storage, FFT workspace, mel coefficients, scratch arrays, or operator activations. Full-system memory must be budgeted separately.

## Fixed-point choices in the DSP chain

PCM input may be INT16 while the FFT and mel bank use either floating point or fixed point. Cortex-M4F has hardware single-precision FPU support; this makes float DSP practical in many designs, though execution time and energy still require measurement. Fixed-point DSP can reduce cost on devices without suitable FPU support but introduces Q-format scaling, headroom, and saturation analysis.

For a signed $Q_{a.b}$ representation, one integer unit corresponds to $2^{-b}$ real units. Multiplying two fixed-point values produces $2b$ fractional bits before shifting back. If $x$ and $y$ have values $X\cdot2^{-b}$ and $Y\cdot2^{-b}$, then

$$xy=(XY)2^{-2b}.$$

Recovery to $Q_{a.b}$ requires controlled rounding of $XY/2^b$. FFT butterflies may need block scaling to avoid overflow. The result can be fast, but a careless fixed-point conversion may degrade class separation substantially.

## CMSIS-DSP implementation choices

CMSIS-DSP provides FFT, filtering, MFCC, matrix, and numerical kernels for Arm Cortex cores. An STM32 implementation can select the exact supported transform for its data type and CPU, then verify its packed output and scaling against a host oracle [CMSIS-DSP]. Do not change compiler optimization flags merely because a library suggests maximum speed: flags such as `-ffast-math` may change floating-point semantics. Apply optimizations selectively and validate parity, including denormals, NaNs, and boundary cases relevant to the application.

## A minimal streaming design

A practical feature pipeline keeps capture independent of inference:

```text
sensor interrupt or peripheral worker:
    publish complete PCM block to SPSC ring
    increment sample and overflow counters

DSP worker:
    while sufficient unprocessed samples for one hop:
        copy hop to local overlapping frame
        apply periodic window
        compute real FFT and unpack nonredundant bins
        power → mel triangles → natural log
        quantize using stored input scale/zero point
        append one feature row to context ring
    if full context and decision cadence reached:
        publish stable feature snapshot or invoke model
```

The callback must not perform an expensive FFT and should not block on UART output. If the DSP worker can fall behind, specify which samples are dropped, how gaps are detected, and what detector state is reset. A circular ring whose writer silently overwrites unread samples is particularly dangerous: resulting windows may be temporally discontinuous while appearing numerically plausible.

## Verification vectors

Build a reference vector corpus containing silence, DC offset, a sinusoid aligned with an FFT bin, a sinusoid between bins, clipped samples, Gaussian noise with a fixed seed, and representative recorded speech. Compare: PCM handoff, windowed samples, FFT bins (especially DC/Nyquist), power spectrum, mel energies, log values, and quantized feature bytes. A final argmax-only test cannot identify which stage is mismatched.

**Exercises**

1. Derive $T_{\rm context}$ for $R=49$, $N=400$, $H=160$, $f_s=16000$.
2. Explain how a periodic versus symmetric Hann window changes the first and last samples.
3. A feature extractor takes 2.5 ms per 16-ms hop. Is that alone sufficient to guarantee no ring overflow? Identify the missing scheduling assumptions.
