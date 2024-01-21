#pragma once
// BSD 3-Clause License
//
// Copyright (c) 2022, Arm Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <exception>

// Project's base exception class used to capture errors and business logic flow.
class wperf_exception : public std::exception
{
public:
    wperf_exception(const char* msg) : exception_msg(msg) {}
    virtual const char* what() const throw() { return exception_msg; }
private:
    const char* exception_msg;
};

// We throw this in general CLI / IOCTL error
class fatal_exception : public wperf_exception
{
public:
    fatal_exception(const char* msg) : wperf_exception(msg) {}
};

// We throw this exception when we fail to acquire driver resources
class locked_exception : public wperf_exception
{
public:
    locked_exception(const char* msg) : wperf_exception(msg) {}
};

// We throw this exception when driver denies us IOCTL aka someone kicked us with --force-lock.
class lock_denied_exception : public wperf_exception
{
public:
    lock_denied_exception(const char* msg) : wperf_exception(msg) {}
};
