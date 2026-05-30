// MIT License
//
// Copyright (c) 2026-present adachng (github.com/adachng)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ObjectPool.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct poolStateFlyweight
{
    struct ObjectPoolOptArgs optional_callbacks;
    void                     (*obj_free_cb)(void* self_p);
    struct ObjectPool* owner_pool_p; // set to NULL if object pool is freed
    size_t             ref_count;
};

struct ObjectPool
{
    struct poolStateFlyweight* shared_prop_p;
    struct PoolSlot*           head_p;

    size_t capacity;
    size_t size;
};

struct PoolSlot
{
    struct poolStateFlyweight* shared_prop_p;
    void*                      pooled_obj_p;

    struct PoolSlot* next_p;
    bool             is_in_pool;
};

// Dispatch the free() function:
static inline void (*c_free(const struct ObjectPoolOptArgs* const self_p))(
    void*)
{
    if (self_p == NULL || self_p->c_free == NULL)
    {
        return free;
    }

    return self_p->c_free;
}

// Dispatch the malloc() function:
static inline void* (*c_malloc(const struct ObjectPoolOptArgs* const self_p))(
    size_t)
{
    if (self_p == NULL || self_p->c_malloc == NULL)
    {
        return malloc;
    }

    return self_p->c_malloc;
}

static struct poolStateFlyweight*
    poolStateFlyweight_dec_ref_count(struct poolStateFlyweight* const self_p)
{
    if (self_p == NULL)
    {
        return NULL;
    }

    assert(self_p->ref_count > 0);
    self_p->ref_count--;
    if (self_p->ref_count <= 0)
    {
        c_free (&self_p->optional_callbacks)(self_p);
        return NULL;
    }
    return self_p;
}

struct ObjectPool*
    ObjectPool_new(const size_t capacity,
                   void*        (*const obj_new_cb)(void*            arg_p,
                                             struct PoolSlot* slot_p),

                   void        (*const obj_obj_free_cb)(void* self_p),
                   void* const arg_p,
                   const struct ObjectPoolOptArgs* const optional_callbacks_p)
{
    if (capacity <= 1 || obj_new_cb == NULL || obj_obj_free_cb == NULL)
    {
        return NULL;
    }

    // Memory allocations for object pool and the shared properties flyweight:
    struct poolStateFlyweight* const shared_prop_p =
        c_malloc(optional_callbacks_p)(sizeof(struct poolStateFlyweight));

    struct ObjectPool* const ret_p =
        c_malloc(optional_callbacks_p)(sizeof(struct ObjectPool));

    if (shared_prop_p == NULL || ret_p == NULL)
    {
        goto bad_return;
    }

    // Assign values to the shared properties flyweight:
    *shared_prop_p = (struct poolStateFlyweight){
        .obj_free_cb  = obj_obj_free_cb,
        .owner_pool_p = ret_p,
        .optional_callbacks =
            optional_callbacks_p == NULL
                ? (struct ObjectPoolOptArgs){.on_acquire_cb = NULL,
                                             .on_release_cb = NULL,
                                             .c_free = NULL,
                                             .c_malloc = NULL,
                                            }
                : *optional_callbacks_p,
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
        struct PoolSlot* prev_p = NULL;
        for (size_t i = 0; i < capacity; i++)
        {
            struct PoolSlot* const current_p =
                c_malloc(optional_callbacks_p)(sizeof(struct PoolSlot));
            if (current_p == NULL)
            {
                goto bad_return;
            }

            *current_p = (struct PoolSlot){
                .shared_prop_p = shared_prop_p,
                .next_p        = NULL,
                .pooled_obj_p  = obj_new_cb(arg_p, current_p),
                .is_in_pool    = true,
            };

            shared_prop_p->ref_count++;
            if (current_p->pooled_obj_p == NULL)
            {
                // Allocation of underlying object failed:
                c_free(optional_callbacks_p)(current_p);
                goto bad_return;
            }

            if (i > 0)
            {
                assert(prev_p != NULL);
                prev_p->next_p = current_p;
            }
            else
            {
                assert(prev_p == NULL);
                ret_p->head_p = current_p;
            }

            prev_p = current_p;
        }
    }

    assert(shared_prop_p->ref_count == capacity + 1);

    return ret_p;
bad_return:
    c_free(optional_callbacks_p)(shared_prop_p);

    struct PoolSlot* current_p = ret_p->head_p;
    while (current_p != NULL)
    {
        struct PoolSlot* const tmp_p = current_p;
        current_p                    = current_p->next_p;

        obj_obj_free_cb(tmp_p->pooled_obj_p);
        c_free(optional_callbacks_p)(tmp_p);
    }

    c_free(optional_callbacks_p)(ret_p);
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
    assert(self_p->shared_prop_p->obj_free_cb != NULL);

    // Set the owner_pool_p to NULL in the flyweight to signify that the object
    // pool has been freed to prevent use-after-free:
    self_p->shared_prop_p->owner_pool_p = NULL;

    // Decrement the reference count, since the object pool is freed:
    struct poolStateFlyweight* shared_prop_p = self_p->shared_prop_p;

    // Store the dispatched free() because of flyweight lifetime:
    const struct ObjectPoolOptArgs opt_callbacks =
        self_p->shared_prop_p->optional_callbacks;

    // Iterate through the linked list to free all pooled objects within:
    struct PoolSlot* current_p = self_p->head_p;
    while (current_p != NULL)
    {
        struct PoolSlot* const tmp_p = current_p;
        assert(tmp_p->is_in_pool == true);
        current_p = current_p->next_p;

        shared_prop_p->obj_free_cb(tmp_p->pooled_obj_p);
        shared_prop_p = poolStateFlyweight_dec_ref_count(shared_prop_p);

        c_free (&opt_callbacks)(tmp_p);

        // Impossible for the reference count to be less than expected:
        assert(current_p != NULL && shared_prop_p != NULL || true);
    }

    // Decrement the reference count once more, since the object pool is freed:
    assert(shared_prop_p != NULL); // should be impossible to be NULL here
    poolStateFlyweight_dec_ref_count(self_p->shared_prop_p);
    c_free (&opt_callbacks)(self_p);
}

