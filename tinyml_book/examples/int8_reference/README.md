# Tested educational INT8 primitives

This C++20 library contains `requantize`, `dense`, and `conv1d` with no heap allocation or I/O inside kernel execution. It accepts signed INT8 activations and symmetric INT8 weights and uses INT32 biases / accumulation checks and INT64 arithmetic intermediates. The explicit multiplier convention is `numerator / 2^shift`, with ties rounded away from zero, NOT a claim of TFLM or CMSIS-NN bit-exact behavior.

The example `demo.cpp` prints `[5, -2, 4]`; automated tests cover fixed outputs, zero points, padding, shape mismatch, invalid scale, overflow, and 100,000 seeded numeric comparisons. These are host tests, not a board timing test or model-accuracy test. The code does not load an actual trained model.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```
