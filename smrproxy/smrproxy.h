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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <latch>
#include <vector>
#include <memory>

#include <cassert>

#include "epoch.h"

#include <proxy.h>
#include <membarrier.h>

#include <stdint.h>


#ifndef SMRPROXY_MB
    constexpr bool _smrproxy_mb = false;        // using global memory barrier
#else
    constexpr bool _smrproxy_mb = true;        // no global memory barrier, local memory barriers required
#endif


class smr_ref;
class smrproxy;
class smrexpiry;

class smr_obj_base
{
public:
    smr_obj_base * smr_obj_next = nullptr;

    std::atomic<epoch_t> expiry;            // set to expiry epoch by reclaim

    /**
     * set on call to retire
     * guaranteed to be less than expiry
     */
    std::atomic<epoch_t> pre_expiry;

    bool deleted = false;

    smr_obj_base() {
        expiry.store(0, std::memory_order_relaxed);
        pre_expiry.store(0, std::memory_order_relaxed);
    }


    virtual ~smr_obj_base() {
        if (deleted)
            abort();

        deleted = true;
    }
};

class alignas(64) smr_ref
{
    friend class smrproxy;
    friend class smrexpiry;

    smr_ref(smr_ref&) = delete;     // no copy
    smr_ref(smr_ref&&) = delete;    // no move
    smr_ref& operator=(smr_ref&&) = delete; 


    std::atomic<epoch_t> _ref_epoch;        // set by lock() from shadow_epoch, by unlock() to 0, read by reclaim

    std::atomic<epoch_t>  shadow_epoch;     // set by reclaim thread, read by reader thread lock()

    epoch_t effective_epoch;                // set and read by reclaim thread -- not atomic

public:

    smr_ref(epoch_t epoch) {
        _ref_epoch.store(0);
        shadow_epoch = epoch;
        effective_epoch = epoch;
    }

    ~smr_ref()
    {
        print();
    }


    inline void lock()
    {
        if constexpr(_smrproxy_mb)
        {
            epoch_t _epoch = shadow_epoch.load(std::memory_order_relaxed);
            _ref_epoch.store(_epoch, std::memory_order_seq_cst);
            // _ref_epoch.store(_epoch, std::memory_order_relaxed);
            // std::atomic_thread_fence(std::memory_order_seq_cst);
        }
        else
        {
            epoch_t _epoch = shadow_epoch.load(std::memory_order_relaxed);
            _ref_epoch.store(_epoch, std::memory_order_relaxed);
            std::atomic_signal_fence(std::memory_order_seq_cst);
        }
    }

    inline void unlock()
    {
        _ref_epoch.store(0, std::memory_order_release);
    }

    /**
     * Update ref_epoch if object is stale (being retire)
     * and has a newer epoch than current ref epoch.
     * 
     * Used for slow traversal
     */
    void update_epoch(smr_obj_base* obj)
    {
        epoch_t ref_epoch = _ref_epoch.load(std::memory_order_relaxed);
        if (ref_epoch == 0) 
            return; // not locked

        epoch_t pre_expiry = obj->pre_expiry.load(std::memory_order_relaxed);
        if (pre_expiry == 0)
            return;

        if (pre_expiry > ref_epoch)
            _ref_epoch.store(pre_expiry, std::memory_order_relaxed);
    }


    void print() {
        fprintf(stdout, "effective_epoch=%lu shadow=%lu, _ref=%lu\n",
            effective_epoch,
            shadow_epoch.load(std::memory_order_relaxed),
            _ref_epoch.load(std::memory_order_relaxed),
            1);
    }

};

class smrproxy
{
    epoch_t domain_epoch = 1;

    std::vector<smr_ref *> refs = std::vector<smr_ref *>();

    std::thread reclaim_task;

    std::mutex mutex;
    std::condition_variable_any cvar;
    std::chrono::milliseconds wait_ms;

    std::atomic_bool active{true};             //

    std::atomic<smr_obj_base *> tail = nullptr;     // retire queue
    std::vector<smr_obj_base *> defer_queue;        // ...

public:

    smrproxy(uint32_t wait_ms)
    {
        this->wait_ms = std::chrono::milliseconds(wait_ms);
        membarrier::_register();
        reclaim_task = std::thread([this] () { this->reclaim(); });
    }

    smrproxy() : smrproxy(50) {}

