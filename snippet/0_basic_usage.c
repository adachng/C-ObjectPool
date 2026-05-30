#include <ObjectPool.h>

#include <stdlib.h>

// User-defined struct:
struct SomeStruct
{
    struct PoolSlot* slot_p; // MANDATORY

    // some other data...
    // or another struct that the user cannot modify...
};

// User-defined callback for the library:
static void* SomeStruct_new(void*            arg_p,
                            struct PoolSlot* slot_p)
{
    struct SomeStruct* const ret_p = malloc(sizeof(struct SomeStruct));
    if (ret_p == NULL)
    {
        return NULL;
    }

    // MANDATORY: user must keep this in struct.
    ret_p->slot_p = slot_p;

    return ret_p;
}

// User-defined callback for the library:
static void SomeStruct_free(void* self_p)
{
    // NOTE: when using this library, provide this but do not call this.
    return free(self_p);
}

// MANDATORY: user must provide an wrapper function to release (not a callback).
static void SomeStruct_release(struct SomeStruct* self_p)
{
    return PoolSlot_release(self_p->slot_p);
}

int main()
{
    const size_t             pool_capacity = 100U;
    struct ObjectPool* const pool_p        = ObjectPool_new(pool_capacity,
                                                     SomeStruct_new,
                                                     SomeStruct_free,
                                                     NULL,
                                                     NULL);

    ObjectPool_free(pool_p);

    return 0;
}
