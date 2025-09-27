
#pragma once


#include <memory>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <string.h>
#include <tuple>
#include <stdexcept>

namespace pipe
{

constexpr size_t GETID(const char* name)
{
    size_t hash = 0x811c9dc5;
    size_t h1   = 0x959954B9;
    while (*name)
    {
        h1 = *name + (h1 << 5) + (h1 << 7) + (h1 << 17) - h1;
        hash ^= (unsigned char)*name++;
        hash *= 0x01000193;
    }

    return (hash | (h1 << 32)) & 0xFFFFFFFFFFFFFFFEULL | 0x1ULL;
}

#define GID_V(x) pipe::GETID(x)
#define GID(x) pipe::GETID(#x)

namespace ser {

typedef std::vector<uint8_t> data_t;

static void Read(size_t& offset, const data_t& var, void* ret, size_t size)
{
    if (var.size() < offset + size)
        throw(std::runtime_error("Read: buffer to small"));
    memcpy(ret, var.data() + offset, size);
    offset += size;
}

template<class type>
static void Read(size_t& offset, const data_t& var, type* ret) { Read(offset, var, ret, sizeof(type)); }

template<class type>
static void Read(size_t& offset, const data_t& var, type& ret) { Read(offset, var, &ret, sizeof(type)); }

template<class type>
static type Read(size_t& offset, const data_t& var) { type ret; Read(offset, var, &ret, sizeof(type)); return ret; }

template<class type>
static void DserRead(size_t& offset, const data_t& var, type* ret) { 
    data_t temp = data_t(var.begin() + offset, var.end());
    offset = offset + Deserialize(temp, ret);
}

static void Write(size_t& offset, data_t& ret, const void* var, const size_t& size)
{
    if (ret.size() < offset + size)
        ret.resize(offset + size, 0);

    memcpy(ret.data() + offset, var, size);
    offset += size;
}

template<class type>
static void Write(size_t& offset, data_t& ret, const type* var) { Write(offset, ret, var, sizeof(type)); }

template<class type>
static void Write(size_t& offset, data_t& ret, const type& var) { Write(offset, ret, &var, sizeof(type)); }

template<class type>
static void SerWrite(size_t& offset, data_t& ret, const type* var) { 
    data_t temp;
    Serialize(temp, var);
    Write(offset, ret, temp.data(), temp.size());
}

template <typename T>
struct SerHelper {
    static constexpr bool val = false;
    static size_t Serialize(data_t& ret, const void* var) { throw std::runtime_error("SerHelper:: try use undeclared Serialize"); }
};

template <typename T>
struct DserHelper {
    static constexpr bool val = false;
    static size_t Deserialize(const data_t& ret, void* var) { throw std::runtime_error("Deserialize:: try use undeclared Deserialize"); }
};

};
template <typename T>
struct Helper {
    static constexpr size_t ID() { static const auto v = GID_V(typeid(T).name()); return v; }
    static constexpr const char* NAME() { static const auto v = typeid(T).name(); return v; }
};

struct RawData
{
    typedef void type;
    RawData(size_t type, const char* name) : dataType(type), name(name) {}
    size_t      dataType = 0;
    const char* name     = "";
    virtual ~RawData() {};
    template<typename T>
    T* as()
    {
        if (dataType == Helper<T>::ID())
            return (T*)ptr();
        return nullptr;
    }
    virtual void* ptr() { return nullptr; };

    virtual size_t Serialize(ser::data_t& ret) const { return 0; }
    virtual size_t Deserialize(const ser::data_t& var) { return 0; }
};

template<typename T>
struct RawValue : public RawData
{
    static_assert(!std::is_same_v<bool, T>, "Data cannot store a \"bool\" value");
    typedef T type;
    RawValue() : RawData(Helper<T>::ID(), Helper<T>::NAME()) {}
    RawValue(const T& value) : RawData(Helper<T>::ID(), Helper<T>::NAME()), data(new T(value)) {}
    RawValue(T* value) : RawData(Helper<T>::ID(), Helper<T>::NAME()), data(value) {}
    inline static RawData* make(const T& _data) { return new RawValue<T>(_data); }
    inline static RawData* make(T* _data) { return new RawValue<T>(_data); }
    inline static RawData* make_empty() { return new RawValue<T>(); }
    T*                     data;
    virtual ~RawValue() { delete data; };
    virtual void* ptr() override { return data; }
    operator T&() { return data; }
    operator const T&() const { return *data; }
    template<bool A = typename std::enable_if<std::is_pointer<T>::value == true>::type* = nullptr>
    auto& operator->()
    {
        return **(T)data;
    }