    ~smrproxy()
    {
        active.store(false);
        {
            std::scoped_lock m(mutex);
            cvar.notify_all();
        }

        reclaim_task.join();

        std::for_each(this->refs.begin(), this->refs.end(), [] (smr_ref* ref) { ref->print(); });
        refs.clear();
        try_reclaim();              // _try_reclaim?

        smr_obj_base* _tail = tail.exchange(nullptr);
        if (_tail != nullptr)
        {
            fprintf(stderr, "return queue not null\n");
                delete_objects(_tail);
        }
        delete_objects(_tail);
        if (!defer_queue.empty()) {
            fprintf(stderr, "defer queue size = %d\n", defer_queue.size());
            std::for_each(defer_queue.begin(), defer_queue.end(), [] (smr_obj_base* obj)
            {
                delete_objects(obj);
            });
            defer_queue.clear();
        }

    }

    smr_ref* acquire_ref() {
        std::scoped_lock m(mutex);

        smr_ref *ref = new smr_ref(domain_epoch);
        refs.push_back(ref);
        return ref;
    }

    void release_ref(smr_ref* ref) {
        std::scoped_lock m(mutex);

        std::erase_if(refs, [ref] (smr_ref* ref2) { return ref == ref2; });
    }

    void retire(smr_obj_base * data) {
        if (data == nullptr)
            return;

        epoch_t pre_expiry = std::atomic_ref(domain_epoch).load(std::memory_order_relaxed);   // TODO not actually atomic
        data->pre_expiry.store(pre_expiry, std::memory_order_relaxed);

        smr_obj_base* next;
        do {
            data->smr_obj_next = next = tail.load(std::memory_order_relaxed);
        } while (!tail.compare_exchange_weak(next, data, std::memory_order_release));

        if (next == nullptr)
        {
            // std::scoped_lock m(mutex);  // ?
            cvar.notify_all();
        }
    }

private:

    /**
     * delete linked list of objects
     * @param head head of null terminated linked list
     */
    static void delete_objects(smr_obj_base* head)
    {
        smr_obj_base* next = head;
        while (next != nullptr)
        {
            smr_obj_base* _obj = next;
            next = next->smr_obj_next;
            _obj->smr_obj_next = nullptr;
            delete _obj;
        }
    }

    static void set_expiry(smr_obj_base* head, epoch_t expiry)
    {
        smr_obj_base* next = head;
        while (next != nullptr)
        {
            next->expiry.store(expiry, std::memory_order_relaxed);
            next = next->smr_obj_next;
        }
    }


    /**
     * @brief try reclaim, mutex must be held
     * @return 
     */
    bool _try_reclaim() {
        smr_obj_base* _tail = tail.exchange(nullptr, std::memory_order_acquire);
        if (_tail != nullptr)
        {
            domain_epoch += 2;
            const epoch_t expiry = domain_epoch;

            // _tail->expiry.store(expiry, std::memory_order_relaxed);
            set_expiry(_tail, expiry);
            defer_queue.push_back(_tail);

            if constexpr (_smrproxy_mb)
            {
                std::atomic_thread_fence(std::memory_order_seq_cst);    // needed?
            }
            else
            {
                std::atomic_thread_fence(std::memory_order_seq_cst);
                membarrier::sync();
                std::atomic_thread_fence(std::memory_order_seq_cst);
            }
        }

        /*
        * set ref effective epochs and
        * find oldest referenced epoch
        */

        const epoch_t current_epoch = domain_epoch;
        epoch_t oldest = domain_epoch;


        std::for_each(this->refs.begin(), this->refs.end(), [current_epoch, &oldest] (smr_ref * ref) {
            ref->shadow_epoch.store(current_epoch, std::memory_order_relaxed);
            epoch_t ref_epoch = ref->_ref_epoch.load(std::memory_order_relaxed);
            if (ref_epoch == 0)
                ref->effective_epoch = current_epoch;                       // current epoch
            else if (ref_epoch > ref->effective_epoch) 
                ref->effective_epoch = ref_epoch;

            if (ref->effective_epoch < oldest)
                oldest = ref->effective_epoch;
        });

        /**
         * 
         */
        auto expired = [this, &oldest]  (smr_obj_base* obj)
        {
            if (obj->expiry.load(std::memory_order_relaxed) > oldest)     // obj->expiry > oldest
            {
                return false;                           // retain in defer_queue
            }
            else // expiry <= oldest
            {
                delete_objects(obj);
                return true;                            // remove from defer_queue
            }
        };

        std::erase_if(defer_queue, expired);

        return !defer_queue.empty();    // per ProxyType requirement
    }

    public:

    bool try_reclaim() {
        std::scoped_lock(this->mutex);
        return _try_reclaim();
    }

    void reclaim()
    {
        {
            std::scoped_lock m(mutex);

            while (active.load(std::memory_order_relaxed)) {
                if (_try_reclaim())
                    cvar.wait_for(mutex, wait_ms);
                else
                    cvar.wait(mutex);
            }

            _try_reclaim();
        }

    }

};


static_assert(ProxyType<smrproxy, smr_ref, smr_obj_base>, "smrproxy does not meet ProxyType requirement");


/*-*/