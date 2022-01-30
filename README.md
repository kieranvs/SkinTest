# Skin Test

## Development Environment setup:
1. Clone repository with submodules.
	1. `git clone --recurse-submodules`
	1. Or after cloning `git submodule update --init`
1. `mkdir build`
1. `cd build`
1. `cmake ..`
1. Build:
	1. Linux: `make -j8` (replace '8' with number of threads)
	1. Windows: Open SkinTest.sln in Visual Studio
