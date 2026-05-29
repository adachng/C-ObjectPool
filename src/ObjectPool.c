#include "ObjectPool.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct sharedPropCallbacks
{
    struct ObjectPoolOptArgs additional_cbs;
    void                     (*free_cb)(void* self_p);
    struct ObjectPool*       origin_p; // set to NULL if object pool is freed
    size_t                   ref_count;
};

static struct sharedPropCallbacks*
    sharedPropCallbacks_dec_ref_count(struct sharedPropCallbacks* const self_p)
{
    if (self_p == NULL)
    {
        return NULL;
    }

    assert(self_p->ref_count > 0);
    self_p->ref_count--;
    if (self_p->ref_count <= 0)
    {
        free(self_p);
        return NULL;
    }
    return self_p;
}

struct ObjectPool
{
    struct sharedPropCallbacks* shared_prop_p;
    struct PooledObject*        head_p;

    size_t capacity;
    size_t size;
};

struct PooledObject
{
    struct sharedPropCallbacks* shared_prop_p;
    void*                       underlying_obj_p;

    struct PooledObject* next_p;
    bool                 is_in_pool;
};

struct ObjectPool*
    ObjectPool_new(const size_t capacity,
                   void*        (*const obj_new_cb)(void*                arg_p,
                                             struct PooledObject* tag_p),

                   void        (*const obj_free_cb)(void* self_p),
                   void* const arg_p,
                   const struct ObjectPoolOptArgs* const optional_callbacks_p)
{
    if (capacity <= 1 || obj_new_cb == NULL || obj_free_cb == NULL)
    {
        return NULL;
    }

    // Memory allocations for object pool and the shared properties flyweight:
    struct sharedPropCallbacks* const shared_prop_p =
        malloc(sizeof(struct sharedPropCallbacks));

    struct ObjectPool* const ret_p = malloc(sizeof(struct ObjectPool));

    if (shared_prop_p == NULL || ret_p == NULL)
    {
        goto bad_return;
    }

    // Assign values to the shared properties flyweight:
    *shared_prop_p = (struct sharedPropCallbacks){
        .free_cb  = obj_free_cb,
        .origin_p = ret_p,
        .additional_cbs =
            {
                .on_acquire_cb = optional_callbacks_p == NULL
                                     ? NULL
                                     : optional_callbacks_p->on_acquire_cb,
                .on_release_cb = optional_callbacks_p == NULL
                                     ? NULL
                                     : optional_callbacks_p->on_release_cb,
            },
        .ref_count = 1,
    };

    // Assign values to object pool:
    *ret_p = (struct ObjectPool){
        .shared_prop_p = shared_prop_p,
        .head_p        = NULL,
        .capacity      = capacity,
        .size          = capacity,
    };

    // Memory allocations for the pooled objects linked list:
    {
        struct PooledObject* prev_p = NULL;
        for (size_t i = 0; i < capacity; i++)
        {
            struct PooledObject* const current_p =
                malloc(sizeof(struct PooledObject));
            if (current_p == NULL)
            {
                goto bad_return;
            }

            *current_p = (struct PooledObject){
                .shared_prop_p    = shared_prop_p,
                .next_p           = NULL,
                .underlying_obj_p = obj_new_cb(arg_p, current_p),
                .is_in_pool       = true,
            };

            shared_prop_p->ref_count++;
            if (current_p->underlying_obj_p == NULL)
            {
                // Allocation of underlying object failed:
                goto bad_return;
            }

            if (i > 0)
            {
                assert(prev_p != NULL);
                prev_p->next_p = current_p;
            }
            else
            {
                ret_p->head_p = current_p;
            }

            prev_p = current_p;
        }
    }

    assert(shared_prop_p->ref_count == capacity + 1);

    return ret_p;
bad_return:
    free(shared_prop_p);

    struct PooledObject* current_p = ret_p->head_p;
    while (current_p != NULL)
    {
        struct PooledObject* const tmp_p = current_p;
        current_p                        = current_p->next_p;

        obj_free_cb(tmp_p->underlying_obj_p);
        free(tmp_p);
    }

    free(ret_p);
    return NULL;
}

