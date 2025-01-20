/*
   Copyright 2024 Joseph W. Seigh
   
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include <proxy.h>

#include <stdint.h>

#include <stdio.h>      // temp


// constexpr std::memory_order relaxed = std::memory_order_relaxed;
// constexpr std::memory_order release = std::memory_order_release;
// constexpr std::memory_order acquire = std::memory_order_acquire;


inline constexpr uint64_t dword(uint32_t hi, uint32_t lo) {
    uint64_t val = hi;
    val <<= 32;
    return val + lo;
}
inline uint32_t word0(uint64_t val) { return val >> 32; }
inline uint32_t word1(uint64_t val) { return val; }


// constexpr uint64_t LREF = 1LLU << 32;


class arc_obj_base
{
public:
    virtual ~arc_obj_base() {}

    arc_obj_base* next;
};

/*
 *   0-31   local or ephemeral count (word0)
 *  32-63   link eference count (word1)
*/
using refcount_t = uint64_t;

constexpr refcount_t ONE_REF = dword(1, 0);
constexpr refcount_t ONE_LINK = dword(0, 1);
constexpr refcount_t free_count = dword(0, 2);


/*
 *   0-31   local or ephemeral count (word0)
 *  32-63   node index (word1)
*/
using arc_ref = uint64_t;


static_assert(sizeof(arc_ref) == sizeof(refcount_t), "arcref not same size as refcount_t");

struct arcnode_t
{
    std::atomic<refcount_t>  count;
    std::atomic<arc_obj_base*> reclaim_queue;
};

class arc_ref_t;
class arcproxy;
static arc_ref_t* new_arc_ref(arcproxy* proxy);
static void  delete_arc_ref(arc_ref_t* ref);


class arcproxy
{
    static constexpr std::memory_order _relaxed = std::memory_order_relaxed;
    static constexpr std::memory_order _release = std::memory_order_release;
    static constexpr std::memory_order _acquire = std::memory_order_acquire;

    friend class arc_ref_t;


    std::atomic<arc_ref> tail;

    arcnode_t   *nodes;

    std::size_t const size;


    uint32_t _lock()
    {
        arc_ref _ref = tail.fetch_add(dword(1, 0), std::memory_order_acquire);
        return word1(_ref);
    }

    void _unlock(uint32_t const _ndx)
    {
        uint32_t ndx = _ndx;
        refcount_t dropcount = ONE_REF; // drop local reference

        for (;;)
        {
            arcnode_t& node = nodes[ndx];

            refcount_t prev = node.count.fetch_sub(dropcount, std::memory_order_relaxed);

            if (prev == dropcount)  // refcount zero
            {
                arc_obj_base* obj = node.reclaim_queue.exchange(nullptr, std::memory_order_acquire);
                while (obj != nullptr)
                {
                    arc_obj_base* obj2 = obj;
                    obj = obj->next;            
                    delete obj2;            
                }
                node.count.store(dword(0, 2), std::memory_order_release);   // return to free list
            }

            else if (word1(tail.load(_relaxed)) == ndx && node.reclaim_queue.load(_relaxed) != nullptr)
            {
                uint32_t _local = _lock();
                add_tail(ndx);

                // non-recursive release of lock
                ndx = _local;
                dropcount = ONE_REF; // drop local reference
                continue;

                //  -- or -- 

                // recursive release of lock
                // _unlock(_local);
                // break;
            }

            else    // refcount not zero
                break;


            dropcount = ONE_LINK;           // drop link reference
            ndx = (ndx + 1) % size;         // next node
        }
    }

    /**
     * @brief attempt to bump tail index, arc lock must be held
     * 
     * @param old_ndx possible tail index (tail may have changed)
     */
    void add_tail(const uint32_t old_ndx)
    {
        arcnode_t& node = nodes[old_ndx];   // ?

        if (word1(node.count.load(_relaxed)) != 1)   // tail node should be only node
            return;

        if (node.reclaim_queue.load(_relaxed) == nullptr)      // no retires
            return;

        arc_ref old_tail, new_tail;

        uint32_t new_ndx = (old_ndx + 1) %size;
        if (nodes[new_ndx].count.load(_relaxed) != free_count)   // no space, next node not in free list
            return;

        new_tail = dword(0, new_ndx);  // local refcount = 0, ndx++

        do {
            old_tail = tail.load(std::memory_order_relaxed);
            if (word1(old_tail) != old_ndx)     // tail changed
                return;
        }
        while (!tail.compare_exchange_weak(old_tail, new_tail, std::memory_order_relaxed, std::memory_order_relaxed));
        uint32_t xx = word0(old_tail);
        nodes[old_ndx].count.fetch_add(dword(xx, 0) - 1);
    }


public:

    arcproxy(std::size_t const size) : size(size)
    {
        nodes = new arcnode_t[size];
        nodes[0].count = dword(0, 1);
        for (int ndx = 1; ndx < size; ndx++)
        {
            nodes[ndx].count = dword(0, 2);
        }
        tail = dword(0, 0);
    }

    ~arcproxy()
    {
        delete[] nodes;
    }

    arc_ref_t* acquire_ref()
    {
        return new_arc_ref(this);
    }

    void release_ref(arc_ref_t* ref)
    {
        delete_arc_ref(ref);
    }

    void retire(arc_obj_base* obj)
    {
        if (obj == nullptr)
            return;
                
        uint32_t _local = _lock();      // ======

        obj->next = nodes[_local].reclaim_queue.exchange(obj, std::memory_order_relaxed);     // push onto node reclaim queue

        _unlock(_local);                // ======

        return;
    }



    void dump(std::string label)
    {
        fprintf(stdout, "%s:\n", label.c_str());
        fprintf(stdout, "  tail = %u.%u\n", word0(tail), word1(tail));
        for (int ndx = 0; ndx < size; ndx++) {
            fprintf(stdout, "  [%0d] %d.%u %p\n", ndx,
                word0(nodes[ndx].count),
                word1(nodes[ndx].count),
                nodes[ndx].reclaim_queue.load(),
                1);
        }
        fprintf(stdout, "\n");

    }
};



class arc_ref_t
{
    uint32_t ndx = -1;

    arcproxy* proxy;

public:
    arc_ref_t(arcproxy* proxy) : proxy(proxy) {}

    void lock()
    {
        ndx = proxy->_lock();
    }

    void unlock()
    {
        if (ndx == -1)
            return;

        proxy->_unlock(ndx);
        ndx = -1;
    }
};

static_assert(ProxyType<arcproxy, arc_ref_t, arc_obj_base>);


static arc_ref_t* new_arc_ref(arcproxy* proxy)
{
    return new arc_ref_t(proxy);
}

static void  delete_arc_ref(arc_ref_t* ref)
{
    delete ref;
}


/*==*/