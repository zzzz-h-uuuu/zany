#ifndef ZANY_HPP
#define ZANY_HPP


// Compiler detection

#if defined(__GNUC__) 
    #define ZANY_COMPILER_GCC 1
#else
    #error "zany currently supports GCC only."
#endif

// Platform detection

#if defined(_WIN32) || defined(_WIN64)
    #define ZANY_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define ZANY_PLATFORM_LINUX 1
#else
    #error "zany currently supports Windows and Linux only."
#endif

// Shared library symbol visibility

// Define ZANY_BUILD_SHARED when building zany as a shared library.
// Define ZANY_EXPORTS when compiling the zany library itself.

#if defined(ZANY_BUILD_SHARED)

    #if defined(ZANY_PLATFORM_WINDOWS)
        #if defined(ZANY_EXPORTS)
            #define ZANY_API __attribute__((dllexport))
        #else
            #define ZANY_API __attribute__((dllimport))
        #endif

    #elif defined(ZANY_PLATFORM_LINUX)
        #define ZANY_API __attribute__((visibility("default")))
    #endif

#else
    #define ZANY_API
#endif

// Inline

#define ZANY_FORCE_INLINE inline __attribute__((always_inline))

// Debug

#if defined(NDEBUG)
    #define ZANY_DEBUG 0
#else
    #define ZANY_DEBUG 1
#endif

// GCC diagnostic helpers

#define ZANY_DO_PRAGMA(x) _Pragma(#x)

#define ZANY_DIAGNOSTIC_PUSH \
    ZANY_DO_PRAGMA(GCC diagnostic push)

#define ZANY_DIAGNOSTIC_POP \
    ZANY_DO_PRAGMA(GCC diagnostic pop)

#define ZANY_DIAGNOSTIC_IGNORE(warning) \
    ZANY_DO_PRAGMA(GCC diagnostic ignored warning)

// Version

#define ZANY_VERSION_MAJOR 1
#define ZANY_VERSION_MINOR 0
#define ZANY_VERSION_PATCH 0

#define ZANY_VERSION \
    (ZANY_VERSION_MAJOR * 10000 + \
     ZANY_VERSION_MINOR * 100 + \
     ZANY_VERSION_PATCH)

#endif // ZANY_HPP
#include <string>
#include <stdexcept>
#include <new>
#include <initializer_list>
#include <cstddef>
#include <cstdlib>
#include <typeinfo>
#include <cxxabi.h>
#include<iostream>
#include<atomic>
#include<any>



//use for get readable type information(Only GNU/GCC)
inline std::string readable_type(const char* mangled) 
{
    int status = 0;
    char* p = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    std::string result = (status == 0 && p) ? p : mangled;
    std::free(p);
    return result;
}

namespace anyc
{
    template<class _Tp,class _Obj>
    concept constructor = requires
    {
        requires (!std::is_same_v<std::decay_t<_Tp>, std::decay_t<_Obj>>);
    };


    template<class _Tp , class _Storage>
    concept is_internal = requires
    {
        requires sizeof(_Tp) <= sizeof(_Storage);
        requires alignof(_Tp) <= alignof(_Storage);
        std::is_nothrow_move_constructible_v<_Tp>;
    };

    template<class _Tp>
    concept is_only_move = requires
    {
        requires std::is_move_constructible_v<_Tp>;
        requires (!std::is_copy_constructible_v<_Tp>);
    };
    
    template<class _Tp,class...Args>
    concept able_to_emplace = 
        std::is_constructible_v<_Tp, Args...>
        &&
        std::is_move_constructible_v<_Tp>; 
    

    template<class _Tp>
    concept valid_to_cast = std::is_reference_v<_Tp>||std::is_copy_constructible_v<_Tp>;

    template<class _Tp>
    concept valid_to_cast_r = std::is_reference_v<_Tp>||std::is_move_constructible_v<_Tp>;
    
};

static int id = 0;

class Only_move
{
    public:
    Only_move() :i{++id}{};

    Only_move(const Only_move&) = delete;

    Only_move(Only_move&& rval)
    {
        i = rval.i;
        std::cout<<"only_move is moved!"<<std::endl;
    }

    ~Only_move(){std::cout<<"only_move is called!"<<i<<std::endl;}

    void get_id()
    {
        std::cout<<"id : "<<i<<std::endl;
    }
    private:  

    int i;
};

class Only_move_l
{
    public:
    Only_move_l() :i{++id}{};

    Only_move_l(const Only_move_l&) = delete;

