//
//  VirtualRingBuffer.hpp
//  Test_AEC
//
//  Created by ddp on 16/7/13.
//  Copyright © 2016年 ddp. All rights reserved.
//

#ifndef VirtualRingBuffer_hpp
#define VirtualRingBuffer_hpp

#include <assert.h>
#include <atomic>
#include <mutex>
#include <condition_variable>

class VirtualRingBuffer{
public:
    VirtualRingBuffer(int32_t size){
        init(size);
    }
    ~VirtualRingBuffer();
    
    uint8_t* GetReadPointer(int32_t* availableBytes) {
        *availableBytes = _fillCount;
        if ( *availableBytes == 0 ) return nullptr;
        return _buffer + _tail;
    }
    void DidRead(int32_t amount) {
        _tail = (_tail + amount) % _length;
        _fillCount -= amount;
        assert(_fillCount >= 0);
    }
    
    bool ReadWait(uint32_t wait_time){
        std::unique_lock<std::mutex> lock(_writeMutex);
        bool wake = _writeCV.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{return _fillCount != 0; });
        return wake;
    }
    
    
    uint8_t* GetWritePointer(int32_t* availableBytes) {
        *availableBytes = _length - _fillCount;
        if ( *availableBytes == 0 ) return nullptr;
        return _buffer + _head;
    }
    void DidWrite(int32_t amount) {
        _head = (_head + amount) % _length;
        _fillCount += amount;
        assert(_fillCount <= _length);
        
        std::unique_lock<std::mutex> lock(_writeMutex);
        _writeCV.notify_one();
    }
    
    void Reset(){
        int32_t fillCount;
        if (GetReadPointer(&fillCount)) {
            DidRead(fillCount);
        }
    }
    
private:
    bool init(int32_t length);
    
private:
    uint8_t           *_buffer = nullptr;
    int32_t           _length = 0;
    int32_t           _tail = 0;
    int32_t           _head = 0;
    std::atomic_int _fillCount = {0};
    
    std::mutex _writeMutex;
    std::condition_variable _writeCV;
};

#endif /* VirtualRingBuffer_hpp */
