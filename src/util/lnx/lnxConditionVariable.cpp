/*
 ***********************************************************************************************************************
 *
 *  Copyright (c) 2014-2018 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 **********************************************************************************************************************/

#include "palConditionVariable.h"
#include "palMutex.h"
#include "palSysMemory.h"
#include "util/lnx/lnxTimeout.h"
#include <errno.h>

namespace Util
{

// =====================================================================================================================
// Frees the pthreads condition variable this object encapsulates.
ConditionVariable::~ConditionVariable()
{
    pthread_cond_destroy(&m_osCondVariable);
}

// =====================================================================================================================
// Initializes the pthreads condition variable this object encapsulates.  Returns ErrorUnknown if the pthread
// initialization call fails.
Result ConditionVariable::Init()
{
    Result result = Result::ErrorUnknown;

    if (pthread_cond_init(&m_osCondVariable, nullptr) == 0)
    {
        result = Result::Success;
    }

    return result;
}

// =====================================================================================================================
// Atomically releases the given mutex object and goes to sleep on the condition variable.  Once we awake from this
// sleep, reacquire the critical section.  Returns false if the specified number of milliseconds elapse before it is
// awoken.
bool ConditionVariable::Wait(
    Mutex* pMutex,
    uint32 milliseconds)  // Can be set to 0xFFFFFFFF to wait forever.
{
    bool result = false;

    if (pMutex != nullptr)
    {
        Mutex::MutexData*const pOsMutex  = pMutex->GetMutexData();
        pthread_cond_t*const   pOsCndVar = &m_osCondVariable;

        constexpr uint32 Infinite = 0xFFFFFFFF;
        if (milliseconds == Infinite)
        {
            // Wait on the condition variable indefinitely.
            const int32 ret = pthread_cond_wait(pOsCndVar, pOsMutex);
            PAL_ASSERT(ret == 0);

            result = true;
        }
        else
        {
            timespec timeout = {};
            ComputeTimeoutExpiration(&timeout, milliseconds * 1000 * 1000);

            // Wait on the condition variable until a timeout occurs.
            const int32 ret = pthread_cond_timedwait(pOsCndVar, pOsMutex, &timeout);
            PAL_ASSERT((ret == 0) || (ret == ETIMEDOUT));

            result = (ret == 0);
        }
    }

    return result;
}

// =====================================================================================================================
// Wakes up one thread that is waiting on this condition variable.
void ConditionVariable::WakeOne()
{
    const int32 ret = pthread_cond_signal(&m_osCondVariable);
    PAL_ASSERT(ret == 0);
}

// =====================================================================================================================
// Wakes up all threads that are waiting on this condition variable.
void ConditionVariable::WakeAll()
{
    const int32 ret = pthread_cond_broadcast(&m_osCondVariable);
    PAL_ASSERT(ret == 0);
}

} // Util
