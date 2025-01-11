# Yogi
**Y**our **O**wn **G**ame **I**nnovator.
A Game engine based on the Hazel Engine by TheCherno.

## Features
- Cross-platform support (Windows, Linux, macOS)
- OpenGL and Vulkan support
- Lua scripting support

## Building
### Prerequisites
- CMake (version 3.10 or higher)
- A C++ compiler with C++17 support

### Instructions
```sh
mkdir build && cd build
cmake ..
make
```

## Project Structure
- **editor**: GUI editing tools
- **engine**: Core library
- **launcher**: Launch editor project
- **sandbox**: Example for directly using the core library