    virtual size_t Serialize(ser::data_t& ret) const override
    {
        if constexpr (ser::SerHelper<T>::val)
            return ser::Serialize(ret, &data);
        else
            throw(std::runtime_error("Non Serializeble"));
    }
    virtual size_t Deserialize(const ser::data_t& var) override
    {
        if constexpr (ser::DserHelper<T>::val)
            return ser::Deserialize(var, &data);
        else
            throw(std::runtime_error("Non Serializeble"));
    }
};

struct __data_imp
{

private:
    struct __counter_t
    {
        std::atomic<size_t> data;
        __counter_t(const size_t& v = 1) : data(v)
        {
            // printf("create   0x%llx\n", this);
        }

        __counter_t* inc()
        {
            ++data;
            return this;
        }
        bool dec()
        {
            --data;
            return data.load() <= 0;
        }

        ~__counter_t()
        {
            // printf("~destroy 0x%llx\n", this);
        }
    };
    __counter_t* counter = nullptr;

    __data_imp(__counter_t* counter, RawData* data) : counter(counter), data(data) {}

public:
    __data_imp() = default;

    template<typename T>
    __data_imp(const T& value) : counter(new __counter_t)
    {
        if constexpr (std::is_base_of<RawData, T>::value)
            data = new T(value);
        else
            data = RawValue<T>::make(value);
    }

    template<typename T>
    static __data_imp make_from_ptr(T* value)
    {
        return {
            new __counter_t(),
            RawValue<T>::make(value)
        };
    }

    __data_imp(const nullptr_t*& value) : counter(nullptr), data(nullptr) {}
    __data_imp(const __data_imp& _)
    {
        counter = (_.counter ? _.counter->inc() : nullptr);
        data    = _.data;
        deallocator = _.deallocator;
    }

    __data_imp& operator=(const __data_imp& _)
    {
        counter = (_.counter ? _.counter->inc() : nullptr);
        data    = _.data;
        deallocator = _.deallocator;
        return *this;
    }

    __counter_t* dec()
    {
        if (counter)
        {
            if (counter->dec())
            {
                if(deallocator)
                {
                    if(deallocator(data->ptr()))
                        delete data;
                }
                else
                    delete data;
                data = nullptr;
                delete counter;
                counter = nullptr;
            }
        }
        return counter;
    }

    template<typename T>
    void reset(const T& value)
    {
        dec();
        counter     = new __counter_t;
        data        = RawValue<T>::make(value);
    }

    template<typename T>
    void reset(T* value)
    {
        dec();
        counter     = new __counter_t;
        data        = RawValue<T>::make(value);
    }

    RawData* data  = nullptr;
    std::function<bool(void*)> deallocator = nullptr;

    ~__data_imp()
    {
        dec();
    }
};

struct defautl_deallocator { bool operator()(void*){return true;}};

template<typename T, typename DEALLOCATOR = nullptr_t>
struct Pointer
{
private:
    __data_imp imp;
    friend class Data;
    Pointer(__data_imp& _) : imp(_) {}

public:
    Pointer() {imp.deallocator = DEALLOCATOR(); }
    Pointer(Pointer& _) : imp(_.imp) {}
    Pointer(Pointer&& _) : imp(_.imp) {}
    Pointer(const Pointer& _) : imp(_.imp) {}
    Pointer& operator=(const Pointer& _)
    {
        imp = _.imp;
        return *this;
    }

    Pointer<T>& SetDeallocator(std::function<bool(void*)> deallocator)
    {
        imp.deallocator = deallocator;
        return *this;
    }

    Pointer(T* value) : imp(__data_imp::make_from_ptr<T>(value)) { imp.deallocator = DEALLOCATOR(); }

    void reset(T* value) { imp.reset<T>(value); imp.deallocator = DEALLOCATOR(); }

    operator T*()
    {
        return (T*)imp.data->ptr();
    }

    operator const T*() const
    {
        return (T*)imp.data->ptr();
    }
    
    T* operator->()
    {
        return ((T*)imp.data->ptr());
    }

