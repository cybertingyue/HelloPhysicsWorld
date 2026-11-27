# HelloPhysicsWorld

## Prerequisites

Before you start development, follow the instructions for your operating system to install the required tools and libraries.

### Linux (Ubuntu)

Install the compiler, build tools, and development packages for OpenGL (`libgl-dev`) and GLFW (`libglfw3-dev`).

```sh
sudo apt update
sudo apt install build-essential libgl-dev libglfw3-dev
```

Check your CMake version with `cmake --version`. Ubuntu 24.04 ships CMake 3.28. If you need a newer version, download the stable Linux `.tar.gz` for your architecture from the [CMake download page](https://cmake.org/download/), extract it, and add its `bin` directory to `PATH`. 

### macOS

The macOS SDK supplied with Xcode or the Xcode Command Line Tools includes OpenGL. If you need the compiler and SDK, install the [Xcode Command Line Tools](https://developer.apple.com/documentation/xcode/installing-the-command-line-tools/) with:

```sh
xcode-select --install
```

Install [GLFW](https://formulae.brew.sh/formula/glfw) and [CMake](https://formulae.brew.sh/formula/cmake) with Homebrew:

```sh
brew install glfw cmake
```
