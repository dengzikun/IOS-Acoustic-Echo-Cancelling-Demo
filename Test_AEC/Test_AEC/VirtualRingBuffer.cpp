//
//  VirtualRingBuffer.cpp
//  Test_AEC
//
//  Created by ddp on 16/7/13.
//  Copyright © 2016年 ddp. All rights reserved.
//

#include <mach/mach.h>
#include <iostream>
#include "VirtualRingBuffer.hpp"

#define reportResult(result,operation) (_reportResult((result),(operation),strrchr(__FILE__, '/')+1,__LINE__))
static inline bool _reportResult(kern_return_t result, const char *operation, const char* file, int line) {
    if ( result != ERR_SUCCESS ) {
        std::cout << file << ":" << line << operation << ": " << mach_error_string(result) << std::endl;
        return false;
    }
    return true;
}

VirtualRingBuffer::~VirtualRingBuffer(){
    vm_deallocate(mach_task_self(), (vm_address_t)_buffer, _length * 2);
}

bool VirtualRingBuffer::init(int32_t length) {
    
    assert(length > 0);
    
    // Keep trying until we get our buffer, needed to handle race conditions
    int retries = 3;
    while ( true ) {
        
        _length = (int32_t)round_page(length);    // We need whole page sizes
        
        // Temporarily allocate twice the length, so we have the contiguous address space to
        // support a second instance of the buffer directly after
        vm_address_t bufferAddress;
        kern_return_t result = vm_allocate(mach_task_self(),
                                           &bufferAddress,
                                           _length * 2,
                                           VM_FLAGS_ANYWHERE); // allocate anywhere it'll fit
        if ( result != ERR_SUCCESS ) {
            if ( retries-- == 0 ) {
                reportResult(result, "Buffer allocation");
                return false;
            }
            // Try again if we fail
            continue;
        }
        
        // Now replace the second half of the allocation with a virtual copy of the first half. Deallocate the second half...
        result = vm_deallocate(mach_task_self(),
                               bufferAddress + _length,
                               _length);
        if ( result != ERR_SUCCESS ) {
            if ( retries-- == 0 ) {
                reportResult(result, "Buffer deallocation");
                return false;
            }
            // If this fails somehow, deallocate the whole region and try again
            vm_deallocate(mach_task_self(), bufferAddress, _length);
            continue;
        }
        
        // Re-map the buffer to the address space immediately after the buffer
        vm_address_t virtualAddress = bufferAddress + _length;
        vm_prot_t cur_prot, max_prot;
        result = vm_remap(mach_task_self(),
                          &virtualAddress,   // mirror target
                          _length,    // size of mirror
                          0,                 // auto alignment
                          0,                 // force remapping to virtualAddress
                          mach_task_self(),  // same task
                          bufferAddress,     // mirror source
                          0,                 // MAP READ-WRITE, NOT COPY
                          &cur_prot,         // unused protection struct
                          &max_prot,         // unused protection struct
                          VM_INHERIT_DEFAULT);
        if ( result != ERR_SUCCESS ) {
            if ( retries-- == 0 ) {
                reportResult(result, "Remap buffer memory");
                return false;
            }
            // If this remap failed, we hit a race condition, so deallocate and try again
            vm_deallocate(mach_task_self(), bufferAddress, _length);
            continue;
        }
        
        if ( virtualAddress != bufferAddress+_length ) {
            // If the memory is not contiguous, clean up both allocated buffers and try again
            if ( retries-- == 0 ) {
                std::cout << "Couldn't map buffer memory to end of buffer" << std::endl;
                return false;
            }
            
            vm_deallocate(mach_task_self(), virtualAddress, _length);
            vm_deallocate(mach_task_self(), bufferAddress, _length);
            continue;
        }
        
        _buffer = (uint8_t*)bufferAddress;
        return true;
    }
    return false;
}