void ObjectPool_free(struct ObjectPool* const self_p)
{
    if (self_p == NULL)
    {
        return;
    }

    assert(self_p->shared_prop_p != NULL);
    assert(self_p->shared_prop_p->ref_count > 0);
    assert(self_p->shared_prop_p->free_cb != NULL);

    // Set the origin_p to NULL in the flyweight to signify that the object pool
    // has been freed to prevent use-after-free:
    self_p->shared_prop_p->origin_p = NULL;

    // Decrement the reference count, since the object pool is freed:
    struct sharedPropCallbacks* shared_prop_p =
        sharedPropCallbacks_dec_ref_count(self_p->shared_prop_p);
    assert(shared_prop_p != NULL); // should be impossible to be NULL here

    // Iterate through the linked list to free all pooled objects within:
    struct PooledObject* current_p = self_p->head_p;
    while (current_p != NULL)
    {
        struct PooledObject* const tmp_p = current_p;
        assert(tmp_p->is_in_pool == true);
        current_p = current_p->next_p;

        shared_prop_p->free_cb(tmp_p->underlying_obj_p);
        shared_prop_p = sharedPropCallbacks_dec_ref_count(shared_prop_p);

        free(tmp_p);

        // Impossible for the reference count to be less than expected:
        assert(current_p != NULL && shared_prop_p != NULL || true);
    }

    free(self_p);
}

size_t ObjectPool_get_size(struct ObjectPool* self_p)
{
    if (self_p == NULL)
    {
        return 0;
    }

    return self_p->size;
}

void* ObjectPool_acquire(struct ObjectPool* self_p)
{
    if (self_p == NULL || self_p->size <= 0)
    {
        return NULL;
    }

    // Decrement the object pool size counter:
    self_p->size--;
    assert(self_p->head_p != NULL);

    // Update linked list relationships:
    struct PooledObject* const to_acquire_p = self_p->head_p;
    self_p->head_p                          = to_acquire_p->next_p;

    to_acquire_p->next_p     = NULL;
    to_acquire_p->is_in_pool = false;

    // Run activation callback if specified during object pool creation:
    if (self_p->shared_prop_p->additional_cbs.on_acquire_cb != NULL)
    {
        self_p->shared_prop_p->additional_cbs.on_acquire_cb(
            to_acquire_p->underlying_obj_p);
    }

    return to_acquire_p->underlying_obj_p;
}

void PooledObject_release(struct PooledObject* const self_p)
{
    if (self_p == NULL)
    {
        return;
    }

    // Defend against attempt to release something already released:
    assert(self_p->is_in_pool == false);
    if (self_p->is_in_pool)
    {
        return;
    }

    assert(self_p->shared_prop_p != NULL);
    assert(self_p->next_p == NULL);
    assert(self_p->shared_prop_p->free_cb != NULL);

    // Run deactivation callback if specified during object pool creation:
    if (self_p->shared_prop_p->additional_cbs.on_release_cb != NULL)
    {
        self_p->shared_prop_p->additional_cbs.on_release_cb(
            self_p->underlying_obj_p);
    }

    if (self_p->shared_prop_p->origin_p == NULL)
    {
        // Object pool is freed, free instead of release into NULL:
        self_p->shared_prop_p->free_cb(self_p->underlying_obj_p);
        free(self_p);
    }
    else
    {
        // Object pool is not freed yet, do the actual release:
        struct ObjectPool* const origin_p = self_p->shared_prop_p->origin_p;
        origin_p->size++;

        self_p->is_in_pool = true;
        self_p->next_p     = origin_p->head_p;

        origin_p->head_p = self_p;
    }
}
