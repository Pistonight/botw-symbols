# Glue API
Generated headers that declare the raw address of member functions (or functions that look like member functions) as offset from main

## Why do I need address of member functions?
If you need to hook a member function, you need its address. However, member functions are special because they have an implicit argument `this`. The compiler will complain when you try to convert a member function pointer to a static function pointer.

```c++
class their::Class {
void* member_func(void*);    
}

void my_hook(their::Class* p_this) {
    // do stuff
}

void init_hooks() {
    replace_ptr(
        reinterpret_cast<uintptr_t>(their::Class::member_func), // compiler error
        my_hook
    )
}
```

## How does glue fix this?
Glue generates symbols that look like the original function, but instead they are static functions.

```c++
#include <glue/their/Class.h> // defines symbols and the mainoff macro

class their::Class {
void* member_func(void*);    
}

void my_hook(their::Class* p_this) {
    // do stuff
}

void init_hooks() {
    replace_at_offset(
        mainoff(their::Class::member_func), // no error!
        my_hook
    )
}
```

Under the hook, glue declares the symbol in another namespace and paste it in the mainoff macro

```c++
// glue/their/Class.h
#define mainoff(sym) _botw_link_glue_##sym
namespace mainoff(their::Class) {
    static ptrdiff_t member_func = 0x12340; // the raw address
}
```

## How to use
1. Make sure `path/to/botw-link/include` is in the include path of your makefile or other build system
1. Run the program with the top level namespaces of the functions you want.
   ```
   python path/to/botw-link/glue.py namespace [ namespace ... ]
   ```
   For example running
   ```
   python tools/botw-link/glue.py uking ksys
   ```
   Will generate glue for all `uking::...` and `ksys::...` functions
1. Include the headers in your code. They will work for both 1.5.0 and 1.6.0

The headers are organized by namespaces. For example:

This includes glue for `my::good::Class` class
```c++
#include <glue/my/good/Class.h>
```

This include glue for every class in `my::good` namespace
```c++
#include <glue/my/good.h>
```

This include glue for everything in `my` namespace
```c++
#include <glue/my.h>
```

This include everything glue generated (don't do this, build will be slow)
```c++
#include <glue/all.hpp> // .hpp so we don't conflict with the `all` namespace
```

## Overloads
If a function is overloaded, all the overloads will be numbered. Look at the comment which says the demangled symbol and pick the one you want

```c++
// glue/their/Class.h
#define mainoff(sym) _botw_link_glue_##sym
namespace mainoff(their::Class) {
static ptrdiff_t member_func_1 = 0x123450;
//               ^ their::Class::member_func(type1&)
static ptrdiff_t member_func_2 = 0x123460;
//               ^ their::Class::member_func(type2&)
}
```