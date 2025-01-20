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

#include<mutex>
#include<shared_mutex>


#include "../include/proxy.h"

class shared_obj_base
{
public:
    virtual ~shared_obj_base() {}
};


class sharedproxy
{
    using rlock_t = std::shared_lock<std::shared_mutex>;

    std::shared_mutex& _rwlock;

public:

    sharedproxy(std::shared_mutex &rwlock) : _rwlock(rwlock) {}

    rlock_t* acquire_ref() { rlock_t* rlock =  new rlock_t(_rwlock); rlock->unlock(); return rlock; }
    void release_ref(rlock_t *rlock) { delete rlock; }
    void retire(shared_obj_base * data) { delete data; }
};

static_assert(ProxyType<sharedproxy, std::shared_lock<std::shared_mutex>, shared_obj_base>, "sharedproxy does not meet ProxyType requirement");


class mutexproxy
{
    std::mutex* _mutex;

public:

    mutexproxy(std::mutex *mutex) : _mutex(mutex) {}

    std::mutex* acquire_ref() { return _mutex; }
    void release_ref(std::mutex* mutex) {}
    void retire(shared_obj_base * data) { delete data; }
};

static_assert(ProxyType<mutexproxy, std::mutex, shared_obj_base>, "mutexproxy does not meet ProxyType requirement");

class noopproxy
{
public:

    noopproxy() {};
    inline void lock() { std::atomic_thread_fence(std::memory_order_acquire); }
    inline void unlock() { std::atomic_thread_fence(std::memory_order_release); }

    noopproxy* acquire_ref() { return this; }
    void release_ref(noopproxy* proxy) {}
    void retire(shared_obj_base * data) { delete data; }
};

static_assert(ProxyType<noopproxy, noopproxy, shared_obj_base>, "noopproxy does not meet ProxyType requirement");

/*=*/