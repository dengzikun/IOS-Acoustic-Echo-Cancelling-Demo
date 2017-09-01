#ifndef SYNC_QUEUE_H__
#define SYNC_QUEUE_H__

#include <stdlib.h>

#include <mutex>
#include <condition_variable>

#include <vector>
#include <list>
#include <deque>
#include <memory>
#include <chrono>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////////////////////
// class CBuffer start

class CBuffer
{
    
private:
    static inline void* aligned_malloc(size_t size, size_t alignment)
    {
        // 检查alignment是否是2^N
        assert(!(alignment & (alignment - 1)));
        // 计算出一个最大的offset，sizeof(void*)是为了存储原始指针地址
        size_t offset = sizeof(void*) + (--alignment);
        
        // 分配一块带offset的内存
        char* p = static_cast<char*>(malloc(offset + size));
        if (!p) return nullptr;
        
        // 通过“& (~alignment)”把多计算的offset减掉
        void* r = reinterpret_cast<void*>(reinterpret_cast<size_t>(p + offset) & (~alignment));
        // 将r当做一个指向void*的指针，在r当前地址前面放入原始地址
        static_cast<void**>(r)[-1] = p;
        // 返回经过对齐的内存地址
        return r;
    }
    
   static inline void aligned_free(void* p)
    {  
        // 还原回原始地址，并free  
        free(static_cast<void**>(p)[-1]);  
    }
public:
	CBuffer(const CBuffer &refBuffer)=delete;
	CBuffer &operator= (const CBuffer &refBuffer)=delete;

	CBuffer()
		:_data(nullptr)
		,_dataSize(0)
		,_dataType(0)
		,_dataAlignmentBytes(4)
		, _prefixSize(16)
		, _buffer(nullptr, aligned_free)
	{
	}

	bool Alloc(size_t BufferSize, size_t DataAlignmentBytes=4, size_t PrefixSize=16)
	{
		_prefixSize = PrefixSize;
		_buffer.reset(aligned_malloc(BufferSize + _prefixSize, DataAlignmentBytes));

        
		if (!_buffer)
		{
			_data = nullptr;
			_bufferSize = 0;
			return false ;
		}
		_data = (unsigned char*)_buffer.get() + _prefixSize;
		_bufferSize = BufferSize ;
		_dataAlignmentBytes = DataAlignmentBytes ;
		return true ;
	}

	~CBuffer()
	{
	}

	const size_t& BufferSize ()
	{
		return _bufferSize ;
	}

	bool CopyFrom ( void *pSrc, size_t size )
	{
		if (_bufferSize < size)
		{
			if (!ReAlloc(size))
			{
				_dataSize = 0 ;
				return false ;
			}
		}

		memcpy(_data, pSrc, size );
		_dataSize = size;
		return true ;
	}
    
    void CopyTo ( void *pDst )
    {
        memcpy(pDst, _data, _dataSize );
    }
    
    bool ReAlloc( size_t size )
	{
		_buffer.reset(aligned_malloc(size + _prefixSize, _dataAlignmentBytes));
		if (!_buffer)
		{
			_data = nullptr;
			return false;
		}
		_data = (unsigned char*)_buffer.get() + _prefixSize;
		_bufferSize = size;
		return true;
	}

public:
	size_t  _dataSize ;
	int32_t _dataType ;
	void *_data ;
	int32_t _width;
	int32_t _height;
	int64_t _pts ;
	size_t _bufferSize ;
	int    _flag;
private:

	size_t _dataAlignmentBytes;
	size_t _prefixSize;
	std::unique_ptr<void, void(*)(void*)> _buffer;
} ;

// class CBuffer end
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
// class ObjectManager start

template<typename T>
class ObjectManager
{
public:
	ObjectManager() = delete;
	ObjectManager(const ObjectManager &refObjectManager) = delete;
	ObjectManager &operator= (const ObjectManager &refObjectManager) = delete;
};

template <typename T>
class ObjectManager<T*>
{
public:
	ObjectManager(const ObjectManager &refObjectManager) = delete;
	ObjectManager &operator= (const ObjectManager &refObjectManager) = delete;

	ObjectManager(size_t count)
	{
		for(int i = 0; i < count; ++i){
			_freeObject.push_back(new T());
		}

	}

	~ObjectManager()
	{
		for(auto *info : _freeObject){
			delete info;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	T* Producer_Get(uint32_t wait_time = 0)
	{
		std::unique_lock<std::mutex> lock(_freeMutex);
		bool not_empty = _freeCV.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{return !_freeObject.empty(); });
		if (not_empty)
		{
			T* p = _freeObject.back();
			_freeObject.pop_back();
			return p;
		}
		return nullptr;
	}

	void Producer_Put(T *p)
	{
		std::lock_guard<std::mutex> lock(_busyMutex);
		_busyObject.push_back(p);
		_busyCV.notify_one();
	}

	std::size_t BusySize(){
		return _busyObject.size();
	}

	T* Consumer_Get(uint32_t wait_time = 0)
	{
		std::unique_lock<std::mutex> lock(_busyMutex);
		bool not_empty = _busyCV.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{return !_busyObject.empty(); });
		if (not_empty)
		{
			T *p = _busyObject.front();
			_busyObject.pop_front();
			return p;
		}

		return nullptr;
	}

	void Consumer_Put(T* p)
	{
		std::lock_guard<std::mutex> lock(_freeMutex);
		_freeObject.push_back(p);
		_freeCV.notify_one();
	}

	void ReleaseProducerWaiting()
	{
		Consumer_Put(nullptr);
	}