    Only_move_l(Only_move_l&& rval)
    {
        i = rval.i;
        std::cout<<"only_move is moved!"<<std::endl;
    }

    ~Only_move_l(){std::cout<<"only_move is called!"<<i<<std::endl;}

    void get_id()
    {
        std::cout<<"id : "<<i<<std::endl;
    }
    private:  
    std::string s;
    int i;
};

template<class _Tp>
constexpr size_t obj_offset()
{
    constexpr size_t raw = sizeof(std::atomic<int>);
    constexpr size_t align = alignof(_Tp);
    return (raw + align - 1) /align * align; 
} 

template<class _Tp>
constexpr size_t max_align()
{
    return alignof(std::atomic<int>) < alignof(_Tp) ? alignof(_Tp) : alignof(std::atomic<int>);
}
class zany
{
    union Storage  
    {
        constexpr Storage() : heap{} {}

        // Prevent trivial copies of this type, buffer might hold a non-POD.
        Storage(const Storage&) = delete;
        Storage& operator=(const Storage&) = delete;

        struct Heap 
        {
            constexpr Heap():_M_ptr(nullptr), counter(nullptr){}
            template<class _Tp, class...Args>
            Heap(std::in_place_type_t<_Tp>, Args&&...args)
            {
                constexpr size_t offset = obj_offset<_Tp>();
                constexpr size_t total = offset + sizeof(_Tp);
                constexpr size_t align = max_align<_Tp>();
                void* raw = ::operator new(total,std::align_val_t(align));
                auto* c = new(static_cast<unsigned char*>(raw)) std::atomic<int> {1};
                try  
                {
                    _M_ptr = new(static_cast<unsigned char*>(raw) + offset) _Tp (std::forward<Args>(args)...);
                }
                catch(...)
                {
                    c->~atomic<int>();
                    ::operator delete(raw, std::align_val_t(align));
                    throw;  
                }
                counter = c;

            }
            std::atomic<int>*counter;
            void* _M_ptr;

        }heap;

        alignas(std::max_align_t) unsigned char _M_buffer[sizeof(heap)];// or on stack

    };

    template<class _Tp>
     struct inter_copy_manager;
    
    template<class _Tp>
     struct exter_copy_manager;

    template<class _Tp>
     struct inter_move_manager;

    template<class _Tp>
     struct exter_move_manager;

    template<class _Tp>
    using _sbo_cond_for_m = std::conditional_t<anyc::is_internal<_Tp, Storage>, inter_move_manager<_Tp>, exter_move_manager<_Tp>>;
    
    template<class _Tp>
    using _sbo_cond_for_c = std::conditional_t<anyc::is_internal<_Tp, Storage>, inter_copy_manager<_Tp>, exter_copy_manager<_Tp>>;

    template<class T,class _Tp = std::decay_t<T>>
    using _manager = std::conditional_t<anyc::is_only_move<_Tp>, _sbo_cond_for_m<_Tp>, _sbo_cond_for_c<_Tp>>;

    enum _Op {
	_Op_access, _Op_clone, _Op_destroy, _Op_xfer
    };

    union _Arg
    {
        void* _M_obj;
        zany* _M_any;
    };

    void (*_M_manager)(_Op, const zany*, _Arg*);

    Storage _M_storage;

    template<class _Tp>
    struct exter_move_manager
    {
    static void _s_manager(_Op which,const zany* __any, _Arg* __arg)//use function pointer to manage different operation on different object type.
    {
        auto __ptr = reinterpret_cast<_Tp*>(__any->_M_storage.heap._M_ptr);
        auto __counter = __any->_M_storage.heap.counter;
        switch(which)
        {
            case(_Op_access):
            {
                __arg->_M_obj = const_cast<_Tp*>(__ptr);
                break;
            }
            case(_Op_clone)://copy __any to __arg->any
            {
                __counter->fetch_add(1,std::memory_order_relaxed);//relaxed add 1 when being copied
                __arg->_M_any->_M_manager = __any->_M_manager;
                __arg->_M_any->_M_storage.heap._M_ptr = __ptr;
                __arg->_M_any->_M_storage.heap.counter = __counter;
                break;
            }
            case(_Op_xfer):
            {
                __arg->_M_any->_M_storage.heap._M_ptr = __ptr;
                __arg->_M_any->_M_manager = __any->_M_manager;
                __arg->_M_any->_M_storage.heap.counter = __counter;
                auto mut_any = const_cast<zany*>(__any);
                mut_any->_M_manager = nullptr;
                mut_any->_M_storage.heap._M_ptr = nullptr;
                mut_any->_M_storage.heap.counter = nullptr;
                break;
            }

            case(_Op_destroy):
            {
                if (__counter->fetch_sub(1,std::memory_order_release) == 1)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    constexpr size_t align = max_align<_Tp>();
                    __ptr->~_Tp();
                    ::operator delete(__counter, std::align_val_t(align));
                }
                break;
            }
        }
    };

