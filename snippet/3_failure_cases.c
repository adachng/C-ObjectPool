#include <ObjectPool.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_MALLOC_CALL_COUNTER 50U
static size_t malloc_call_counter = 0;

static void* my_malloc(const size_t size)
{
    // Simulate malloc() failure:
    if (malloc_call_counter >= MAX_MALLOC_CALL_COUNTER)
    {
        return NULL;
    }
    malloc_call_counter++;
    return malloc(size);
}

struct SomeStruct
{
    struct PoolSlot* slot_p;
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

    // Simulate user validation failure:
    const bool* const is_fail_p = arg_p;
    if (*is_fail_p)
    {
        free(ret_p);
        return NULL;
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
    const size_t pool_capacity = 2UL * MAX_MALLOC_CALL_COUNTER;

    struct ObjectPool* pool_p = NULL;

    // Scenario 1: malloc() failure, not user validation failure:
    {
        const struct ObjectPoolOptArgs args = {
            .on_acquire_cb = NULL,
            .on_release_cb = NULL,
            .c_malloc      = my_malloc,
            .c_free        = NULL,
        };

        bool is_fail = false;

        struct ObjectPool* const pool_p = ObjectPool_new(pool_capacity,
                                                         SomeStruct_new,
                                                         SomeStruct_free,
                                                         &is_fail,
                                                         &args);

        assert(pool_p == NULL);

        // Nothing to free here, but still safe to call without checking:
        ObjectPool_free(pool_p);
        ObjectPool_free(NULL); // equivalent
    }

    // Scenario 2: user validation failure:
    {
        bool is_fail = true;

        struct ObjectPool* const pool_p = ObjectPool_new(pool_capacity,
                                                         SomeStruct_new,
                                                         SomeStruct_free,
                                                         &is_fail,
                                                         NULL);

        assert(pool_p == NULL);

        // Nothing to free here, but still safe to call without checking:
        ObjectPool_free(pool_p);
        ObjectPool_free(NULL); // equivalent
    }

    assert(malloc_call_counter == MAX_MALLOC_CALL_COUNTER);

    // No memory leaks here.
    // No invalid reads and writes here.

    return 0;
}