    bool has() const { return imp.data != nullptr; }
};

struct Data
{
private:
    __data_imp imp;

public:
    Data() = default;
    Data(Data& _) : imp(_.imp) {}
    Data(Data&& _) : imp(_.imp) {}
    Data(const Data& _) : imp(_.imp) {}
    Data(const Data&& _) : imp(_.imp) {}

    template<typename T>
    Data(Pointer<T>& _) : imp(_.imp)
    {}
    template<typename T>
    Data(Pointer<T>&& _) : imp(_.imp)
    {}
    template<typename T>
    Data(const Pointer<T>& _) : imp(_.imp)
    {}
    template<typename T>
    Data(const Pointer<T>&& _) : imp(_.imp)
    {}

    Data& operator=(const Data& _)
    {
        imp = _.imp;
        return *this;
    }

    template<typename T>
    Data& operator=(const Pointer<T>& _)
    {
        imp = _.imp;
        return *this;
    }

    template<typename T>
    Data(const T& value, void* owner = nullptr) : imp(value), sender(owner)
    {}
    Data(const nullptr_t*& value, void* owner = nullptr) : imp(value), sender(owner) {}

    Data& SetDeallocator(std::function<bool(void*)> deallocator)
    {
        imp.deallocator = deallocator;
        return *this;
    }

    template<typename T>
    void reset(const T& value, void* owner = nullptr)
    {
        imp.reset<T>(value);
        sender = owner;
    }

    template<typename T>
    T* as()
    {
        if (imp.data == nullptr)
            return nullptr;
        return imp.data->as<T>();
    }
    template<typename T>
    const T* as() const
    {
        if (imp.data == nullptr)
            return nullptr;
        return imp.data->as<T>();
    }
    template<typename T>
    bool is()
    {
        if (imp.data == nullptr)
            return false;
        if constexpr (std::is_base_of<RawData, T>::value)
            return imp.data->as<typename T::type>();
        else
            return imp.data->as<T>();
    }


    template<typename T, typename std::enable_if<!std::is_same<T, bool>::value>::type* = nullptr>
    operator T&()
    {
        if (imp.data == nullptr)
            throw std::runtime_error(std::string("Trying to get a value from non-initialized data."));
        auto t = imp.data->as<T>();
        if (t)
            return *t;
        throw std::runtime_error(std::string("Trying to get an incorrect type from data. Current: ") + imp.data->name + ". Requested: " + Helper<T>::NAME() + ".");
    }

    template<typename T>
    operator const T&() const
    {
        if constexpr (std::is_same_v<T, bool>)
            return imp.data != nullptr;
        else
        {
            if (imp.data == nullptr)
                throw std::runtime_error(std::string("Trying to get a value from non-initialized data."));
            auto t = imp.data->as<T>();
            if (t)
                return *t;
            throw std::runtime_error(std::string("Trying to get an incorrect type from data. Current: ") + imp.data->name + ". Requested: " + Helper<T>::NAME() + ".");
        }
    }


    template<typename T>
    operator Pointer<T>()
    {
        if (is<T>())
            return Pointer<T>(imp);
        else
            return Pointer<T>(nullptr)
    }

    template<typename T, typename = std::enable_if_t<std::is_pointer<T>::value>>
    auto& operator->()
    {
        return imp.data.operator->();
    }

    template<typename T>
    bool to(T& v) const
    {
        if constexpr (std::is_same_v<T, bool>)
            v = (imp.data != nullptr);
        else
        {
            if (imp.data == nullptr)
                return nullptr;
            auto t = imp.data->as<T>();
            if (!t)
                return false;
            v = *t;
        }
        return true;
    }

    bool        has() const { return imp.data != nullptr; }
    size_t      Serialize(ser::data_t& ret) const { return imp.data->Serialize(ret); }
    ser::data_t Serialize() const
    {
        ser::data_t ret;
        imp.data->Serialize(ret);
        return std::move(ret);
    }

    size_t Deserialize(const ser::data_t& var)
    {
        if (!imp.data)
        {
            throw("try deserealize type without knowed constructor");
        }
        return imp.data->Deserialize(var);
    }

