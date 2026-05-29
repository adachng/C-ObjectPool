#ifndef C_OBJECTPOOL__OBJECTPOOL_H
#define C_OBJECTPOOL__OBJECTPOOL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct ObjectPool;
struct PooledObject;

struct ObjectPoolOptArgs
{
    void (*on_acquire_cb)(void* self_p);
    void (*on_release_cb)(void* self_p);
};

struct ObjectPool*
    ObjectPool_new(size_t capacity,
                   void*  (*obj_new_cb)(void*                arg_p,
                                       struct PooledObject* tag_p),

                   void                            (*obj_free_cb)(void* self_p),
                   void*                           arg_p,
                   const struct ObjectPoolOptArgs* optional_callbacks_p);

void ObjectPool_free(struct ObjectPool* self_p);

size_t ObjectPool_get_size(struct ObjectPool* self_p);

void* ObjectPool_acquire(struct ObjectPool* self_p);

void PooledObject_release(struct PooledObject* self_p);

#ifdef __cplusplus
}
#endif

#endif // C_OBJECTPOOL__OBJECTPOOL_H