    template<class T>
    static void _s_create(Storage& _storage,T&& t)
    {
        _storage.heap = Storage::Heap(std::in_place_type<_Tp>,std::forward<T>(t));
    }

    template<class... Args>
    static void _s_create(Storage& _storage,Args&&...args)
    {
        _storage.heap = Storage::Heap(std::in_place_type<_Tp>,std::forward<Args>(args)...);
    }

    static _Tp* _s_access(const Storage& _storage)
    {
        return static_cast<_Tp*>(_storage.heap._M_ptr);
    }
    };

    template<class _Tp>
    struct inter_move_manager
    {
        static void _s_manager(_Op which, const zany* __any, _Arg* __arg)//use function ptr to manage different operation on different object type.
        {
            auto __ptr = reinterpret_cast<const _Tp*>(&__any->_M_storage._M_buffer);
            switch (which)
            {
                case(_Op_access):
                {
                    __arg->_M_obj = const_cast<_Tp*>(__ptr);//allow getting pointer
                    break;
                }
                case(_Op_destroy):
                {
                    if(__any->has_value())
                    {
                        __ptr->~_Tp();
                    }
                    break;
                }
                case(_Op_clone):
                {
                    std::string e_msg = "try copying move-only type: ";
                    e_msg.append("\"").append(readable_type(typeid(_Tp).name())).append("\"");
                    throw(std::runtime_error(e_msg));
                }
                case(_Op_xfer):
                {
                    ::new(&__arg->_M_any->_M_storage._M_buffer) _Tp(std::move(*const_cast<_Tp*>(__ptr)));
                    __ptr->~_Tp();
                    __arg->_M_any->_M_manager = __any->_M_manager;
                    auto mut_any = const_cast<zany*>(__any);
                    mut_any->_M_manager = nullptr;
                    break;
                }
            }
        }

        template<class T>
        static void _s_create(Storage& _storage,T&& t)
        {
            void* __addr = &_storage._M_buffer;
            ::new(__addr) _Tp(std::forward<T>(t));
        }

        template<class... Args>
        static void _s_create(Storage& _storage,Args&&...args)
        {
            ::new(&_storage._M_buffer) _Tp(std::forward<Args>(args)...);
        }
     
        static _Tp* _s_access(const Storage& _storage)
        {
            const void* __address = &_storage._M_buffer;
            return static_cast<_Tp*>(const_cast<void*>(__address));
        }
    };

    template<class _Tp>
    struct exter_copy_manager
    {
        static void _s_manager(_Op which,const zany* __any, _Arg* __arg)//use function ptr to manage different operation on different object type.
        {
            auto __ptr = reinterpret_cast<_Tp*>(__any->_M_storage.heap._M_ptr);
            switch(which)
            {
                case(_Op_access):
                {
                    __arg->_M_obj = const_cast<_Tp*>(__ptr);
                    break;
                }
                case(_Op_clone):
                {
                    __arg->_M_any->_M_manager = __any->_M_manager;
                    __arg->_M_any->_M_storage.heap._M_ptr = new _Tp(*__ptr);
                    break;
                }
                case(_Op_xfer):
                {
                    __arg->_M_any->_M_storage.heap._M_ptr = __ptr;
                    __arg->_M_any->_M_manager = __any->_M_manager;
                    const_cast<zany*>(__any)->_M_manager = nullptr;
                    break;
                }

                case(_Op_destroy):
                {
                    if(__any->has_value())
                    {
                        __ptr->~_Tp();
                    }
                    break;
                }
            }
        }

        template<class T>
        static void _s_create(Storage& _storage,T&& t)
        {
            _storage.heap._M_ptr = new _Tp(std::forward<T>(t));
        }

        template<class... Args>
        static void _s_create(Storage& _storage,Args&&...args)
        {
            _storage.heap._M_ptr = new _Tp(std::forward<Args>(args)...);
        }

        static _Tp* _s_access(const Storage& _storage)
        {
            return static_cast<_Tp*>(_storage.heap._M_ptr);
        }
    };

    template<class _Tp>
    struct inter_copy_manager
    {
        static void _s_manager(_Op which,const zany* __any, _Arg* __arg)//use function ptr to manage different operation on different object type.
        {
            auto __ptr = reinterpret_cast<const _Tp*>(__any->_M_storage._M_buffer);
            switch (which)
            {
                case(_Op_access):
                {
                    __arg->_M_obj = const_cast<_Tp*>(__ptr);
                    break;
                }
                case(_Op_destroy):
                {
                    if(__any->has_value())
                    {
                        __ptr->~_Tp();
                    }
                    break;
                }
                case(_Op_clone):
                {
                    ::new(&__arg->_M_any->_M_storage._M_buffer) _Tp(*__ptr);
                    __arg->_M_any->_M_manager = __any->_M_manager;
                    break;
                }
                case(_Op_xfer):
                {
                    ::new(&__arg->_M_any->_M_storage._M_buffer) _Tp(std::move(*__ptr));
                    __ptr->~_Tp();
                    __arg->_M_any->_M_manager = __any->_M_manager;
                    auto mut_any = const_cast<zany*>(__any);
                    mut_any->_M_manager = nullptr;
                    break;
                }

            }
        }

        template<class T>
        static void _s_create(Storage& _storage,T&& t)
        {
            void* __addr = &_storage._M_buffer;
            ::new(__addr) _Tp(std::forward<T>(t));
        }

        template<class... Args>
        static void _s_create(Storage& _storage,Args&&...args)
        {
            ::new(&_storage._M_buffer) _Tp(std::forward<Args>(args)...);
        }
     
        static _Tp* _s_access(const Storage& _storage)
        {
            const void* __address = &_storage._M_buffer;
            return static_cast<_Tp*>(const_cast<void*>(__address));
        }
    };

    void reset()
    {
        if (has_value())
        {
            _M_manager(_Op_destroy, this, nullptr);
            _M_manager = nullptr;
        }
    }

        template<class _vTp,class...Args, class Msg = _manager<_vTp>>
    void _do_emplace(Args&&... arg)
    {
        reset();
        Msg::_s_create(_M_storage,std::forward<Args>(arg)...);
        _M_manager = Msg::_s_manager;
    }

    template<class _vTp,class _Up,class...Args,class Msg = _manager<_vTp>>
    void _do_emplace(std::initializer_list<_Up> _li,Args&&...arg)
    {
        reset();
        Msg::_s_create(_M_storage,_li,std::forward<Args>(arg)...);
        _M_manager = Msg::_s_manager;
    }

    public:

    template<typename _Tp>
      friend void* _any_caster(const zany* __any);

    bool has_value() const noexcept { return _M_manager != nullptr; }

    constexpr zany()noexcept :_M_manager(nullptr){}

    template<class _Tp, class _uTp = std::decay_t<_Tp>>
    requires (!std::is_same_v<_uTp, zany>)
    zany(_Tp&& t) : _M_manager(nullptr)
    {
        using _Mgr = _manager<_uTp>;
        _M_manager = &_Mgr::_s_manager;
        _Mgr::_s_create(_M_storage,std::forward<_Tp>(t));
    }

    template<class _Tp,class...Args>
    zany(std::in_place_type_t<_Tp>, Args&&...args):_M_manager(nullptr)
    {
        using _Mgr = _manager<std::decay_t<_Tp>>;
        _M_manager = &_Mgr::_s_manager;
        _Mgr::_s_create(_M_storage,std::forward<Args>(args)...);
    }

    zany(const zany& __other)
    {
        if (!__other.has_value())
            _M_manager = nullptr;
        else
        {
            _Arg __arg;
            __arg._M_any = this;
            try  
            {
                __other._M_manager(_Op_clone, &__other, &__arg);
            }
            catch(const std::exception& error)
            {
                std::cerr<<error.what()<<" from zany(const zany& __other)"<<std::endl;
                throw;
            }
        }
    }

    zany(zany&& __other) noexcept
    {
        if (!__other.has_value())
            _M_manager = nullptr;
        else
        {
            _Arg __arg;
            __arg._M_any = this;
            __other._M_manager(_Op_xfer, &__other, &__arg);
        }
    }

    ~zany()noexcept {reset();}

    template<class _Tp, class _vTp = std::decay_t<_Tp>,  class...Args>
    requires anyc::able_to_emplace<_vTp, Args...>
    _vTp& emplace(Args&&...arg)
    {
        _do_emplace<_vTp>(std::forward<Args>(arg)...);
        return *zany::_manager<_vTp>::_s_access(_M_storage);
    }
    
    template<class _Tp, class _Up, class...Args, typename _vTp = std::decay_t<_Tp>>
    requires anyc::able_to_emplace<_vTp,std::initializer_list<_Up>, Args...>
    _vTp& emplace(std::initializer_list<_Up> v, Args&&... arg)
    {
        _do_emplace(v,std::forward<Args>(arg)...);
        return *zany::_manager<_vTp>::_s_access(_M_storage);
    }

     inline void swap(zany& other) noexcept
    {
        if(!has_value() && !other.has_value())
        {
            return;
        }
        if(has_value() && other.has_value())
        {
            if(this == &other)
                return;
            _Arg __arg;
            zany temp;
            __arg._M_any = &temp;
            _M_manager(_Op_xfer,this,&__arg);
            __arg._M_any = this;
            other._M_manager(_Op_xfer,&other,&__arg);
            __arg._M_any = &other;
            temp._M_manager(_Op_xfer,&temp,&__arg);
        }
        else  
        {
            zany* empty = has_value() ? &other : this;
            zany* full = has_value() ? this : &other;
            _Arg __arg;
            __arg._M_any = empty;
            full->_M_manager(_Op_xfer,full,&__arg);
        }
    }

    inline static void swap(zany& left,zany& right) noexcept
    {
        left.swap(right);
    }

    zany& operator=(const zany& value)
    {
        *this = zany(value);   
        return *this;
    }
    

    zany& operator=(zany&& __rhs) noexcept
    {
        if (!__rhs.has_value())
        reset();
        else if (this != &__rhs)
        {
        reset();
        _Arg __arg;
        __arg._M_any = this;
        __rhs._M_manager(_Op_xfer, &__rhs, &__arg);
        }
        return *this;
    }
};

