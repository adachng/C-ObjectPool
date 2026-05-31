#include <gtest/gtest.h>

#include <c_objectpool/ObjectPool.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <stddef.h>
#include <stdlib.h>

namespace // MainTest, Lifecycle
{

class LifecycleObj
{
  public:
    enum State
    {
        ALLOCATED,
        ACTIVATED,
        DEACTIVATED,
    };

    // Release into the pool:
    void Release(void)
    {
        ASSERT_NE(slot_p, nullptr);
        return PoolSlot_release(slot_p);
    }

    // New callback:
    static void* New(void*            arg_p,
                     struct PoolSlot* slot_p)
    {
        // Return nullptr instead of exception if failed:
        const auto ret_p = new (std::nothrow) LifecycleObj;
        ret_p->StateVec.push_back(State{ALLOCATED});

        // NOTE: ASSERT_EQ() macro needs to be in functions that return void.
        EXPECT_EQ(ret_p->StateVec.size(), 1);

        ret_p->slot_p = slot_p;
        ret_p->Name   = *reinterpret_cast<std::string*>(arg_p);
        return reinterpret_cast<void*>(ret_p);
    }

    // Free callback:
    static void Free(void* self_p)
    {
        delete reinterpret_cast<LifecycleObj*>(self_p);
    }

    // Activation callback:
    static void Activate(void* self_p)
    {
        return reinterpret_cast<LifecycleObj*>(self_p)->StateVec.push_back(
            State{ACTIVATED});
    }

    // Deactivation callback:
    static void Deactivate(void* self_p)
    {
        return reinterpret_cast<LifecycleObj*>(self_p)->StateVec.push_back(
            State{DEACTIVATED});
    }

    std::string        Name;
    std::vector<State> StateVec;

  private:
    struct PoolSlot* slot_p; // non-owning
};

TEST(MainTest,
     Lifecycle)
{
    std::string arg = "ABC";

    struct ObjectPool* pool_p = NULL;

    // Create the object pool (showing the optional arguments can expire):
    {
        // NOTE: it is perfectly fine for this to expire:
        struct ObjectPoolOptArgs opt_args = {
            .on_acquire_cb = LifecycleObj::Activate,
            .on_release_cb = LifecycleObj::Deactivate};

        pool_p = ObjectPool_new(2,
                                LifecycleObj::New,
                                LifecycleObj::Free,
                                reinterpret_cast<void*>(&arg),
                                &opt_args);

        ASSERT_NE(pool_p, nullptr);

        // NOTE: opt_args expires here, safe by design.
    }

    // Acquire all 2 objects from the object pool:
    LifecycleObj* const obj1_p =
        reinterpret_cast<LifecycleObj*>(ObjectPool_acquire(pool_p));
    LifecycleObj* const obj2_p =
        reinterpret_cast<LifecycleObj*>(ObjectPool_acquire(pool_p));

    ASSERT_NE(obj1_p, nullptr);
    ASSERT_NE(obj2_p, nullptr);

    // Pool is exhausted:
    ASSERT_EQ(ObjectPool_acquire(pool_p), nullptr);

    // Callback argument works:
    ASSERT_EQ(obj1_p->Name, std::string{"ABC"});
    ASSERT_EQ(obj2_p->Name, std::string{"ABC"});

    // StateVec state (use () for prvalue to not confuse macro):
    ASSERT_EQ(obj1_p->StateVec,
              (std::vector<LifecycleObj::State>{
                  LifecycleObj::State::ALLOCATED,
                  LifecycleObj::State::ACTIVATED,
              }));
    ASSERT_EQ(obj2_p->StateVec,
              (std::vector<LifecycleObj::State>{
                  LifecycleObj::State::ALLOCATED,
                  LifecycleObj::State::ACTIVATED,
              }));

    // Release obj1 into the pool and get it again:
    obj1_p->Release();
    ASSERT_EQ(obj1_p->StateVec,
              (std::vector<LifecycleObj::State>{
                  LifecycleObj::State::ALLOCATED,
                  LifecycleObj::State::ACTIVATED,
                  LifecycleObj::State::DEACTIVATED,
              }));

    // Acquire obj1 again:
    ASSERT_EQ(reinterpret_cast<LifecycleObj*>(ObjectPool_acquire(pool_p)),
              obj1_p);
    ASSERT_EQ(obj1_p->StateVec,
              (std::vector<LifecycleObj::State>{
                  LifecycleObj::State::ALLOCATED,
                  LifecycleObj::State::ACTIVATED,
                  LifecycleObj::State::DEACTIVATED,
                  LifecycleObj::State::ACTIVATED,
              }));

    // Release obj2 into the pool:
    obj2_p->Release();
    ASSERT_EQ(obj2_p->StateVec,
              (std::vector<LifecycleObj::State>{
                  LifecycleObj::State::ALLOCATED,
                  LifecycleObj::State::ACTIVATED,
                  LifecycleObj::State::DEACTIVATED,
              }));

    // Free the pool (obj2 is invalid after the free, but obj1 is still valid):
    ObjectPool_free(pool_p);

    // Free obj1:
    obj1_p->Release(); // the library frees this object since the pool is freed
}

} // namespace

