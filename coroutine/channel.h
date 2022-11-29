#pragma once
#include "mutex.h"
#include <queue>
#include <list>
#include "task.h"

namespace Coroutine 
{
    template<typename _Ty>
    class Channel
    {
    public:
        Channel(int capacity = 1) : _capacity(capacity), _active(true)
        {
            //暂时不支持 空间为0 的情况。后面支持
            if (_capacity < 1)
                _capacity = 1;      
        }

        ~Channel() {

        }
        // 不希望 Channel 被移动或者复制
		Channel(Channel&& channel) = delete;
		Channel(Channel&) = delete;
		Channel& operator=(Channel&) = delete;

        bool IsClose() {
			return !_active.load(std::memory_order_relaxed);
		}

        void Close()
        {
            _active.store(false,std::memory_order_relaxed);
            std::unique_lock lock(_mtx);
            for (auto write : _writes)
            {
                write->Resume();
            }
            _writes.clear();
            for (auto read : _reads)
            {
                read->Resume();
            }
            _reads.clear();
        }

        bool CanWrite()
        {
            std::unique_lock lck(_block);
            return _buffer.size() < _capacity;
        }

        bool CanRead()
        {
            std::unique_lock lck(_block);
            return _buffer.size() > 0;
        }

        void Write(const _Ty& elem)
        {
            if (IsClose())
                return;
            std::unique_lock lck(_block);
            while (_buffer.size() >= _capacity)
            {
                Task* current = ProcessInstance()->Self();
                current->Yield([&](){
                    std::unique_lock guard(_mtx);
                    _writes.push_back(current);
                    lck.unlock();
                });
                if (IsClose())
                    return;
                lck.lock();
            }
            _buffer.push(elem);
            ResumeRead();
        }

        const _Ty & operator >> (_Ty& elem) 
        {
            Read(elem);
            return elem;
        }

        Channel<_Ty>& operator << (const _Ty& elem) 
        {
            Write(elem);
            return *this;
        }

        void ResumeWrite()
        {
            std::unique_lock lock(_mtx);
            if (!_writes.empty())
            {
                Task* write = _writes.front();
                _writes.pop_front();
                write->Resume();
            }
        }

        void Read(_Ty& elem)
        {
            if (IsClose())
                return;
            
            std::unique_lock lck(_block);
            while (_buffer.empty())
            {
                // //如果capacity = 0，可能有write队列
                // if (_capacity == 0)
                //     ResumeWrite();
                
                Task* current = ProcessInstance()->Self();
                current->Yield([&](){
                    std::unique_lock guard(_mtx);
                    _reads.push_back(current);
                    lck.unlock();
                });
                if (IsClose())
                    return;
                lck.lock();
            }
            elem = _buffer.front();
            _buffer.pop();
            //判断写队列
            if (_capacity != 0)
                ResumeWrite();
        }

        void ResumeRead()
        {
            std::unique_lock guard(_mtx);
            if (!_reads.empty())
            {
                Task* read = _reads.front();
                _reads.pop_front();
                read->Resume();
            }
        }

        using LST_TASK = std::list<Task*>;
    private:
        size_t _capacity;
        Spinlock _block;        //buff lock
		std::queue<_Ty> _buffer;

        LST_TASK _reads;
        LST_TASK _writes;
        Mutex _mtx;
		std::atomic<bool> _active;
    };
}
