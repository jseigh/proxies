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

#include <type_traits>
#include <atomic>
#include <stdint.h>


#ifndef EPOCH_NO_WRAP
    struct epoch_t {
        // using zero_t = std::integral_constant<int, 0>;
    private:
        uint64_t value = 0;
    public:
        epoch_t() : value(0)  {}
        // epoch_t(zero_t) : value(0) {}
        epoch_t(uint64_t value) : value(value) {}
        int64_t operator <=> (const epoch_t rhd) { return (value - rhd.value); }
        epoch_t & operator += (uint64_t inc) { value += inc; return *this; }
        epoch_t & operator -= (uint64_t dec) { value -= dec; return *this; }
        // bool operator != (zero_t) { return value != 0; }
        // bool operator == (zero_t) { return value == 0; }
        operator uint64_t() { return value; }

        epoch_t operator +(uint64_t inc) { return epoch_t(value + inc); }
        epoch_t operator -(uint64_t dec) { return epoch_t(value - dec); }
    };
    
#else
    using epoch_t = uint64_t;
#endif

static_assert(std::is_trivially_copyable_v<epoch_t>);
static_assert(std::is_copy_constructible_v<epoch_t>);
static_assert(std::is_move_constructible_v<epoch_t>);
static_assert(std::is_copy_assignable_v<epoch_t>);
static_assert(std::is_move_assignable_v<epoch_t>);

static_assert(std::atomic<epoch_t>::is_always_lock_free);
