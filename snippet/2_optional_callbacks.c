#include <c_objectpool/ObjectPool.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static size_t malloc_sz_counter   = 0;
static size_t malloc_call_counter = 0;
static size_t free_call_counter   = 0;

static void* my_malloc(const size_t size)
{
    malloc_call_counter++;
    malloc_sz_counter += size;
    void* const ret_p  = malloc(size);
    printf("malloc(%zu) returning %p\n", size, ret_p);
    return ret_p;
}

static void my_free(void* const ptr)
{
    free_call_counter++;
    printf("freeing %p\n", ptr);
    return free(ptr);
}

struct SomeStruct
{
    struct PoolSlot* slot_p;
};

static void SomeStruct_acquire_cb(void* const self_p)
{
    printf("This is called on the object when running ObjectPool_acquire()\n");
}

static void SomeStruct_release_cb(void* const self_p)
{
    printf("This is called on the object when running PoolSlot_release()\n");
}

static void* SomeStruct_new(void*            arg_p,
                            struct PoolSlot* slot_p)
{
    struct SomeStruct* const ret_p = my_malloc(sizeof(struct SomeStruct));
    if (ret_p == NULL)
    {
        return NULL;
    }

    ret_p->slot_p = slot_p;

    return ret_p;
}

static void SomeStruct_free(void* self_p)
{
    return my_free(self_p);
}

static void SomeStruct_release(struct SomeStruct* self_p)
{
    return PoolSlot_release(self_p->slot_p);
}

int main()
{
    const size_t pool_capacity = 2U;

    struct ObjectPool* pool_p = NULL;

    // Optional arguments:
    {
        const struct ObjectPoolOptArgs args = {
            .on_acquire_cb = SomeStruct_acquire_cb,
            .on_release_cb = SomeStruct_release_cb,
            .c_malloc      = my_malloc,
            .c_free        = my_free,
        };

        pool_p = ObjectPool_new(pool_capacity,
                                SomeStruct_new,
                                SomeStruct_free,
                                NULL,
                                &args);
        // It is fine for args to expire here. There will be no illegal reads.
    }
    assert(pool_p != NULL);

    printf("At this point, there will be no more malloc() calls\n");

    struct SomeStruct* const obj1_p = ObjectPool_acquire(pool_p);
    assert(obj1_p != NULL);

    ObjectPool_free(pool_p);

    printf("There are still 3 more free() call left:\n");
    printf("\tSomeStruct_free() called by the library\n");
    printf("\tThe freeing of the flyweight called by the library\n");
    printf("\tThe freeing of the PoolSlot called by the library\n");

    SomeStruct_release(obj1_p);

    printf("malloc_sz_counter = %zu\n", malloc_sz_counter);

    assert(malloc_call_counter == 1 + 1 + pool_capacity * 2);
    assert(free_call_counter == malloc_call_counter);
    return 0;
}
