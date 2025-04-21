# Contributing to SQLQueryBuilder

Thank you for considering contributing to SQLQueryBuilder! This modern C++ library aims to provide a type-safe, efficient SQL query builder with minimal heap allocations. We value your input and want to ensure your contributions align with the project's design philosophy.

---

## Table of Contents
1. [Getting Started](#getting-started)
2. [Coding Conventions](#coding-conventions)
3. [Development Environment](#development-environment)
4. [Testing Guidelines](#testing-guidelines)
5. [Submitting Contributions](#submitting-contributions)
6. [Code of Conduct](#code-of-conduct)

---

## Getting Started

1. **Fork the Repository**  
   Begin by forking this repository and cloning your fork locally.

2. **Create a Branch**  
   Use a descriptive branch name following the format `feature/description` or `fix/issue-description`.

3. **Stay Up-to-Date**  
   Regularly pull changes from the main branch to avoid merge conflicts.

---

## Coding Conventions

SQLQueryBuilder follows modern C++ best practices with a focus on performance and type safety:

- Use **C++17/20 features** including [concepts](https://en.cppreference.com/w/cpp/language/constraints), [`std::string_view`](https://en.cppreference.com/w/cpp/string/basic_string_view), and [`std::span`](https://en.cppreference.com/w/cpp/container/span)
- Maintain a [zero-heap allocation](https://www.youtube.com/watch?v=LIb3L4vKZ7U) philosophy where possible
- Follow naming conventions aligned with [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html):
  - `camelCase` for methods and variables
  - `snake_case` with trailing underscores for private members (`name_`)
  - `PascalCase` for class names
- Apply [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) for:
  - [Strong typing](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-type)
  - [Const correctness](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#con-constants-and-immutability)
  - Use the `[[nodiscard]]` attribute for methods that produce values meant to be used
- String handling guidelines:
  - Prefer `std::string_view` for non-owning references following [abseil's string_view guide](https://abseil.io/tips/1)
  - Use `std::string` when string ownership is required for persistence
  - Be extremely careful about string ownership when constructing complex conditions
- Use the `inline namespace v1` pattern for API versioning
- Document all public APIs with [Doxygen-style](https://www.doxygen.nl/manual/docblocks.html) comments
- Include examples for complex functionality

---

## Development Environment

- **Compiler**: Use a modern C++ compiler supporting C++17 or later
  - GCC 9+
  - Clang 10+
  - MSVC 2019+ with `/std:c++17` or higher
- **Build System**: CMake 3.14 or newer
- **Optional Dependencies**:
  - Qt 5.15+ or Qt 6.x for Qt integration features
  - Catch2 for testing

Configure your environment with:
```bash
mkdir build && cd build
cmake .. -DBUILD_TESTING=ON
cmake --build .
```

---

## Testing Guidelines

1. **Write Unit Tests**: All new features should include corresponding unit tests using Catch2
2. **Test Template Instantiations**: Ensure your tests cover various template instantiations
3. **Performance Testing**: Include benchmarks for performance-critical code paths
4. **String Ownership**: Add specific tests verifying correct string ownership management
5. **Test All Placeholder Styles**: Ensure all placeholder types work correctly

Run tests with:
```bash
cd build
ctest -V
```

---

## Submitting Contributions

1. **Open an Issue First**  
   Discuss significant changes before implementation to ensure alignment with project goals.

2. **Make Atomic Commits**  
   Each commit should represent a single logical change with a clear message.

3. **Pull Request Process**
   - Reference any related issues
   - Include a detailed description of changes
   - Ensure all tests pass
   - Verify code follows all conventions

4. **Code Review**  
   Be responsive to feedback and be prepared to make requested changes.

---

## Code of Conduct

We expect all contributors to be respectful and collaborative. Harassment or disrespectful behavior will not be tolerated. We aim to foster an inclusive community where everyone feels welcome to contribute.

---

Thank you for contributing to SQLQueryBuilder! If you have questions, please open a discussion or contact the maintainers directly.
