# CoreIR to LLVM Lowering Checklist

## 1. Requirement Analysis and Design Strategy
- [ ] Clarify goals and abstraction levels of CoreIR and LLVM.
- [ ] Compare data structures, type systems, and metadata.
- [ ] Determine information to carry over to LLVM IR (types, annotations, attributes).
- [ ] Consider data needed for optimization, debugging, and verification.

## 2. Define Basic Conversion Rules
- [ ] Map control flow (CoreIR flow representation → LLVM basic blocks).
- [ ] Map arithmetic and logic operations.
- [ ] Convert memory and register representations to LLVM memory operations.
- [ ] Represent I/O ports and external interfaces (e.g., functions or globals).

## 3. Organize CoreIR Components
- [ ] List special nodes or modules requiring support.
- [ ] Devise alternatives for elements not directly expressible in LLVM IR (custom instructions, inline assembly, metadata).

## 4. Detail Implementation Strategy
- [ ] Decide where in the CoreIR toolchain to implement the lowering pass.
- [ ] Design traversal algorithms and data structures.
- [ ] Specify error handling and performance considerations.

## 5. Design Tests
- [ ] Create sample CoreIR programs with expected LLVM IR outputs.
- [ ] Add edge-case tests (special data types, redundant modules, undefined behavior).
- [ ] Validate generated LLVM IR using LLVM tools (opt/llc).

## 6. Document the Lowering Process
- [ ] Write specification, conversion rules, and constraints.
- [ ] Record design issues and future extensions.

## 7. Plan Future Extensions and Optimization
- [ ] Evaluate LLVM optimization passes and code generation strategies.
- [ ] Ensure extensibility for new operations or types.
- [ ] Plan performance evaluation and collect real-world examples.