	void ReleaseConsumerWaiting()
	{
		Producer_Put(nullptr);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////


private:
	std::vector<T*> _freeObject;
	std::list<T*>	_busyObject;
	std::mutex _freeMutex;
	std::condition_variable _freeCV;

	std::mutex _busyMutex;
	std::condition_variable _busyCV;
public:
	long width, height;
};

template<>
class ObjectManager<CBuffer*>
{
public:
	ObjectManager(const ObjectManager &refObjectManager) = delete;
	ObjectManager &operator= (const ObjectManager &refObjectManager) = delete;
	ObjectManager() = delete;

	ObjectManager(size_t BufferCount, size_t BufferSize, size_t DataAlignmentBytes = 4)
	{
		if (BufferCount == 0 || BufferSize == 0 || DataAlignmentBytes == 0 || (DataAlignmentBytes % 2) != 0)
		{
			return;
		}

//		_buffer = std::make_unique<CBuffer[]>(BufferCount);
		_buffer.reset(new CBuffer[BufferCount]);

		for (size_t i = 0; i < BufferCount; i++)
		{
			if (_buffer[i].Alloc(BufferSize, DataAlignmentBytes))
			{
				_freeObject.push_back(&_buffer[i]);
			}
			else
			{
				return;
			}
		}
	}

	~ObjectManager()
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	CBuffer* Producer_Get(uint32_t wait_time = 0)
	{
		std::unique_lock<std::mutex> lock(_freeMutex);
		bool not_empty = _freeCV.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{return !_freeObject.empty(); });
		if (not_empty)
		{
			CBuffer* p = _freeObject.back();
			_freeObject.pop_back();
			return p;
		}
		return nullptr;
	}

	void Producer_Put(CBuffer* p)
	{
		std::lock_guard<std::mutex> lock(_busyMutex);
		_busyObject.push_back(p);
		_busyCV.notify_one();
	}

	CBuffer* Consumer_Get(uint32_t wait_time = 0)
	{
		std::unique_lock<std::mutex> lock(_busyMutex);
		bool not_empty = _busyCV.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{return !_busyObject.empty(); });
		if (not_empty)
		{
			CBuffer* p = _busyObject.front();
			_busyObject.pop_front();
			return p;
		}

		return nullptr;
	}

	void Consumer_Put(CBuffer* p)
	{
		std::lock_guard<std::mutex> lock(_freeMutex);
		_freeObject.push_back(p);
		_freeCV.notify_one();
	}

	void Consumer_Clear(){
		std::lock(_busyMutex, _freeMutex);
		std::lock_guard<std::mutex> lock1(_busyMutex, std::adopt_lock);
		std::lock_guard<std::mutex> lock2(_freeMutex, std::adopt_lock);
		while(!_busyObject.empty()){
			CBuffer* p = _busyObject.front();
			_busyObject.pop_front();
			_freeObject.push_back(p);
		}
	}

	void ReleaseProducerWaiting()
	{
		Consumer_Put(nullptr);
	}

	void ReleaseConsumerWaiting()
	{
		Producer_Put(nullptr);
	}

	std::size_t BusySize(){
		return _busyObject.size();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////


private:
	std::vector<CBuffer*> _freeObject;
	std::list<CBuffer*>	_busyObject;
	std::mutex _freeMutex;
	std::condition_variable _freeCV;

	std::mutex _busyMutex;
	std::condition_variable _busyCV;

	std::unique_ptr<CBuffer[]> _buffer;

public:
		long width, height;
};

typedef ObjectManager<CBuffer*> BufferManager ;


// class ObjectManager end
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
class DequeBlocking
{

};

template <typename T>
class DequeBlocking<T*>
{
public:
	DequeBlocking(const DequeBlocking &refObjectManager) = delete;
	DequeBlocking &operator= (const DequeBlocking &refObjectManager) = delete;

	DequeBlocking()
	{
	}

	~DequeBlocking()
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////



	void Put(T *p)
	{
		std::lock_guard<std::mutex> lock(_busyMutex);
		_busyObject.push_back(p);
		_busyCV.notify_one();
	}

	std::size_t BusySize(){
		return _busyObject.size();
	}

	T* Get(uint32_t wait_time = 0)
	{
		std::unique_lock<std::mutex> lock(_busyMutex);
		bool not_empty = _busyCV.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{return !_busyObject.empty(); });
		if (not_empty)
		{
			T *p = _busyObject.front();
			_busyObject.pop_front();
			return p;
		}

		return nullptr;
	}

	void ReleaseWaiting()
	{
		Put(nullptr);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////


private:

	std::deque<T*>	_busyObject;
	std::mutex _busyMutex;
	std::condition_variable _busyCV;
};

template <>
class DequeBlocking<ssize_t>
{
public:
	DequeBlocking(const DequeBlocking &refObjectManager) = delete;
	DequeBlocking &operator= (const DequeBlocking &refObjectManager) = delete;

	DequeBlocking()
	{
	}

	~DequeBlocking()
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////



	void Put(ssize_t p)
	{
		std::lock_guard<std::mutex> lock(_busyMutex);
		_busyObject.push_back(p);
		_busyCV.notify_one();
	}

	std::size_t BusySize(){
		return _busyObject.size();
	}

	ssize_t Get(uint32_t wait_time = 0)
	{
		std::unique_lock<std::mutex> lock(_busyMutex);
		bool not_empty = _busyCV.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{return !_busyObject.empty(); });
		if (not_empty)
		{
			ssize_t p = _busyObject.front();
			_busyObject.pop_front();
			return p;
		}

		return -1;
	}

	void ReleaseWaiting()
	{
		Put(-1);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////


private:

	std::deque<ssize_t>	_busyObject;
	std::mutex _busyMutex;
	std::condition_variable _busyCV;
};

#endif