size_t ObjectPool_get_size(const struct ObjectPool* const self_p)
{
    if (self_p == NULL)
    {
        return 0;
    }

    return self_p->size;
}

size_t ObjectPool_get_capacity(const struct ObjectPool* const self_p)
{
    if (self_p == NULL)
    {
        return self_p->size;
    }

    return self_p->capacity;
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
    struct PoolSlot* const to_acquire_p = self_p->head_p;
    self_p->head_p                      = to_acquire_p->next_p;

    to_acquire_p->next_p     = NULL;
    to_acquire_p->is_in_pool = false;

    // Run activation callback if specified during object pool creation:
    if (self_p->shared_prop_p->optional_callbacks.on_acquire_cb != NULL)
    {
        self_p->shared_prop_p->optional_callbacks.on_acquire_cb(
            to_acquire_p->pooled_obj_p);
    }

    return to_acquire_p->pooled_obj_p;
}

void PoolSlot_release(struct PoolSlot* const self_p)
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
    assert(self_p->shared_prop_p->obj_free_cb != NULL);

    // Run deactivation callback if specified during object pool creation:
    if (self_p->shared_prop_p->optional_callbacks.on_release_cb != NULL)
    {
        self_p->shared_prop_p->optional_callbacks.on_release_cb(
            self_p->pooled_obj_p);
    }

    if (self_p->shared_prop_p->owner_pool_p == NULL)
    {
        // Object pool is freed, free instead of release into NULL:
        self_p->shared_prop_p->obj_free_cb(self_p->pooled_obj_p);
        struct poolStateFlyweight* const tmp_p = self_p->shared_prop_p;
        c_free (&self_p->shared_prop_p->optional_callbacks)(self_p);
        poolStateFlyweight_dec_ref_count(tmp_p);
    }
    else
    {
        // Object pool is not freed yet, do the actual release:
        struct ObjectPool* const owner_pool_p =
            self_p->shared_prop_p->owner_pool_p;
        owner_pool_p->size++;

        self_p->is_in_pool = true;
        self_p->next_p     = owner_pool_p->head_p;

        owner_pool_p->head_p = self_p;
    }
}

bool PoolSlot_is_in_pool(const struct PoolSlot* self_p)
{
    if (self_p == NULL)
    {
        return false;
    }

    return self_p->is_in_pool;
}
