#include <gtest/gtest.h>

#include <ObjectPool.h>

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
        return PooledObject_release(slot_p);
    }

    // New callback:
    static void* New(void*                arg_p,
                     struct PooledObject* slot_p)
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
    struct PooledObject* slot_p; // non-owning
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

void* new_cb(void* const          arg_p,
             struct PooledObject* slot_p)
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

// TODO:
// namespace // MainTest, Leakage
// {
//
// TEST(MainTest,
//      Leakage)
// {
// }
//
// } // namespace