namespace // MainTest, NewFailed
{

void* new_cb(void* const      arg_p,
             struct PoolSlot* slot_p)
{
    (void)slot_p;
    size_t* const new_counter_p = reinterpret_cast<size_t*>(arg_p);
    (*new_counter_p)++;
    return NULL;
}

size_t free_counter = 0; // does not pass into callback by design

void free_cb(void* const self_p)
{
    free_counter++;
}

TEST(MainTest,
     NewFailed)
{
    size_t new_counter = 0;

    struct ObjectPool* const pool_p =
        ObjectPool_new(100,
                       new_cb,
                       free_cb,
                       reinterpret_cast<void*>(&new_counter),
                       NULL);

    ASSERT_EQ(pool_p, nullptr);
    ASSERT_EQ(new_counter, 1);
    ASSERT_EQ(free_counter, 0);

    ObjectPool_free(pool_p);
}

} // namespace

namespace // MainTest, Leakage
{

struct MyStruct
{
    struct PoolSlot* slot_p;
};

void* MyStruct_new(void* const            arg_p,
                   struct PoolSlot* const slot_p)
{
    struct MyStruct* const ret_p =
        reinterpret_cast<struct MyStruct*>(malloc(sizeof(struct MyStruct)));
    if (ret_p == NULL)
    {
        return NULL;
    }

    *ret_p = (struct MyStruct){.slot_p = slot_p};
    return reinterpret_cast<void*>(ret_p);
}

void MyStruct_free(void* const self_p)
{
    return free(self_p);
}

void MyStruct_release(struct MyStruct* const self_p)
{
    return PoolSlot_release(self_p->slot_p);
}

TEST(MainTest,
     Memory)
{
    // NOTE: if using Valgrind, ensure --max-stackframe is sufficient.

    const struct ObjectPoolOptArgs add_args = {
        .c_malloc = malloc,
        .c_free   = free,
    };

    // Allocate and deallocate 1 million objects:
    {
        constexpr auto pool_capacity = size_t{1000000ULL};

        struct ObjectPool* const pool_p = ObjectPool_new(pool_capacity,
                                                         MyStruct_new,
                                                         MyStruct_free,
                                                         NULL,
                                                         &add_args);
        ASSERT_NE(pool_p, nullptr);

        ObjectPool_free(pool_p);
    }

    // Allocate, acquire all, and deallocate the pool:
    {
        constexpr auto           pool_capacity = size_t{1000000ULL};
        struct ObjectPool* const pool_p        = ObjectPool_new(pool_capacity,
                                                         MyStruct_new,
                                                         MyStruct_free,
                                                         NULL,
                                                         &add_args);
        ASSERT_NE(pool_p, nullptr);

        // Array to hold all objects acquired:
        std::array<struct MyStruct*, pool_capacity> pointer_arr;
        pointer_arr.fill(nullptr);
        ASSERT_TRUE(std::all_of(pointer_arr.begin(),
                                pointer_arr.end(),
                                [](const struct MyStruct* const p)
                                { return p == nullptr; }));

        // Acquire all objects:
        for (size_t i = 0; i < pointer_arr.size(); // NOTE: same as max_size()
             i++)
        {
            pointer_arr[i] =
                reinterpret_cast<struct MyStruct*>(ObjectPool_acquire(pool_p));
        }
        for (size_t i = 0; i < pointer_arr.size(); i++)
        {
            ASSERT_EQ(ObjectPool_acquire(pool_p), nullptr);
        }
        ASSERT_TRUE(std::all_of(pointer_arr.begin(),
                                pointer_arr.end(),
                                [](const struct MyStruct* const p)
                                { return p != nullptr; }));

        // Free the pool:
        ObjectPool_free(pool_p);

        // Release all objects:
        for (size_t i = 0; i < pointer_arr.size(); i++)
        {
            // CAUTION: doing MyStruct_free(pointer_arr[i]) is incorrect usage!
            MyStruct_release(pointer_arr[i]);
        }
    }

    // Allocate, acquire half, and free the pool + release the acquired objects:
    {
        constexpr auto           pool_capacity = size_t{1000000ULL};
        struct ObjectPool* const pool_p        = ObjectPool_new(pool_capacity,
                                                         MyStruct_new,
                                                         MyStruct_free,
                                                         NULL,
                                                         &add_args);
        ASSERT_NE(pool_p, nullptr);

        // Array to hold all objects acquired:
        std::array<struct MyStruct*, pool_capacity / 2> pointer_arr;
        pointer_arr.fill(nullptr);
        ASSERT_TRUE(std::all_of(pointer_arr.begin(),
                                pointer_arr.end(),
                                [](const struct MyStruct* const p)
                                { return p == nullptr; }));

        // Acquire all objects:
        for (size_t i = 0; i < pointer_arr.size(); // NOTE: same as max_size()
             i++)
        {
            pointer_arr[i] =
                reinterpret_cast<struct MyStruct*>(ObjectPool_acquire(pool_p));
        }
        ASSERT_TRUE(std::all_of(pointer_arr.begin(),
                                pointer_arr.end(),
                                [](const struct MyStruct* const p)
                                { return p != nullptr; }));

        // Free the pool:
        ObjectPool_free(pool_p);

        // Release all objects:
        for (size_t i = 0; i < pointer_arr.size(); i++)
        {
            // CAUTION: doing MyStruct_free(pointer_arr[i]) is incorrect usage!
            MyStruct_release(pointer_arr[i]);
        }
    }
}

} // namespace