    void* sender = nullptr;
};


template<typename T>
static constexpr size_t ID()
{
    return Helper<T>::ID();
}

namespace ser
{
template<class T>
static size_t Serialize(data_t& ret, const T* var) { if constexpr (SerHelper<T>::val) return SerHelper<T>::Serialize(ret, var); return 0; }

template<class T>
static size_t Deserialize(const data_t& var, T* ret) {if constexpr (DserHelper<T>::val) return DserHelper<T>::Deserialize(var, ret); return 0; }

template <typename... VALS>
static auto __make_tuple_helper__(VALS& ... vals) { return std::tuple(&vals...); } 
};


template<typename T, typename T1, typename... Ts>
constexpr size_t TGETID()
{
    auto t = TGETID<T1, Ts...>() ^ ((0xC5965A3CAA556695 >> ((sizeof...(Ts) * 31) % 64)) | (0xC5965A3CAA556695 >> (64 - ((sizeof...(Ts) * 31) % 64))));
    return (Helper<T>::ID() ^ ((t << 11) | (t >> 53))) & 0xFFFFFFFFFFFF0000ULL;
}

template<typename T>
constexpr size_t TGETID()
{
    return Helper<T>::ID() & 0xFFFFFFFFFFFF0000ULL;
}


#if 1 // DECLARE_DATA_TYPE defines

#define TGID(...) pipe::TGETID<__VA_ARGS__>()

#define __I(...) __VA_ARGS__


template<typename Ret, typename... Vars> 
struct Helper<Ret(Vars...)>
{
    static constexpr size_t ID() { return pipe::TGETID<Ret, Vars...>(); };
    static constexpr const char* NAME() { return Helper<Ret>::NAME(); }
};

#define DECLARE_DATA_TYPE(T) template<> struct pipe::Helper<T> {\
    static constexpr size_t ID() { return GID(T); } \
    static constexpr const char* NAME() { return #T; } \
}; template<> struct pipe::Helper<const T> {\
    static constexpr size_t ID() { return GID(T); } \
    static constexpr const char* NAME() { return #T; }\
};

#define DECLARE_DATA_TEMPLATE_TYPE(T, tf, ts) template<tf> struct pipe::Helper<T<ts>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(ts); } \
    static constexpr const char* NAME() { return #T; } \
}; template<tf> struct pipe::Helper<const T<ts>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(ts); } \
    static constexpr const char* NAME() { return #T; } \
};

#define DECLARE_DATA_AUTO_TEMPLATE_TYPE(T) template<typename... Ts> struct pipe::Helper<T<Ts...>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(Ts...); } \
    static constexpr const char* NAME() { return #T; } \
}; template<typename... Ts> struct pipe::Helper<const T<Ts...>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(Ts...); } \
    static constexpr const char* NAME() { return #T; } \
};

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_FUNC(T) \
namespace pipe{namespace ser{static size_t SerializeImp(data_t&, const T*);};}; \
template<> struct pipe::ser::SerHelper<T> { static constexpr bool val = true; static size_t Serialize(data_t& ret, const void* var) { return pipe::ser::SerializeImp(ret, (const T*) var); }}; \
static size_t pipe::ser::SerializeImp(data_t& serealized, const T* void_deserealized)

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_DESERIALIZER_FUNC(T) \
namespace pipe{namespace ser{static size_t DeserializeImp(const data_t&, T*);};}; \
template<> struct pipe::ser::DserHelper<T> { static constexpr bool val = true; static size_t Deserialize(const data_t& ret, void* var) { return pipe::ser::DeserializeImp(ret, (T*) var); }}; \
static size_t pipe::ser::DeserializeImp(const data_t& serealized, T* void_deserealized)

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_TEMPLATE_FUNC(T, tf, ts) \
template<tf> struct pipe::ser::SerHelper<T<ts>> { static constexpr bool val = true; \
    static size_t Serialize(data_t& ret, const void* var); \
}; \
template<tf> size_t pipe::ser::SerHelper<T<ts>>::Serialize(data_t& serealized, const void* void_deserealized)

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(T, tf, ts)\
template<tf> struct pipe::ser::DserHelper<T<ts>> { static constexpr bool val = true; \
    static size_t Deserialize(const data_t& var, void* ret); \
}; \
template<tf> size_t pipe::ser::DserHelper<T<ts>>::Deserialize(const data_t& serealized, void* void_deserealized)

#define DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(T) const T* deserealized = (const T*)void_deserealized;\
if(deserealized == 0) throw std::invalid_argument("empty pointer"); \
size_t offset = 0; \
Write(offset, serealized, pipe::Helper<T>::ID())

