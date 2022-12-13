#pragma once
#include <functional>

namespace Coroutine 
{
    class Defer 
    {
    public:
        explicit Defer(std::function<void()> && func) 
        {
            _func = std::move(func);
        }

        Defer(const Defer&) = delete;
        Defer(Defer&&) = default;
        Defer& operator = (const Defer&) = delete;
        Defer& operator = (Defer&&) = delete;

        ~Defer() 
        {
            _func();
        }

    private:
        std::function<void()> _func;
    };

    
}
#define defer(func) auto Defer##__COUNTER__##_smarlab_private = Coroutine::Defer(func)