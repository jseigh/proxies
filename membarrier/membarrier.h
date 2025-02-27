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
#include <mutex>

#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/membarrier.h>

#define MB_REGISTER  MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED
#define MB_SYNC      MEMBARRIER_CMD_PRIVATE_EXPEDITED

    static std::once_flag _registered;


class membarrier
{
public:
    // static std::once_flag _registered;

    static int _membarrier(int cmd, unsigned int flags, int cpu_id)
    {
        return syscall(__NR_membarrier, cmd, flags, cpu_id);
    }


    static void _register()
    {
        _membarrier(MB_REGISTER, 0, 0);
    }

    static void sync()
    {
        // std::call_once(_registered, _register);
        _membarrier(MB_SYNC, 0, 0);
    }

};

static_assert(std::is_empty_v<membarrier>);

/*==*/