#define DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(T)  T* deserealized = (T*)void_deserealized;\
if(deserealized == 0) throw std::invalid_argument("empty pointer");\
size_t offset = 0;\
if(Read<size_t>(offset, serealized) != pipe::Helper<T>::ID()) throw std::invalid_argument("uncorrect uid")

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_DEFAULT(T) DECLARE_DATA_SERIALIZER_FUNC(T) {\
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(T);\
    Write(offset, serealized, deserealized);\
    return offset; \
}
// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_DESERIALIZER_DEFAULT(T) DECLARE_DATA_DESERIALIZER_FUNC(T) {\
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(T);\
    Read(offset, serealized, deserealized);\
    return offset;\
}
// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_DESERIALIZER_DEFAULT(T) DECLARE_DATA_SERIALIZER_DEFAULT(T); DECLARE_DATA_DESERIALIZER_DEFAULT(T);

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_TEMPLATE_DEFAULT(T, tf, ts) DECLARE_DATA_SERIALIZER_FUNC(T, tf, ts) {\
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(T<ts>);\
    Write(offset, serealized, deserealized);\
    return offset; \
}

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_DESERIALIZER_TEMPLATE_DEFAULT(T, tf, ts) DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(T, tf, ts) {\
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(T<ts>);\
    Read(offset, serealized, deserealized);\
    return offset;\
}
// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_DESERIALIZER_TEMPLATE_DEFAULT(T, tf, ts) DECLARE_DATA_SERIALIZER_TEMPLATE_DEFAULT(T, tf, ts); DECLARE_DATA_DESERIALIZER_TEMPLATE_DEFAULT(T, tf, ts);

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(T) DECLARE_DATA_TYPE(T); DECLARE_DATA_SERIALIZER_DESERIALIZER_DEFAULT(T);

#endif
} // namespace pipe

template<typename... ARGS>
using Ref = pipe::Pointer<ARGS...>;

template<typename To, typename From>
constexpr static To* RefCast(Ref<From>& v)
{
    return dynamic_cast<To*>((From*)(v));
}
using Data = pipe::Data;

DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(long long);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(long);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(int);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(short);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(char);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(unsigned long long);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(unsigned long);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(unsigned int);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(unsigned short);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(unsigned char);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(float);
DECLARE_DATA_TYPE_AND_DEFAULT_SERIALIZER(double);

DECLARE_DATA_TYPE(void*);
DECLARE_DATA_TYPE(nullptr_t);

DECLARE_DATA_TYPE(pipe::Data);

DECLARE_DATA_TEMPLATE_TYPE(std::shared_ptr, __I(typename T), __I(T));
DECLARE_DATA_TEMPLATE_TYPE(std::unique_ptr, __I(typename T), __I(T));
DECLARE_DATA_TEMPLATE_TYPE(std::weak_ptr, __I(typename T), __I(T));


DECLARE_DATA_SERIALIZER_FUNC(pipe::Data) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(pipe::Data);
    data_t temp ;
    deserealized->Serialize(temp);
    Write(offset, serealized, temp.data(), temp.size());
    return offset;
}
DECLARE_DATA_DESERIALIZER_FUNC(pipe::Data) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(pipe::Data);
    data_t temp = data_t(serealized.begin() + offset, serealized.end());
    offset += deserealized->Deserialize(temp);
    return offset;
}

// std::string

DECLARE_DATA_TYPE(std::string);
DECLARE_DATA_SERIALIZER_FUNC(std::string) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(std::string);
    Write(offset, serealized, (int)deserealized->size());
    Write(offset, serealized, deserealized->data(), deserealized->size());
    return offset;
}
DECLARE_DATA_DESERIALIZER_FUNC(std::string) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(std::string);
    deserealized->resize(Read<int>(offset, serealized));
    Read(offset, serealized, deserealized->data(), deserealized->size());
    return offset;
}

// std::vector

DECLARE_DATA_TEMPLATE_TYPE(std::vector, __I(typename T), __I(T))
DECLARE_DATA_SERIALIZER_TEMPLATE_FUNC(std::vector, __I(typename T), __I(T)) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(std::vector<T>);
    Write(offset, serealized, (int)deserealized->size());
    for(int i = 0; i < deserealized->size(); ++i)
        SerWrite(offset, serealized, deserealized->data() + i);
    return offset;
}
DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(std::vector, __I(typename T), __I(T)) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(std::vector<T>);
    deserealized->resize(Read<int>(offset, serealized));
    for(int i = 0; i < deserealized->size(); ++i)
        DserRead(offset, serealized, deserealized->data() + i);
    return offset;
}