template<class _Tp,class...Args>
requires anyc::able_to_emplace<_Tp,Args...>
inline zany make_any(Args&&...args)
{
    return zany(std::in_place_type<_Tp>,std::forward<Args>(args)...);
}

template<class _Tp,class _Up,class...Args>
requires anyc::able_to_emplace<_Tp,std::initializer_list<_Up>,Args...>
inline zany make_any(std::initializer_list<_Up> list,Args&&...args)
{
    return zany(std::in_place_type<_Tp>,list,std::forward<Args>(args)...);
}

template<class _Tp>
requires anyc::valid_to_cast_r<_Tp>
inline _Tp any_cast(zany&& any)
{
    using _Up = std::remove_cvref_t<_Tp>;
    static_assert(std::is_constructible_v<_Up, _Tp>,"Template argument must be constructible from an rvalue.");
    auto ptr = any_cast<_Up>(&any);
    if(ptr)
        return static_cast<_Tp>(std::move(*ptr));
    throw std::bad_any_cast{};
}

template<class _Tp>
inline const _Tp* any_cast(const zany* any) noexcept
{
    static_assert(! std::is_void_v<_Tp>);

    if constexpr (std::is_object_v<_Tp>)
        return static_cast<_Tp*>(_any_caster<_Tp>(any));

    return nullptr;
}

