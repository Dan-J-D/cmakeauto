# CMakeAuto
## A tool to simplify CMake Toolchain Commands

### Usage:
```bash
Usage: cmakeauto (help|build) -b <build path> -s <source path> [-g <Generator>] [-m <Mode>] [-a <Architecture>] [-ei <Extra Init Params>] [-eb <Extra Build Params>] [-ar]
	help: print help message
	build: build project

Parameters:
	-b <build path>: (Default: build) This specifies where the project would be built.
	-s <source path>: (Default: src) This specifies where the project source is.
	-g <Generator Name>: (Optional) This specifies the generator to be used.
	-m: (Default: release) This specifies the build mode to be Debug.
		debug: Debug mode
		release: Release mode
	-a: (Default: x64) This specifies the architecture to be built.
		x86: 32-bit
		x64: 64-bit
	-ei: (Optional) This specifies extra init params to be passed to cmake.
	-eb: (Optional) This specifies extra build params to be passed to cmake.
	-ar: (Optional) This specifies whether to auto reload the project when source files changed.
```

### Build with CMake For Release X64:
```bash
cmake -S . -B build -A x64
cmake --build build --config Release
```