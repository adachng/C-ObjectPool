#include <c_objectpool/ObjectPool.h>

#include <stdlib.h>

//! [User-defined struct]
struct SomeStruct
{
    struct PoolSlot* slot_p; // MANDATORY

    // some other data...
    // or another struct that the user cannot modify...
};
//! [User-defined struct]

//! [User-defined callback for the library to instantiate the object]
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
//! [User-defined callback for the library to instantiate the object]

//! [User-defined callback for the library to destroy the object]
static void SomeStruct_destroy(void* self_p)
{
    // NOTE: when using this library, provide this but do not call this.
    return free(self_p);
}
//! [User-defined callback for the library to destroy the object]

//! [User-defined function to enable proper release]
static void SomeStruct_release(struct SomeStruct* self_p)
{
    return PoolSlot_release(self_p->slot_p);
}
//! [User-defined function to enable proper release]

//! [Main function]
int main()
{
    const size_t             pool_capacity = 100U;
    struct ObjectPool* const pool_p        = ObjectPool_new(pool_capacity,
                                                     SomeStruct_new,
                                                     SomeStruct_destroy,
                                                     NULL,
                                                     NULL);

    ObjectPool_destroy(pool_p);

    return 0;
}
//! [Main function]
