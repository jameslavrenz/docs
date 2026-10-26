# From Model Training to Firmware Release

## The reproducibility boundary

Training happens away from the MCU, usually on a workstation or service with ample compute. But the deployed model and its provenance must cross a controlled boundary. The deliverable is not just a `.tflite` or another binary file; it is a versioned set of inputs, arithmetic semantics, training choices, preprocessing code, and validation records.

At minimum, record:

- dataset versions, collection conditions, subject/device partitioning, and license;
- feature-extraction parameters and software version;
- model architecture, input/output tensor schemas, and operator set;
- training seed, optimizer/hyperparameters, and commit hash;
- quantization calibration data and resulting per-tensor/per-channel parameters;
- frozen model file hash and generated firmware array hash;
- numerical reference vectors and expected outputs;
- known limitations and operating-domain exclusions.

A mathematically good model that cannot be reproduced or audited is a poor embedded dependency.

## Training/test leakage in sensor systems

Randomly splitting overlapping audio windows or adjacent vibration snippets can put nearly identical observations into training and test sets. This inflates apparent accuracy. A stronger split separates subjects, recording sessions, devices, environments, or equipment units based on the expected deployment variation. Temporal overlap must be accounted for before partitioning.

Metrics should correspond to product use. An always-on wake-word detector cares about false activations per hour and miss rate at a defined operating point, not just frame classification accuracy. A bearing-fault monitor may care about false alarm rate per operating day and latency to detect a sustained fault. An imbalanced dataset makes overall accuracy especially misleading.

For a binary decision:

$$\operatorname{precision}=\frac{TP}{TP+FP},\quad
\operatorname{recall}=\frac{TP}{TP+FN},\quad
\operatorname{FPR}=\frac{FP}{FP+TN}.$$

These are defined only when denominators are nonzero. For streaming detection, count **events** over time rather than treating heavily overlapping frames as independent Bernoulli trials.

## Quantization-aware training versus post-training quantization

Post-training quantization (PTQ) maps a trained floating-point model to integer parameters using a calibration dataset. Quantization-aware training (QAT) introduces approximated quantization effects during training to help the model adapt. Neither is universally superior: QAT may recover accuracy for sensitive networks, but adds engineering and training complexity.

The conversion process must ensure every required operator can be executed on the target using the chosen integer backend. A graph that silently includes a float-only operator is not a fully integer MCU deployment. An inspection script should reject unsupported operator versions, dynamic input shapes, and unexpected FLOAT32 tensors before firmware code generation.

## Export and validation gates

A recommended gate pipeline is:

```text
train model on development machine
    ↓
freeze preprocessing and graph revision
    ↓
calibrate / quantize / export
    ↓
inspect operators, shapes, scales, zero points
    ↓
run host operator and graph parity tests
    ↓
compile model bytes into firmware image
    ↓
link-size and static-RAM checks
    ↓
run device inference parity corpus
    ↓
run streaming hardware tests and fault injection
    ↓
produce release manifest, logs and sign-off
```

The firmware may embed the model as a C array or load it from flash storage, provided authenticity/integrity checks and memory costs are understood. For an MCU without a filesystem, compiled-in model arrays are often the simplest option. If field-updatable models are required, the model format must be versioned and validated before the runtime accepts it.

## Model versioning and secure updates

A neural network can change application behavior without a line of application C++ changing. Treat model versions as software releases. A release manifest should carry model hash, preprocessing version, inference runtime version, compatible hardware, tested latency/memory bounds, and rollback procedure. Firmware/model mismatches should fail closed when incompatible.

Use cryptographic verification and secure boot/update mechanisms appropriate to the product threat model. A hash alone detects accidental change only if the expected hash is itself trusted. A signed artifact verifies provenance only when key management, signature checking, and rollback controls are designed correctly. These security topics are not developed into a full secure-boot design in this book.

## Model and detector tests must be separate

The model may score a keyword correctly while the application generates duplicate button events; conversely, a sound cooldown mechanism cannot repair a model that misses varied voices. Build distinct test layers:

- tensor/operator numerical parity;
- model classification on frozen datasets;
- end-to-end streaming detection with real audio;
- event policy under gaps and stale scores;
- sensor disconnection and restart;
- long-running soak and resource high-water measurements.

This separation makes failures diagnosable, reduces the temptation to change model thresholds to mask firmware race conditions, and supports controlled iterative improvement.

## Documentation as an engineering interface

Publish a compact model card that describes the intended input domain, training data limitations, classes, preprocessing, quantization, model dimensions, metrics, runtime version, and safety constraints. This is useful to firmware developers who did not participate in model training. Similarly, the firmware team should produce a hardware validation sheet with board firmware SHA, timing, memory, DMA/ring-loss counters, and fault behavior.

**Exercises**

1. Describe a leakage-free split for a multi-device vibration-monitor dataset.
2. Explain why 98% test-window accuracy does not directly predict false triggers per hour in a wake-word product.
3. Design a compatibility check preventing an outdated model from running against a newer preprocessing pipeline.