template<class _Tp>
inline _Tp* any_cast(zany* any) noexcept
{
    static_assert(! std::is_void_v<_Tp>);

    if constexpr (std::is_object_v<_Tp>)
        return static_cast<_Tp*>(_any_caster<_Tp>(any));

    return nullptr;
}
template<class _Tp>
requires anyc::valid_to_cast<_Tp>
inline _Tp any_cast(zany& any)
{
    using _Up = std::remove_cvref_t<_Tp>;
    static_assert(std::is_constructible_v<_Up, _Tp>,"Template argument must be constructible from an lvalue.");
    auto ptr = any_cast<_Up>(&any);
    if(ptr)
        return static_cast<_Tp>(*ptr);
    throw std::bad_any_cast{};
}

template<class _Tp>
void* _any_caster(const zany* any)
{
    using Msg = zany::_manager<std::remove_cv_t<_Tp>>;
    if constexpr (!std::is_same_v<std::decay_t<_Tp>, _Tp>&& ! std::is_same_v<const std::decay_t<_Tp>, _Tp>)
        return nullptr;
    if(any ==nullptr)
        return nullptr;
    else if (any->_M_manager == &Msg::_s_manager )//rtti
        return static_cast<_Tp*>(Msg::_s_access(any->_M_storage));
    auto ptr = &Msg::_s_manager;
    auto ptr2 = any->_M_manager;
    return nullptr;
}

