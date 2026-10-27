# A Complete Tested Two-Layer Integer Classifier


![Complete fixed-weight, two-layer inference example.](figures/graph-new.png){width=4.45in}

## Bridging the gap between operators and inference

The companion program `tinyml_classifier_demo` executes an entire *fixed-weight* two-layer classifier, including feature quantization, two dense layers, ReLU, and argmax. The dataset and coefficients are deliberately tiny and hand-checkable. This is a **reproducible inference demonstration**, not a trained, statistically validated product model. In particular, the weights below were chosen by hand. The distinction prevents an educational example from being mistaken for proof of trained-model accuracy.

Let the two real-valued input features be $f_0,f_1$ with feature scale $s_x=0.25$ and zero point zero. Quantization is $q_i=\operatorname{clip}_{[-128,127]}(\operatorname{round}(4f_i))$. Here test features are exactly representable: $(f_0,f_1)=(1,0.5)$ becomes $(4,2)$. The first layer has two outputs, weights

$$W_1=\begin{bmatrix}1&1\\1&-1\end{bmatrix},\qquad b_1=(0,0)^T,$$

and integer multiplier $1/2$ with ties away from zero, followed by ReLU. Thus $a=(3,1)$. The second layer has weights

$$W_2=\begin{bmatrix}1&-1\\-1&1\end{bmatrix},\qquad b_2=(0,0)^T,$$

with unity requantization, producing logits $(2,-2)$. The winning class is index 0. Input $(-1,0.5)$ becomes $(-4,2)$; the hidden layer clips negative activations and the result is independently asserted in host tests. Run `ctest` to verify both vectors. These exact integer logits, not a probabilistic claim, are the contract.

## Why this small example is still useful

A complete inference program must specify what each tensor *means*, not simply connect operator pointers. The demo allocates every tensor in fixed-size `std::array` storage, does not allocate in the inference path, checks both operator statuses, and explicitly rejects failure before class selection. This illustrates how an MCU firmware component can expose a small deterministic API without a general interpreter or file parser.

It is not equivalent to a trained wake-word network. For that next step, first freeze a real model's weights and every per-channel quantization parameter, export golden input/intermediate/output tensors from the production runtime, and prove parity at each layer on both host and target. Then compare detection error rates on held-out, field-representative recordings. The example's pedagogical rational requantizer intentionally differs from CMSIS-NN's exact fixed-point conventions; do not substitute kernels without numerical comparison.

## Reproducibility contract

From the book root run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/tinyml_classifier_demo
```

The test suite asserts two end-to-end classifications and intermediate results, alongside the existing arithmetic tests. This proves deterministic behavior for the specified inputs on the tested host; it proves neither generalization nor an STM32 timing bound. See the accompanying `examples/full_classifier/` source for the exact constants and the upgrade path to a trained, quantized model.
