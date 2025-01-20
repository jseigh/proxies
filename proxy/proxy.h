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

#include <concepts>
#include <type_traits>
// #include <utility>

template<typename T>
concept BasicLockable = requires(T a)
{
    { a.lock() } -> std::same_as<void>;
    { a.unlock() } -> std::same_as<void>;
};


// using dtor = void (*)(void *);

/**
 * @tparm T proxy collector type
 * @tparm R shared lock reference
 * @tparm B base object type for managed objects
 */
template<typename T, typename R, typename B>
concept ProxyType = requires(T proxy, R* ref, B * obj) 
{
    BasicLockable<R>;
    std::has_virtual_destructor<B>::value;                  // base object must have virtual destructor

    { proxy.acquire_ref() } -> std::same_as<R*>;            // acquire shared lock reference
    { proxy.release_ref(ref) } -> std::same_as<void>;       // release shared lock reference
    { proxy.retire(obj) } -> std::same_as<void>;            // reclaim object when it is no longer referenced
};

/**
 * @tparam B 
 * @tparam T 
 */
template<typename B, typename T>
requires std::has_virtual_destructor<B>::value
class managed_obj : public B, public T
{
public:
    using T::T;
};

/*
* template testing classes
*/

class proto_obj_base
{
public:
    virtual ~proto_obj_base() {}
};

class proto_proxy
{
public:
    // static constexpr bool retire_is_lockfree = false;
    // static constexpr bool retire_is_synchronous = false;

    /**
     * @brief get shared_lock
     */
    proto_proxy* acquire_ref() { return this; }
    void release_ref(proto_proxy* shared) {}
    
    /**
     * Proxy lock
     */

    // void lock() {}

    /**
     * Proxy unlock
     */
    // void unlock() {}

    /**
     * Retire object.
     * Object will be reclaimed once there are no
     * more references to it.
     * @param obj object to be retired
     */
    void retire(proto_obj_base* obj) {}

};

static_assert(ProxyType<proto_proxy, proto_proxy, proto_obj_base>, "protoproxy does not meet ProxyType requirement");

/*-*/