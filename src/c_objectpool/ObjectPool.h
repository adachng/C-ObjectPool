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

#ifndef C_OBJECTPOOL__OBJECTPOOL_H
#define C_OBJECTPOOL__OBJECTPOOL_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct ObjectPool;
struct PoolSlot;

struct ObjectPoolOptArgs
{
    void  (*on_acquire_cb)(void* self_p);
    void  (*on_release_cb)(void* self_p);
    void* (*c_malloc)(size_t size);
    void  (*c_free)(void* ptr);
};

struct ObjectPool*
    ObjectPool_new(size_t capacity,
                   void*  (*obj_new_cb)(void*            arg_p,
                                       struct PoolSlot* slot_p),

                   void  (*obj_destroy_cb)(void* self_p),
                   void* arg_p,
                   const struct ObjectPoolOptArgs* optional_callbacks_p);

void ObjectPool_destroy(struct ObjectPool* self_p);

size_t ObjectPool_get_size(const struct ObjectPool* self_p);

size_t ObjectPool_get_capacity(const struct ObjectPool* self_p);

void* ObjectPool_acquire(struct ObjectPool* self_p);

void PoolSlot_release(struct PoolSlot* self_p);

bool PoolSlot_is_in_pool(const struct PoolSlot* self_p);

#ifdef __cplusplus
}
#endif

#endif // C_OBJECTPOOL__OBJECTPOOL_H
