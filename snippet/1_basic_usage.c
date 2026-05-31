#include <c_objectpool/ObjectPool.h>

#include <assert.h>
#include <stdlib.h>

struct SomeStruct
{
    struct PoolSlot* slot_p;

    int i;
    int j;
};

struct ExampleArg
{
    size_t counter;
};

static void* SomeStruct_new(void*            arg_p,
                            struct PoolSlot* slot_p)
{
    struct SomeStruct* const ret_p = malloc(sizeof(struct SomeStruct));
    if (ret_p == NULL)
    {
        return NULL;
    }

    ret_p->slot_p = slot_p;

    // Example of using the argument passed into this:
    {
        struct ExampleArg* const tmp_p = arg_p;
        tmp_p->counter++;
    }

    return ret_p;
}

static void SomeStruct_free(void* self_p)
{
    return free(self_p);
}

static void SomeStruct_release(struct SomeStruct* self_p)
{
    return PoolSlot_release(self_p->slot_p);
}

int main()
{
    const size_t pool_capacity = 100U;

    // Extra argument example:
    struct ExampleArg arg = {.counter = 123};

    struct ObjectPool* const pool_p =
        ObjectPool_new(pool_capacity,
                       SomeStruct_new,
                       SomeStruct_free,
                       &arg, // argument passed into callback
                       NULL);

    // In the user-defined SomeStruct_new(), the arg_p parameter
    // is casted and its counter is incremented:
    assert(arg.counter == 123 + pool_capacity);

    // NOTE: the argument passed into the callback can be used as both input and
    // output for the user-defined initialisation logic.

    // Scenario 1: object is released into pool before freeing pool:
    {
        // Acquire an object from the pool:
        struct SomeStruct* const obj1_p = ObjectPool_acquire(pool_p);
        assert(obj1_p != NULL); // only NULL when the pool is exhausted

        // Do something with the user-defined object that is pooled:
        obj1_p->i = 1;
        obj1_p->j = 2;

        // Done with the pooled object, release back into the pool:
        SomeStruct_release(obj1_p);
        // CAUTION: do not use SomeStruct_free(), as it will leak the PoolSlot!

        // The next time an object is acquired, it returns the same object:
        struct SomeStruct* const obj2_p = ObjectPool_acquire(pool_p);
        assert(obj1_p == obj2_p);
        SomeStruct_release(obj2_p); // identical to SomeStruct_release(obj1_p)
    }

    // Scenario 2: the pool is freed before the object:
    struct SomeStruct* const obj1_p = ObjectPool_acquire(pool_p);
    assert(obj1_p != NULL);

    // Pool is freed, but the pooled object is still usable:
    ObjectPool_free(pool_p);

    // Still fine to do something with the user-defined object that is pooled:
    obj1_p->i = 3;
    obj1_p->j = 4;

    // The library knows that the pool is freed through a flyweight:
    SomeStruct_release(obj1_p);

    // No memory leaks here.
    // No invalid reads and writes here.

    return 0;
}