namespace // MainTest, OptionalCallbacks
{

struct ArbitraryStruct
{
    struct PoolSlot* slot_p;
};

size_t malloc_call_counter = 0;
size_t free_call_counter   = 0;

void* malloc_call(const size_t size)
{
    malloc_call_counter++;
    return malloc(size);
};

void free_call(void* const ptr)
{
    free_call_counter++;
    return free(ptr);
};

void* ArbitraryStruct_new(void* const            unused_p,
                          struct PoolSlot* const slot_p)
{
    const auto ret_p = reinterpret_cast<struct ArbitraryStruct*>(
        malloc_call(sizeof(struct ArbitraryStruct)));
    ret_p->slot_p = slot_p;
    return ret_p;
}

void ArbitraryStruct_release(struct ArbitraryStruct* const self_p)
{
    return PoolSlot_release(self_p->slot_p);
}

void ArbitraryStruct_free(void* const self_p)
{
    return free_call(self_p);
}

size_t acquire_counter = 0;
size_t release_counter = 0;

TEST(MainTest,
     OptionalCallbacks)
{
    auto acquire_cb = [](void* const unused_p)
    {
        acquire_counter++;
    };
    auto release_cb = [](void* const unused_p)
    {
        release_counter++;
    };

    ASSERT_EQ(acquire_counter, 0);
    ASSERT_EQ(release_counter, 0);
    ASSERT_EQ(malloc_call_counter, 0);
    ASSERT_EQ(free_call_counter, 0);

    const struct ObjectPoolOptArgs args = {.on_acquire_cb = acquire_cb,
                                           .on_release_cb = release_cb,
                                           .c_malloc      = malloc_call,
                                           .c_free        = free_call};

    constexpr size_t pool_capacity = 100ULL;

    struct ObjectPool* const pool_p = ObjectPool_new(pool_capacity,
                                                     ArbitraryStruct_new,
                                                     ArbitraryStruct_free,
                                                     NULL,
                                                     &args);

    ASSERT_EQ(acquire_counter, 0);
    ASSERT_EQ(release_counter, 0);

    // Assert acquire and release counters:
    {
        struct ArbitraryStruct* const obj1_p =
            reinterpret_cast<struct ArbitraryStruct*>(
                ObjectPool_acquire(pool_p));

        ASSERT_EQ(acquire_counter, 1);
        ASSERT_EQ(release_counter, 0);

        struct ArbitraryStruct* const obj2_p =
            reinterpret_cast<struct ArbitraryStruct*>(
                ObjectPool_acquire(pool_p));

        ASSERT_EQ(acquire_counter, 2);
        ASSERT_EQ(release_counter, 0);

        ArbitraryStruct_release(obj1_p);

        ASSERT_EQ(acquire_counter, 2);
        ASSERT_EQ(release_counter, 1);

        ArbitraryStruct_release(reinterpret_cast<struct ArbitraryStruct*>(
            ObjectPool_acquire(pool_p)));

        ASSERT_EQ(acquire_counter, 3);
        ASSERT_EQ(release_counter, 2);

        ArbitraryStruct_release(obj2_p);

        ASSERT_EQ(acquire_counter, 3);
        ASSERT_EQ(release_counter, 3);
    }

    ObjectPool_free(pool_p);

    // Assert for malloc and free call counters:
    ASSERT_EQ(malloc_call_counter,
              size_t{1U + 1U} + size_t{pool_capacity * 2U});
    // NOTE: 1U + 1U for the pool and flyweight.
    ASSERT_EQ(malloc_call_counter, free_call_counter);
}

} // namespace
