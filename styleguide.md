**Language**
- Code is written in Modern C++17.
- Don't use features from C++20 unless that feature is available in both Visual Studio and GCC.
- Do not use exceptions.  Disable them whenever possible.
- Runtime error checking should be wrapped in `#ifdef DEBUG/#endif` blocks unless the error could have been caused by user input, bad data, or system/hardware failures.
- Errors should be checked as close to the user/data as possible, and never more than once.
- Use `static_assert` to check for compile-time errors.
- Use `auto` when reasonable, especially for iterator types.
- Use ranged-based `for` loops whenever possible.
- Any globally-accessible functions or variables should be inside a namespace or static class member.
- Never use `long` - its size depends on platform/architecture.
- Use `nullptr` instead of `NULL`.
- Never use `using namespace` in .h files (.cpp files are fine, even `std`).

**Strings**
- All internal and data strings are UTF-8.
- The only time a non-UTF-8 string should ever be used is when explicitly interfacing with Win32 or performing a per-character loop.
- Don't use `std::u8string`; assume that add `std::string` objects are UTF-8.
- String literals which contain unicode should start with the `u8` prefix to ensure it's endoded as UTF-8.
- An array of `char` is just bytes; use `char8_t` to denote a c-style string.
- Use `std::string_view` if you need a read-only string parameter so you have full string functionality and never allocate/copy data.

**Running code outside main**
- In C++, the constructor of a global variable will run before `main`.  This behavior should be embraced, carefully.
- Code which will be used by other code during static initialization should be put inside the constructor of a static variable inside a function; this is the Initialization On First Use paradigm.
- Code which should "call itself" should be in the constructor of an ordinary global variable, so long as this code is never called by other static initialization code.
- ALWAYS be aware that the initialization order of ordinary global variables is UNDEFINED across compilation units! If this matters, wrap the variable in a function!

**Other*
- Avoid short-lifetime objects that allocate resources, such as `std::string` or `std::vector`.
- Either re-use these objects across multiple frames/iterationr or use stack-allocated arrays.

**Style**
- Math pseudo-primitive classes/structs should be all lowercase and as short as possible, ie: `float3`, `mat4`, `quat`, `half`.
- All other classes/structs should use capitalized camel-case, as in `ClassName`.
- Functions and method names should use uncapitalized camel-case, as in `funcName`.
- Constants should be all-caps with underscores, as in `MY_CONST`.
- Regular variable names should use underscores, as in `var_name`.
- variable names should be descriptive but as short as possible.
- Single-letter variables should only ever be used for co-ordinates (`x`, `y`, `x`) or iterators (`i`, `j`, `k`).
- Variables and data should be allocated statically or on the stack whenever possible.
- Variables should be scoped as tightly as possible; use anonymous `{}` blocks to explicitly tighten variable scope.
- Interfaces should be as neat and tidy as possible, even if that means a more complex implementation.
- Function parameters should pass objects by reference unless the object has fewer than 32 bytes with no resource allocation.
- Non-const references in function parameters imply that the parameter will be written to, and should be commented and named appropriately.
- Only use pointers in function parameters is `nullptr` is a valid input.
- Classes which will never have more than one instance should be converted into a namespace, a static instanceless class, or a singleton (in decreasing order of preference).
- Namespaces should be lowercase and as short as possible, such as 'namespace'.
- Static instanceless classes should be treated like namespaces, and commented appropriately.
- Use anonymous namespaces in `.cpp` files to protect static variables and "private" functions.
- Write template functions/classes if you expect to need multiple versions of that function or class.
- Function and variable names may start with `_` if they need to be in a publicly-visible space but should never be used as part of the interface.
- Whenever possible, avoid having a function return an `std::vector`.  If the intention is to iterate over a list, have a function which maintains state so subsequent calls can return the next entry (similar to `strtok`).  If a vector must be returned, it should do so as a non-const reference parameter.

**Braces**
```
auto regularFunction() {
	// ...
	// ...
	// ...
}

auto veryShortFunction() { /* ... */ }

auto fairlyShortFunction()
	{ /* ... ... ... */ }

auto functionThatDoesNothing() {}

if (condition) {
	// ...
}
else {
	// ...
}

switch (condition) {
	case FIRST:
		// ...
		break;
	case SECOND: {
		// ...
		// ...
	}	break;
	case THIRD: // deliberate overflow
	case FOURTH:
		// ...
		break;
	default:
		// ...
		break;
}
```