# libFuzzer harnesses for JPEGView's untrusted-input decoders

These are **standalone [libFuzzer](https://llvm.org/docs/LibFuzzer.html) targets**
for the file-format decoders that parse untrusted, attacker-controlled bytes.
They drive the **real** decoder entry points (no reimplementation) under
AddressSanitizer so that out-of-bounds reads/writes, integer-overflow-driven
allocations, use-after-free and similar memory bugs surface as crashes with a
reproducer.

> **These files are intentionally NOT part of the MSVC solution
> (`src/JPEGView/JPEGView.sln`).** They live under `extras/fuzzing/` so they can
> never affect the normal Visual Studio build. They are compiled separately with
> clang. Nothing here is wired into the product, the installer, or any required
> CI check.

This is, by a wide margin, the **highest-value continuous-discovery tool for the
untrusted-file attack surface**: every target keeps mutating real inputs forever
and finds the corner cases that hand-written tests and one-off audits miss. The
companion GitHub workflow (`.github/workflows/fuzz.yml`) runs a short best-effort
pass on a schedule, but the real value comes from running these locally (or in a
dedicated fuzzing service like ClusterFuzzLite/OSS-Fuzz) for hours, with a corpus.

## Targets

| Target          | Decoder under test | Real entry point | Input shape |
|-----------------|--------------------|------------------|-------------|
| `fuzz_png.cpp`  | libpng/APNG (`PngReader`)   | `PngReader::ReadImage(...)` and `PngReader::GetEXIFBlock(...)` | memory buffer (raw file bytes) |
| `fuzz_exif.cpp` | EXIF/TIFF (`CEXIFReader`)    | `CEXIFReader::CEXIFReader(void*, EImageFormat)` | memory buffer (APP1 block) |
| `fuzz_tga.cpp`  | TGA (`CReaderTGA`)          | `CReaderTGA::ReadTgaImage(LPCTSTR, COLORREF, bool&)` | **file path** (harness writes a temp file) |
| `fuzz_bmp.cpp`  | BMP/DIB (`CReaderBMP`)      | `CReaderBMP::ReadBmpImage(LPCTSTR, bool&)` | **file path** (harness writes a temp file) |

### Why two targets take a file path, not a buffer

`CReaderTGA::ReadTgaImage` and `CReaderBMP::ReadBmpImage` read with
`_tfopen`/`fread` from a path rather than from a memory buffer. The harness
therefore writes each fuzz input to a temporary file and passes its path. This
adds a small amount of filesystem I/O per execution (slower than the in-memory
PNG/EXIF targets), but exercises the genuine production code path.

### Ownership / freeing (so ASan stays clean)

- `PngReader::ReadImage` returns a `malloc`'d pixel buffer and a `malloc`'d
  `exif_chunk` — the harness `free()`s both, then calls `PngReader::DeleteCache()`
  to release the internal libpng cache (animated PNGs keep it alive across frames).
- `PngReader::GetEXIFBlock` returns a `malloc`'d block — `free()`d.
- `CReaderTGA`/`CReaderBMP` return a heap `CJPEGImage*` — `delete`d.
- `CEXIFReader` takes no ownership of its input; its small `GPSCoordinate`
  allocations are released by its destructor. Construct + destruct is the full
  cycle, so the harness just lets the object go out of scope.

## Building (clang)

Exact flags depend heavily on your local setup — toolchain, where the SDK/ATL/WTL
headers live, and which prebuilt codec import libraries you have. **Treat the
commands below as a starting point, not a copy-paste recipe.** You will almost
certainly have to adjust include (`-I`) and link (`-L`/`.lib`) paths.

The general shape of a libFuzzer + ASan build is:

```
clang++ -g -O1 -fsanitize=address,fuzzer \
    -I<include dirs> \
    fuzz_<name>.cpp <decoder .cpp + its dependency .cpp files> \
    <import libs> \
    -o fuzz_<name>
```

On Windows, use the clang that ships with Visual Studio (or LLVM) so the MSVC
ABI, the Windows SDK, and the ATL/WTL headers are available; `clang-cl` with
`/fsanitize=address` plus the libFuzzer runtime is the most ABI-compatible option.

### `fuzz_png` (most self-contained)

The PNG decoder depends on libpng + zlib, both of which are vendored as prebuilt
libs in the tree:

- headers: `src/JPEGView/libpng-apng/include` (provides `png.h`)
- libs (x64): `src/JPEGView/libpng-apng/lib64/libpng16.lib`,
  `src/JPEGView/libpng-apng/lib64/zlib.lib` (use `lib/` for 32-bit)

Example skeleton (adjust paths):

```
clang++ -g -O1 -fsanitize=address,fuzzer -D_WIN64 \
    -I../../src/JPEGView \
    -I../../src/JPEGView/libpng-apng/include \
    fuzz_png.cpp ../../src/JPEGView/PNGWrapper.cpp \
    ../../src/JPEGView/libpng-apng/lib64/libpng16.lib \
    ../../src/JPEGView/libpng-apng/lib64/zlib.lib \
    -o fuzz_png
```

`PNGWrapper.cpp` includes `StdAfx.h`, which pulls in ATL/WTL. You may need to add
the Windows SDK and ATL/WTL include directories (`-I`), or provide a minimal
precompiled-header shim. The PNG path itself is buffer-based and does not need
`CJPEGImage`.

### `fuzz_exif`, `fuzz_tga`, `fuzz_bmp` (need ATL/WTL + CJPEGImage)

These decoders use `LPCTSTR`/`CString`/`COLORREF` and (for TGA/BMP)
`CJPEGImage`, so the build must put the Windows SDK and the ATL/WTL headers on
the include path, and link the decoder's dependency translation units. At
minimum:

- `fuzz_exif`: `EXIFReader.cpp` + `Helpers.cpp` (and their includes).
- `fuzz_tga` : `ReaderTGA.cpp` + `JPEGImage.cpp` + `BasicProcessing.cpp` +
  `Helpers.cpp` (+ whatever those pull in — `CJPEGImage` has a wide dependency
  fan-out into the GDI+/processing code).
- `fuzz_bmp` : `ReaderBMP.cpp` + `JPEGImage.cpp` + `BasicProcessing.cpp` +
  `Helpers.cpp` (same caveat).

Because `CJPEGImage` drags in much of the processing layer, the TGA/BMP targets
are the most involved to link. If you only want quick coverage, start with
`fuzz_png` and `fuzz_exif`. Expect to iterate on missing-symbol link errors by
adding the referenced `.cpp` files one at a time.

## Running

```
mkdir corpus
./fuzz_png corpus/ -max_total_time=600        # run 10 minutes
./fuzz_png corpus/ some_real.png              # seed with valid samples first
```

Seed each corpus with a handful of **valid** files of the matching format
(real PNGs, EXIF-bearing JPEG APP1 blocks, TGAs, BMPs). A good corpus is the
single biggest factor in how fast the fuzzer reaches deep code. On a crash,
libFuzzer writes a `crash-<hash>` reproducer; re-run the target on that file to
reproduce deterministically.

## Honesty / caveats

- **Not build-verified in this environment.** The harnesses were written against
  the real decoder signatures in `src/JPEGView/`, but the exact clang invocation
  (include dirs, ATL/WTL/SDK locations, import libs, PCH handling) was not
  compiled here and will need local tuning.
- The TGA/BMP targets do real temp-file I/O per execution, so they run slower
  than the in-memory PNG/EXIF targets.
- These are a discovery tool, not a regression gate. Do not make them a required
  CI check.
