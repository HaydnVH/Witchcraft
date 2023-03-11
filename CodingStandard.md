# Language

The Witchcraft project is written in Modern C++, specifically C++ 20. [This page](https://en.cppreference.com/w/cpp/compiler_support) has tables describing compiler support for various C++ core and library features.  The compilers that we care about are MSVC and GCC, so any feature that is well-supported by both may be used, which is the majority of C++ 20.  The following notable exceptions remain a challenge for cross-platform development and should be avoided (for now):
- `std::format` isn't available on GCC until version 13, which has yet to release as of the time of this writing (2023-03-04).
- Modules only have partial support on GCC and still have issues on MSVC, and so should be avoided.

Features of modern C++ should be used liberally, such as `auto`, ranged-based `for` loops, lambda functions, and so on.  These features, once familiar to a programmer, make code easier to read, write, reason, and debug.

## Compilers & Build System

Witchcraft is built using CMake version 3.19 or newer.  This is the minimum version that supports `CMakePresets.json`, which is required for proper building.  Beyond the contents of this repository, the following installations are required:
- On Windows, install the latest version of Visual Studio 2022 and make sure development environmnents for C++ and CMake are enabled.  If you plan to cross-compile to Linux, ensure that the relevant package is enabled as well.
- On Linux, you'll need CMake version 3.19 or newer and GCC version 1.11 or newer.  This may be newer that what's available in your OS's standard app repository, and if so will need to be installed manually.  You may also need a standard installation of `gdb`, `ninja`, and `zip`.
- [Vulkan SDK](https://vulkan.lunarg.com/) is required.

## Included Dependencies

In the `dependencies` directory of this repository are a number of third-party libraries which have been built, tested, and included for usage here.  Only pre-built binary libraries and header files are present; see the [WitchcraftDependencies](https://github.com/HaydnVH/WitchcraftDependencies) repository for more information.  Each of these libraries has a license which is compatible with the Witchcraft project; see the `licenses` directrory for details.

# Naming Conventions

The following naming styles are used throughout the project:
- `cameCase`: Each word or abbreviation begins with a capital letter, no spaces or punctuation, and the first letter is lowercase.  For abbreviations, only the first letter is capitalized.
- `PascalCase`: Like camel case, except the first letter is uppercase.
- `snake_case`: Each word or compound is separated by an underscore.  Either all uppercase or all lowercase, depending on the situation.

## Variables

All variables, including member variables and function arguments, should be written in camel case.  Give descriptive names to variables whenever it makes sense to do so.  Private or protected member variables should be postfixed with an underscore.

## Constants

Local variables should always be declared as `const` and still written in camel case.

Global constants, whether in header files or translation units, should be written with `UPPERCASE_SNAKE_CASE`, matching the traditional syntax for `#define`'d constants.  These should be declared `constexpr` whenever possible.

## Global and Static Variables

Global variables should be written in camel case and given a `_g` suffix.

Static variables are global variables that either belong to a class, or are inaccessible outside of the translation unit they're defined in.  They should be declared in an anonymous namespace, and should be given a `_s` suffix.
```
int myGlobal_g = 0;
namespace {
	int myStatic_s = 0;
}
int MyClass::myClassStatic_s = 0;
```

## Operator Overload Operands

For binary operators, the operator should be declared `friend` and the operands should be named `lhs` and `rhs`.

## Type Names

Types and alises should be written in pascal case.
```
class MyClass {};
struct MyStruct {};
using MyAlias = MyClass;
```

## Enums

Enumerations should always be defined in some sensible order; when in doubt, sort alphabetically.  Enums should always be class enums, with the name and enumeration in pascal case.  For enums which contain bitfield values, values should be assigned explicitly using binary notation (`0b0001`).  Enums which are used beyond runtime (saved to disc etc) should have explicit values and should be in value order if that differs from alphabetical.
```
enum class MyBitfield {
	First  = 0b0001,
	Second = 0b0010,
	Third  = 0b0100,
	Fourth = 0b1000
};
```

## Function Names

Functions should be written in camel case and should describe its intended purpose as well as possible.

## Namespaces

Namespaces should be written in snake case, and should be as concise as possible.  Nested namespaces can be nested directly using `::`.
```
namespace parent_namespace::child_namespace {
	...
}
```

In headers, a namespace can scope all of the included definitions.  In translation units, namespace should scope only to the function being defined.
```
// in foo.h
namespace etc {
	void bar();
}
// in foo.cpp
void etc::bar() {
}
```

## Template Parameters

Single capital letters should be preferred for template parameters.  If there is only one template typename, `T` is the best choice.  When this would cause confusion, pascal case with a `T` suffix for a single typename or `Ts` for a variadic template, is acceptable.  Non-typename template parameters may use a single capitalized letter other than `T` or pascal case as appropriate.
```
template <typename T>
template <typename FirstT, RestTs...>
template <size_t I, Size>
```

# Iterator Support

Iterators are a key means of interfacing with any sort of collection-like data, and support for iterators should be provided whenever possible.  The bare minimum of iterator support, which enables ranged-based `for` loops, must cover the following:
- A `begin()` function which returns an iterator at the "beginning" of the collection.
- An `end()` function which returns an iterator "one past the end" of the collection.
- The iterator class must support the following:
  - `operator ==` (equality check)
  - `operator !=` (inequality check)
  - `operator ++` (prefix increment)
  - `operator ++(int)` (postfix increment)
  - `operator *` (reference access to current element)
  - `operator ->` (pointer access to current element)

Iterators need not be random access, or reversible, or even connected to a "container" in the traditional sense.  Ranged-based `for` loops should be supported whenever possible, and all this requires is that when the iterator is no longer valid, it is equal to `end()`.  Any iterator support beyond this is simply icing on the cake.

When the data you want to iterate is not a "container" in the traditional sense, it can be useful to create a "Proxy" class which represents a query or the result of some function.  This Proxy class doesn't need to provide any functionality other than `begin()` and `end()`, but may store data related to the query that created it.  The `Tokenizer` class (in `src/tools/strtoken.h`) is a good example of a proxy which provides non-trivial iterators that act on a non-traditional "collection".

Check out [this article](https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp) for some tips on writing your own iterator class.

# Error Handling

First, some definitions; There are 4 different types of results from an operation:
- *Success* means that the operation completed as expected.
- *Warning* means that something went wrong, but the operation still produced a usable result.
- *Error* mens that the operation failed, but the engine as a whole can continue running.
- A *Fatal Error* is an error so severe that the engine cannot continue running.

There are two ways to properly handle errors in C++: exceptions and return values.  Witchcraft uses both, as appropriate.

## Exceptions

Modern compilers use "zero-cost exceptions"; this means that code running in a `try` block has no performance overhead whatsoever, but throwing and catching exceptions is very expensive.  As a result, exceptions should only be used for *exceptional* circumstances.  The entirety of Witchcraft runs inside a `try` block in order to catch any exceptions that may not have been handled closer to the source.

In general, the rule of thumb is: only throw exceptions for fatal errors.  Non-fatal errors and warnings should be handled via a return mechanism.  Code in Witchcraft should be exception-aware, but reluctant to throw!

There will be times when a library capable of throwing exceptions must be interfaced with.  If the library provides `nothrow` versions of functions that might throw an exception, the `nothrow` function should always be preferred.  If no such function exists, then exceptions from the library should be caught as appropriate.  If the failure would cause a fatal error, then the exception may be left to be caught by the outer-level `try` block in main.

## Returns

When in doubt, returning a meaningful value should be preferred method of error handling.  This has no overhead and forces the caller to handle any unexpected behavior at the source.

A mechanism has been provided in `src/tools/result.h` which allows a `Result` object to be returned from any function.  `Result` may or may not contain a value, and may or may not contain a message, but can always indicate whether the result is a success, warning, or error.  A function should return like so:

```
return wc::Result::success(value)
// or
return wc::Result::warning("message", value);
// or
return wc::Result::error("message");
```

and should be called like so:

```
auto result = myFunction();
if (!result.isSuccess()) {
	// handle error appropriately
}
```