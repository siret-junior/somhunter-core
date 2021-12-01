

# Naming

## **File names**
Filenames should be all **lowercase** and can include dashes (-) and underscores (_). Prefer dashes (-).

## **Type names**
Use **capitalized camel case**.
```cpp
// classes and structs
class UrlTable { ...
class UrlTableTester { ...
struct UrlTableProperties { ...

// typedefs
typedef hash_map<UrlTableProperties *, std::string> PropertiesMap;

// using aliases
using PropertiesMap = hash_map<UrlTableProperties *, std::string>;

// enums
enum class UrlTableError { ...
```

## **Variable names, class & struct data members**
Use **lowercase with underscores**. If the variable is boolean, prefix it with "be" verb to make it clear it's a `bool`. Prefix pointers with `p_`.
```cpp
double size_of_whatever;
bool is_ready;
bool should_end;
char* p_name;
```

Moreover if **private/protected data member**, prefix with `_`.

```cpp
class ExampleClass {
protected:
    std::size _protected_count;

private:
    std::size _size;
    bool _something_else;
}
```

## **Constant names**
Use **uppercase with underscores**.
```cpp
constexpr std::size_t BIN_COUNT = 100l;
``` 

## **Function names**
Use **lowercase with underscores**.
```cpp
get_topn_display();
apply_filters();
```

### **Getters & setters**
Use style like standard library. For example for field `std::size_t _size` create getter like `std::size_t size() const;` and getter `void size(std::size_t x);`. Omit any "get_" or "set_" prefixes.

```cpp
...
std::size_t size() const { return _size; }
void size(std::size_t x) { _size = x; }
...
```

## **Namespace names**
Name them like **type names**.

```cpp
namespace sh {
...
}; // namespace sh
``` 


## **Enumerator names**
Name them like **constants** - uppercase with _.

```cpp
enum class SomeEnum {
    NONE,
    OK
};
``` 

# **Header files**

## **The #define guard**
Use capitalized class name or source file name postfixed with `_H_`. Do not use pragma.
```cpp
#ifndef SOME_CLASS_H_
#define SOME_CLASS_H_
...
#endif  // SOME_CLASS_H_

// OR
#ifndef SOME_SOURCE_H_
#define SOME_SOURCE_H_
...
#endif  // SOME_SOURCE_H_
```

## **Names and order of includes**
Include headers in the following order: 
Related header, C system headers (with angle brackets), C++ standard library headers (with angle brackets), other libraries' headers (with angle brackets), your project's headers (with qotes).

```cpp
#include <stdio.h>

#include <filesystem>
#include <vector>

#include <nlohmann/json.hpp>

#include "canvas-query-ranker.h"
#include "common.h"
```


# Classes & struct
## **Structs vs. classes**
Use a struct only for passive objects that carry data; everything else is a class.

## **Class sctructure**
[Based on Google style](https://google.github.io/styleguide/cppguide.html#Declaration_Order) \
Within each section, prefer grouping similar kinds of declarations together, and prefer the following order: types and type aliases (typedef, using, enum, nested structs and classes), static constants, factory functions, constructors and assignment operators, destructor, all other member and friend functions, data members

```cpp
namespace sh {

class ExampleClass {
public:
	// << Types & typedeft

	// << Constants

	// << The big 5
	ExampleClass() {}
	ExampleClass(const ExampleClass& other) {}
	ExampleClass(ExampleClass&& other) {}
	ExampleClass& operator==(const ExampleClass& other) {}
	ExampleClass& operator==(ExampleClass&& other) {}
	~ExampleClass() {}

	// << Other methods

	// << Data members

protected:
	// << Types & typedeft

	// << Constants

	// << The big 5
	ExampleClass(int x) {}

	// << Other methods

	// << Data members

private:
    // << Types & typedeft

	// << Constants

	// << The big 5
	ExampleClass(double x) {}

	// << Other methods

	// << Data members

}
};  // namespace sh
```

# Comments
Comment code according to [Doxygen](https://www.doxygen.nl/manual/docblocks.html) documentation using Javadoc style (`/**`). 

```cpp
/**
 *	Generates the new debug targets.
 *
 * \param   x   Input variable.
 * \returns     Number of new targets.
 */
int generate_new_targets(float x);

/** This holds something. */
std::size_t _something;

```

# Formatting
As defined in `.clang-format` for clang-format.

Based on Google with some alterations:
- Line length 120
- Use tabs
- Indent PP directives after hash
- Preserve include blocks, just sort them