// std::list

DECLARE_DATA_TEMPLATE_TYPE(std::list, __I(typename T), __I(T))
DECLARE_DATA_SERIALIZER_TEMPLATE_FUNC(std::list, __I(typename T), __I(T)) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(std::list<T>);
    Write(offset, serealized, (int)deserealized->size());
    for(auto el& : deserealized)
        SerWrite(offset, serealized, &el);
    return offset;
}
DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(std::list, __I(typename T), __I(T)) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(std::list<T>);
    deserealized->resize(Read<int>(offset, serealized));
    for(auto el& : deserealized)
        DserRead(offset, serealized, deserealized->data() + i);
    return offset;
}

// std::unordered_map

DECLARE_DATA_TEMPLATE_TYPE(std::unordered_map, __I(typename KEY, typename VALUE), __I(KEY, VALUE))
DECLARE_DATA_SERIALIZER_TEMPLATE_FUNC(std::unordered_map, __I(typename KEY, typename VALUE), __I(KEY, VALUE)) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(std::unordered_map<__I(KEY, VALUE)>);
    Write(offset, serealized, (int)deserealized->size());
    for(auto el& : deserealized)
    {
        SerWrite(offset, serealized, &el.first);
        SerWrite(offset, serealized, &el.second);
    }
    return offset;
}
DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(std::unordered_map, __I(typename KEY, typename VALUE), __I(KEY, VALUE)) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(std::list<T>);
    deserealized->resize(Read<int>(offset, serealized));
    for(auto el& : deserealized)
        DserRead(offset, serealized, deserealized->data() + i);
    return offset;
}

// std::tuple

DECLARE_DATA_AUTO_TEMPLATE_TYPE(std::tuple)
DECLARE_DATA_SERIALIZER_TEMPLATE_FUNC(std::tuple, __I(typename... ARGS), __I(ARGS...)) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(std::tuple<ARGS...>);
    Write(offset, serealized, (int)sizeof...(ARGS));
    std::apply([&] (auto&&... args) {
		(SerWrite(offset, serealized, &args), ...);
	}, *deserealized);
    return offset;
}

DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(std::tuple, __I(typename... ARGS), __I(ARGS...)) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(std::tuple<ARGS...>);
    if(Read<int>(offset, serealized) != (int)sizeof...(ARGS)) throw std::invalid_argument("uncorrect size");
    std::apply([&] (auto&&... args) 
    {
        (DserRead(offset, serealized, &args), ...);
        }, *deserealized);
    return offset;
}

// struct

#define DECLARE_DATA_STRUCT_AND_SERIALIZE(name, ...) DECLARE_DATA_TYPE(name); \
DECLARE_DATA_SERIALIZER_FUNC(name) { DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(name); \
    struct __HELPER__ : name { using tuple_type = decltype(std::tuple(__VA_ARGS__)); auto make_tuple() const { return std::tuple(__VA_ARGS__); }  }; \
    Write(offset, serealized, pipe::Helper<__HELPER__::tuple_type>::ID()); \
    Write(offset, serealized, (int)(std::tuple_size_v<__HELPER__::tuple_type>)); \
    auto tpl = ((const __HELPER__*)deserealized)->make_tuple();  \
    std::apply([&] (auto&&... args) { (SerWrite(offset, serealized, &args), ...); }, tpl); \
    return offset; \
} DECLARE_DATA_DESERIALIZER_FUNC(name) { DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(name); \
    struct __HELPER__ : name { using tuple_type = decltype(std::tuple(__VA_ARGS__)); auto make_tuple() { return ser::__make_tuple_helper__(__VA_ARGS__); } }; \
    if(Read<size_t>(offset, serealized) != pipe::Helper<__HELPER__::tuple_type>::ID()) throw std::invalid_argument("uncorrect tuple uid"); \
    if(Read<int>(offset, serealized) != std::tuple_size_v<__HELPER__::tuple_type>) throw std::invalid_argument("uncorrect tuple size"); \
    auto tpl = ((__HELPER__*)deserealized)->make_tuple(); \
    std::apply([&] (auto&&... args) { (DserRead(offset, serealized, args), ...); }, tpl); \
    return offset; \
}