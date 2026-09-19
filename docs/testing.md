# Testing and Verification

This project has no host C++ runtime and no emulator. The OpenOrbis cross-toolchain is used for compilation, while runtime verification requires the PS4.

## Verification layers

### 1. Cross-compile

A full clean build catches C++ type, syntax, compilation, and link errors.

A successful build does not establish that the application works correctly on the PS4.

### 2. Python verification

Scripts matching `app/tools/*_verify.py` provide Python mirrors of randomizer algorithms and validate their behavior against real game data.

List the available verifiers with:

```bash
ls app/tools/*_verify.py
```

Each verifier provides a `selftest` command. For example:

```bash
python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4
```

Reuse existing parsers when adding verification rather than duplicating file-format logic. For example, `treasure_verify` reuses `boss_verify`'s MSBB reader.

### Important limitation

A Python mirror verifies the rules implemented by the Python mirror. It does not prove that the C++ implementation follows those same rules.

The Python and C++ implementations can drift. Passing a Python verifier must therefore be reported as evidence about the algorithm, not as proof of the PS4 implementation.

### 3. Hardware testing

Hardware testing on the PS4 is the final verification of runtime behavior.

Claude cannot perform this test. When reporting status:

* Clean build + passing Python verification = **ready for hardware testing**
* Hardware testing is required before considering runtime behavior verified

## Test data

### Vanilla baseline

`data/vanilla/dvdroot_ps4`

This is the trusted vanilla game-data baseline used for verification and comparison.

### Hardware run outputs

`data/runs/`

Contains saved randomized output trees from previous hardware runs. Use these for comparison and regression investigation.

## Comparison methodology

When investigating randomization behavior, compare randomized output against the vanilla baseline as well as against other randomized runs.

Comparing two randomized runs alone is insufficient. Identical results across runs can have multiple explanations, including intentional exclusion, independent random selection, or the same result being selected by chance.

The vanilla baseline provides the reference needed to distinguish these cases.

A previous investigation that compared randomized runs without the vanilla baseline reported 221 false anomalies. Treat the vanilla comparison as a required part of this type of investigation.
