# `C-ObjectPool`

`C-ObjectPool` is a small and portable C library that manages a fixed-size pool of
user-defined objects without any `malloc()` call past the pool's initialisation phase.

This library requires user-provided function callbacks that manage the lifecycle of a user-defined object,
and in turn shifts all `malloc()` calls to take place during the object pool's initialisation.

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
    - [Advanced Usage](#advanced-usage)
- [External Dependencies](#external-dependencies)
    - [GoogleTest](#googletest-v1-17-0)
        - [Usage](#usage-1)

## Installation

The library is compatible with CMake. It is recommended to include this library as
a [Git submodule](https://git-scm.com/docs/git-submodule).

```sh
git submodule add https://github.com/adachng/C-ObjectPool.git cobjectpool
```

Note that `cobjectpool` is a directory name and can be specified as another name.

In the `CMakeLists.txt` at the parent to `cobjectpool` directory:

```cmake
add_subdirectory(cobjectpool)
```

This adds the CMake target library `c_objectpool` into your project.
To use this library in your executable:

```cmake
add_executable(my_target my_source.c)

target_link_libraries(my_target PRIVATE c_objectpool)
```

## Usage

`C-ObjectPool` is intended for object-oriented C/C++ codebases,
and requires some knowledge of manual memory management to use
(as with the vast majority of object-oriented C codebases).

The lifecycle of an object in C without object pooling is as such:

```c
struct SomeStruct
{
    // some useful data that the library user needs
};

struct SomeStruct* SomeStruct_new(void);         // returns some form of malloc()'s return
void SomeStruct_free(struct SomeStruct* self_p); // does some form of free()
```

```c
// Allocation:
struct SomeStruct* obj1_p = SomeStruct_new(); // likely malloc() underneath

// Usage:
// do some stuff with obj1_p...

// Deallocation:
SomeStruct_free(obj1_p); // likely free() underneath
obj1_p = NULL;
```

With `C-ObjectPool`, the lifecycle becomes:

```c
#include <c_objectpool/ObjectPool.h>

// Allocation of the object pool:
struct ObjectPool* pool_p = ObjectPool_new(100,             // pool capacity
                                           SomeStruct_new2, // modified allocation function
                                           SomeStruct_free, // identical deallocation function
                                           NULL,            // argument passed into SomeStruct_new2()
                                           NULL);           // optional arguments for advanced usage

// This may fail upon system OOM or user-defined SomeStruct_new2() fails (decided by user):
assert(pool_p != NULL);

assert(ObjectPool_get_capacity(pool_p) == 100);
assert(ObjectPool_get_size(pool_p) == 100);

// SomeStruct_new2() is called 100 times with all instances allocated within the pool:
struct SomeStruct* instances_arr[100];
for(size_t i = 0; i < 100; i++)
{
    arr[i] = ObjectPool_acquire(pool_p);
    assert(arr[i] != NULL); // if pool_p is not NULL then this is always true
}

// The object pool is exhausted while instances_arr is full:
assert(ObjectPool_acquire() == NULL); // returns NULL since it is exhausted

assert(ObjectPool_get_capacity(pool_p) == 100);
assert(ObjectPool_get_size(pool_p) == 0);

// Release all instances and forget about instances_arr:
for(size_t i = 0; i < 100; i++)
{
    SomeStruct_release(instances_arr[i]);
    instances_arr[i] = NULL;

    assert(ObjectPool_get_size(pool_p) == i + 1);
}

assert(ObjectPool_get_capacity(pool_p) == 100);
assert(ObjectPool_get_size(pool_p) == 100);

// The object pool is now full at capacity again:
struct SomeStruct* obj1_p = ObjectPool_acquire(pool_p);
// do some stuff with obj1_p...

// Deallocation phase:
ObjectPool_free(pool_p);
SomeStruct_release(obj1_p); // the library allows release after freeing the pool

// No memory leak at this point.
```

`C-ObjectPool` requires client code to store 1 more member in the user-defined pooled object,
which is the `struct PoolSlot*` instance. This is used to release the object back into the pool.

Here is a more concrete example of the `SomeStruct` definitions, including the
"original" definitions not intended for object pooling:

```c
#include <c_objectpool/ObjectPool.h>

struct SomeStruct
{
    // The library user needs to add this:
    struct PoolSlot* slot_p;

    // some useful data that the library user needs
};

// Already defined without object pooling:
struct SomeStruct* SomeStruct_new(void)
{
    struct SomeStruct* const ret_p = malloc(sizeof(struct SomeStruct));
    if(ret_p == NULL)
    {
        return NULL;
    }

    return ret_p;
}

// Already defined without object pooling:
void SomeStruct_free(struct SomeStruct* self_p)
{
    return free(self_p);
}

// The pooled object needs to store the PoolSlot instance:
void* SomeStruct_new2(void* arg_p, struct PoolSlot* pool_p)
{
    void* ret_p = SomeStruct_new();
    if(ret_p == NULL)
    {
        return NULL;
    }

    assert(pool_p != NULL); // this is guaranteed by the library

    ret_p->pool_p = pool_p; // this is provided by the library
    return ret_p;
}

// PoolSlot is used for proper object lifecycle with no memory leaks:
void SomeStruct_release(struct SomeStruct* self_p)
{
    if(self_p != NULL)
    {
        return PoolSlot_release(self_p->slot_p); // function provided by library
    }
}
```

In short:

1. Implement a `new()` function and store the `PoolSlot` provided by the library.
    - Never directly call this function.
    - The only appropriate usage of this function is to specify this as a callback
during the object pool creation.
2. Implement a `free()` function that directly frees the object as if it is not pooled.
    - Never directly call this function.
    - The only appropriate usage of this function is to specify this as a callback
during the object pool creation.
3. Implement a `release()` function to release/deallocate the pooled object.
    - Always directly call this function on objects that are acquired from the pool
to ensure all pooled objects that are acquired from the object pool are released after use.
    - The library will deallocate the pooled object upon 2 scenarios:
the pooled object is released into a freed pool,
and the pool being freed while the object is inside the pool.
4. When everything is done, free the object pool and release all acquired objects from the pool
(in no particular order).

#### Advanced Usage

`C-ObjectPool` optional features include:

- Accepting replacements for `malloc()` and `free()`.
- Additional event callbacks triggered upon acquiring and
releasing the object.
    - The release callback is invoked before the actual release/deallocation.

See [snippets](snippet) for more examples.

See the [header file](src/c_objectpool/ObjectPool.h) for the complete API reference.

## External Dependencies

### [GoogleTest](https://github.com/google/googletest) ([v1.17.0](https://github.com/google/googletest/releases/tag/v1.17.0))

#### Usage

Testing framework. Required only if this library is a top-level project (not used as a library).

Note that running the unit test with Valgrind's [memcheck](https://valgrind.org/docs/manual/mc-manual.html) requires
increasing the `--max-stackframe` value.
