
#pragma once


#include <memory>
#include <cstdio>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <string.h>
#include <tuple>
#include <list>
#include <stdexcept>

namespace data_core
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

#define GID_V(x) data_core::GETID(x)
#define GID(x) data_core::GETID(#x)

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
template<class T>
static size_t Serialize(data_t& ret, const T* var);

template<class T>
static size_t Deserialize(const data_t& var, T* ret);

template<class type>
struct imp_t {};
};

template <typename T>
struct SerHelper {
    static constexpr bool val = false;
};
template <typename T>
struct Helper {
    static size_t ID() { static const auto v = GID_V(typeid(T).name()); return v; }
    static const char* NAME() { static const auto v = typeid(T).name(); return v; }
};
template <typename T>
struct ConstructorHelper { };

class TypeConverter {
private:
    static std::unordered_map<size_t, std::unordered_map<size_t, std::function<bool(const void*, void*)>>>& get_converters() {
        static std::unordered_map<size_t, std::unordered_map<size_t, std::function<bool(const void*, void*)>>> converters;
        return converters;
    }

public:
    template<typename From, typename To>
    static bool RegisterConverter() {
        auto& converters = get_converters();
        converters[Helper<From>::ID()][Helper<To>::ID()] = 
            [](const void* from, void* to) {
                try {
                    To& result = *static_cast<To*>(to);
                    const From& source = *static_cast<const From*>(from);
                    result = (To)(source);
                    return true;
                }
                catch(...) {
                    return false;
                }
            };
        return true;
    }

    template<typename From, typename To>
    static bool RegisterConverter(std::function<bool(const From&, To&)> func) {
        auto& converters = get_converters();
        converters[Helper<From>::ID()][Helper<To>::ID()] = 
            [conv_func = func](const void* from, void* to) {
                try {
                    return conv_func(*static_cast<const From*>(from), *static_cast<To*>(to));
                }
                catch(...) {
                    return false;
                }
                
            };
        return true;
    }

    static bool TryConvert(size_t from_type, size_t to_type, const void* from, void* to) {
        const auto& converters = get_converters();
        const auto it_from = converters.find(from_type);
        if (it_from != converters.end()) {
            auto it_to = it_from->second.find(to_type);
            if (it_to != it_from->second.end()) {
                return it_to->second(from, to);
            }
        }
        return false;
    }

    template<typename To, typename From>
    static bool TryConvert(const From& from, To& to) {
        return TryConvert(Helper<From>::ID(), Helper<To>::ID(), &from, &to);
    }

    static bool CanConvert(size_t from_type, size_t to_type) {
        const auto& converters = get_converters();
        const auto it_from = converters.find(from_type);
        return it_from != converters.end() && it_from->second.find(to_type) != it_from->second.end();
    }
};


template<typename To, typename From>
struct TypeConverterHelper
{
    static bool Can() { return false; }
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
        if constexpr (SerHelper<T>::val)
            return ser::Serialize(ret, &data);
        else
            throw(std::runtime_error("Non Serializeble"));
    }
    virtual size_t Deserialize(const ser::data_t& var) override
    {
        if constexpr (SerHelper<T>::val)
            return ser::Deserialize(var, &data);
        else
            throw(std::runtime_error("Non Serializeble"));
    }
};

template<>
struct RawValue<const char*> : public RawValue<std::string>
{
    inline static RawData* make(const char*& _data) { return new RawValue<std::string>(_data); }
    inline static RawData* make(const char**& _data) { return new RawValue<std::string>(*_data); }
};

template<>
struct RawValue<char*> : public RawValue<std::string>
{
    inline static RawData* make(const char*& _data) { return new RawValue<std::string>(_data); }
    inline static RawData* make(const char**& _data) { return new RawValue<std::string>(*_data); }
};

template<size_t S>
struct RawValue<const char[S]> : public RawValue<std::string>
{
    inline static RawData* make(const char(&_data)[S]) { return new RawValue<std::string>(_data); }
    inline static RawData* make(const char(*&_data)[S]) { return new RawValue<std::string>(*_data); }
};

template<size_t S>
struct RawValue<char[S]> : public RawValue<std::string>
{
    inline static RawData* make(const char(&_data)[S]) { return new RawValue<std::string>(_data); }
    inline static RawData* make(const char(*&_data)[S]) { return new RawValue<std::string>(*_data); }
};

struct __data_imp
{

private:
    struct __counter_t
    {
        std::atomic<size_t> data;
        __counter_t(const size_t& v = 1) : data(v)
        {
            printf("create   0x%llx\n", this);
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
            printf("~destroy 0x%llx\n", this);
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

    template<typename... T>
    Data(Pointer<T...>& _) : imp(_.imp)
    {}
    template<typename... T>
    Data(Pointer<T...>&& _) : imp(_.imp)
    {}
    template<typename... T>
    Data(const Pointer<T...>& _) : imp(_.imp)
    {}
    template<typename... T>
    Data(const Pointer<T...>&& _) : imp(_.imp)
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
    
    /*!
    * @brief Get a pointer to the stored value of type T
    * @tparam T The type to retrieve
    * @return Pointer to the stored value, or nullptr if type doesn't match or data is empty
    * @note This method performs exact type matching only
    */
    template<typename T>
    T* as()
    {
        if (imp.data == nullptr)
            return nullptr;
        return imp.data->as<T>();
    }

    /*!
    * @brief Get a const pointer to the stored value of type T
    * @tparam T The type to retrieve
    * @return Const pointer to the stored value, or nullptr if type doesn't match or data is empty
    * @note This method performs exact type matching only
    */
    template<typename T>
    const T* as() const
    {
        if (imp.data == nullptr)
            return nullptr;
        return imp.data->as<T>();
    }
    
    /*!
    * @brief Check if the stored data is of type T
    * @tparam T The type to check against
    * @return true if data is not empty and matches type T, false otherwise
    * @note For RawData types, checks against the underlying type
    */
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
    
    /*!
    * @brief Implicit conversion to non-const reference of type T
    * @tparam T The target type (must not be bool)
    * @return Non-const reference to the stored value
    * @throw std::runtime_error if data is empty or type doesn't match exactly
    * @note This operator requires exact type matching, no conversions are performed
    */
    template<typename T, typename std::enable_if<!std::is_same<T, bool>::value>::type* = nullptr>
    operator T&()
    {
        if (imp.data == nullptr)
            throw std::runtime_error(std::string("Trying to get a value from non-initialized data."));
        auto t = imp.data->as<T>();
        if (t)
            return *t;
        auto s = std::string("Trying to get an incorrect type from data. Current: ") + imp.data->name + ". Requested: " + Helper<T>::NAME() + ".";
        throw std::runtime_error(s);
    }

    /*!
    * @brief Implicit conversion to const reference of type T
    * @tparam T The target type
    * @return If T is bool: returns whether data is not empty
    *         Otherwise: const reference to the stored value
    * @throw std::runtime_error if data is empty or type doesn't match exactly
    * @note For bool: returns data presence. For other types: exact matching required
    */
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

    /*!
    * @brief Convert to Pointer<T> wrapper
    * @tparam T The target pointer type
    * @return Pointer<T> containing the data if type matches, empty Pointer<T> otherwise
    * @note Useful for reference-counted pointer semantics
    */
    template<typename T>
    operator Pointer<T>()
    {
        if (is<T>())
            return Pointer<T>(imp);
        else
            return Pointer<T>(nullptr)
    }

    /*!
    * @brief Arrow operator for pointer-like access
    * @tparam T Enabled only for pointer types
    * @return Reference to the underlying pointer operator
    * @note Provides direct access to the stored pointer's methods
    */
    template<typename T, typename = std::enable_if_t<std::is_pointer<T>::value>>
    auto& operator->()
    {
        return imp.data.operator->();
    }

    /*!
    * @brief Safe conversion to type T with fallback
    * @tparam T The target type
    * @param[out] v Reference to store the converted value
    * @return true if conversion was successful, false otherwise
    * @note For bool: sets v to data presence. For other types: attempts exact match, then type conversion
    */
    template<typename T>
    bool to(T& v) const
    {
        if constexpr (std::is_same_v<T, bool>)
            v = (imp.data != nullptr);
        else
        {
            if (imp.data == nullptr)
                return false;
            auto t = imp.data->as<T>();
            if (t)
                v = *t;
            
            std::remove_const_t<T> val;
            if(TypeConverter::TryConvert(imp.data->dataType, Helper<T>::ID(), imp.data->ptr(), &val))
            {
                v = val;
                return true;
            }
            return false;
            
        }
        return true;
    }

    /*!
    * @brief Explicit conversion to type T with type conversion support
    * @tparam T The target type
    * @return Converted value of type T
    * @throw std::runtime_error if data is empty or conversion fails
    * @note Attempts exact type match first, then falls back to registered type converters
    * @warning Unlike implicit operators, this method allows cross-type conversions
    */
    template<typename T>
    T cast() const
    {

        if constexpr (std::is_same_v<T, bool>)
            return (imp.data != nullptr);
        else
        {
            if (imp.data == nullptr)
                throw std::runtime_error(std::string("Trying to get a value from non-initialized data."));
            auto t = imp.data->as<T>();
            if (t)
                return *t;
            
            std::remove_const_t<T> val;
            if(TypeConverter::TryConvert(imp.data->dataType, Helper<T>::ID(), imp.data->ptr(), &val))
            {
                return val;
            }
            throw std::runtime_error(std::string("Trying to get an incorrect type from data. Current: ") + imp.data->name + ". Requested: " + Helper<T>::NAME() + ".");
            
        }
        throw std::runtime_error(std::string("Trying to get an incorrect type from data. Current: ") + imp.data->name + ". Requested: " + Helper<T>::NAME() + ".");
    }

    /*!
    * @brief Check if data contains a value
    * @return true if data is not empty, false otherwise
    * @note Equivalent to operator bool() but more explicit
    */
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

namespace ser{
template<class T>
static size_t Serialize(data_t& ret, const T* var) { return 0; }

template<class T>
static size_t Deserialize(const data_t& var, T* ret) { return 0; }

template<class T>
static size_t SerializeImp(data_t& ret, const T* var);

template<class T>
static size_t DeserializeImp(const data_t& var, T* ret);

template <typename... VALS>
static auto __make_tuple_helper__(VALS& ... vals) { return std::tuple(&vals...); } 
};


template<typename T>
constexpr size_t TGETID()
{
    return Helper<T>::ID() & 0xFFFFFFFFFFFF0000ULL;
}

template<typename T, typename T1, typename... Ts>
constexpr size_t TGETID()
{
    auto t = TGETID<T1, Ts...>() ^ ((0xC5965A3CAA556695 >> ((sizeof...(Ts) * 31) % 64)) | (0xC5965A3CAA556695 >> (63 - ((sizeof...(Ts) * 31) % 64))));
    return (Helper<T>::ID() ^ ((t << 11) | (t >> 53))) & 0xFFFFFFFFFFFF0000ULL;
}


#if 1 // DECLARE_DATA_TYPE defines

#define TGID(...) data_core::TGETID<__VA_ARGS__>()

#define __I__(...) __VA_ARGS__
#define __I(...) __I__(__VA_ARGS__)


template<typename Ret, typename... Vars> 
struct Helper<Ret(Vars...)>
{
    static constexpr size_t ID() { return data_core::TGETID<Ret, Vars...>(); };
    static constexpr const char* NAME() { return Helper<Ret>::NAME(); }
};

#define DECLARE_DATA_TYPE(T) template<> struct data_core::Helper<T> {\
    static constexpr size_t ID() { return GID(T); } \
    static constexpr const char* NAME() { return #T; } \
}; template<> struct data_core::Helper<const T> {\
    static constexpr size_t ID() { return GID(T); } \
    static constexpr const char* NAME() { return #T; }\
};

#define DECLARE_DATA_TEMPLATE_TYPE(T, tf, ts) template<tf> struct data_core::Helper<T<ts>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(ts); } \
    static constexpr const char* NAME() { return #T; } \
}; template<tf> struct data_core::Helper<const T<ts>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(ts); } \
    static constexpr const char* NAME() { return #T; } \
};

#define DECLARE_DATA_AUTO_TEMPLATE_TYPE(T) template<typename... Ts> struct data_core::Helper<T<Ts...>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(Ts...); } \
    static constexpr const char* NAME() { return #T; } \
}; template<typename... Ts> struct data_core::Helper<const T<Ts...>> {\
    static constexpr size_t ID() { return GID(T) ^ TGID(Ts...); } \
    static constexpr const char* NAME() { return #T; } \
};

// WARNING use this in global scope (without namespace) and AFTER DECLARE_DATA_SERIALIZER_FUNC and DECLARE_DATA_DESERIALIZER_FUNC
#define DECLARE_DATA_SERIALIZABLE(T) template<> struct data_core::SerHelper<T> { static constexpr bool val = true; }; \
template<> struct data_core::ser::imp_t<T> { static size_t Serialize(data_t& ret, const void* var) { return data_core::ser::SerializeImp<T>(ret, (const T*) var); } static size_t Deserialize(const data_t& var, void* ret){return data_core::ser::DeserializeImp<T>(var, (T*) ret);} };  
// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZABLE_TEMPLATE(T, tf, ts) template<tf> struct data_core::SerHelper<T<ts>> { static constexpr bool val = true; }; \
template<tf> struct data_core::ser::imp_t<T<ts>> { static size_t Serialize(data_t& ret, const void* var); static size_t Deserialize(const data_t& var, void* ret); }; 

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_FUNC(T)   template<> size_t data_core::ser::SerializeImp<T>  (data_t& serealized, const T* void_deserealized)
// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_DESERIALIZER_FUNC(T) template<> size_t data_core::ser::DeserializeImp<T>(const data_t& serealized, T* void_deserealized)

// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_SERIALIZER_TEMPLATE_FUNC(T, tf, ts)   DECLARE_DATA_SERIALIZABLE_TEMPLATE(T, __I(tf), __I(ts)) \
                                                           template<tf> size_t data_core::ser::imp_t<T<ts>>::Serialize  (data_t& serealized, const void* void_deserealized)
// WARNING use this in global scope (without namespace)
#define DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(T, tf, ts) template<tf> size_t data_core::ser::imp_t<T<ts>>::Deserialize(const data_t& serealized, void* void_deserealized)

#define DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(T) const T* deserealized = (const T*)void_deserealized;\
if(deserealized == 0) throw std::invalid_argument("empty pointer"); \
size_t offset = 0; \
Write(offset, serealized, data_core::Helper<T>::ID())

#define DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(T)  T* deserealized = (T*)void_deserealized;\
if(deserealized == 0) throw std::invalid_argument("empty pointer");\
size_t offset = 0;\
if(Read<size_t>(offset, serealized) != data_core::Helper<T>::ID()) throw std::invalid_argument("uncorrect uid")

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
#define DECLARE_DATA_SERIALIZER_DESERIALIZER_DEFAULT(T) DECLARE_DATA_SERIALIZER_DEFAULT(T); DECLARE_DATA_DESERIALIZER_DEFAULT(T); DECLARE_DATA_SERIALIZABLE(T);

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


#define REGISTER_CONVERSION_CONCAT_INNER(a, b) a ## b
#define REGISTER_CONVERSION_CONCAT(a, b) REGISTER_CONVERSION_CONCAT_INNER(a, b)

#define REGISTER_CONVERSION_DEFAULT(From, To) template<> struct data_core::TypeConverterHelper<From, To> {\
    static bool Can() { return true; } \
}; namespace data_core { namespace helpers { static const bool REGISTER_CONVERSION_CONCAT(REGISTER_CONVERSION_CONCAT(_REGISTER_CONVERSION_STATIC_OBJECT_, __COUNTER__), __LINE__) = data_core::TypeConverter::RegisterConverter<From, To>(); } }

#define REGISTER_CONVERSION_FUNCTION(From, To, ...) template<> struct data_core::TypeConverterHelper<From, To> {\
    static bool ConvFunction(const From& from, To& to)__VA_ARGS__; \
    static bool Can() { return true; } \
}; namespace data_core { namespace helpers { static const bool REGISTER_CONVERSION_CONCAT(REGISTER_CONVERSION_CONCAT(_REGISTER_CONVERSION_STATIC_OBJECT_, __COUNTER__), __LINE__) = data_core::TypeConverter::RegisterConverter<From, To>(&data_core::TypeConverterHelper<From, To>::ConvFunction); } }


#endif
} // namespace data_core

template<typename... ARGS>
using Ref = data_core::Pointer<ARGS...>;

template<typename To, typename From, typename... ARGS>
constexpr static To* RefCast(Ref<From, ARGS...>& v)
{
    return dynamic_cast<To*>((From*)(v));
}
using Data = data_core::Data;

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

#define __HELPER__(X) \
REGISTER_CONVERSION_DEFAULT(X, long long); \
REGISTER_CONVERSION_DEFAULT(X, long); \
REGISTER_CONVERSION_DEFAULT(X, int); \
REGISTER_CONVERSION_DEFAULT(X, short); \
REGISTER_CONVERSION_DEFAULT(X, char); \
REGISTER_CONVERSION_DEFAULT(X, unsigned long long); \
REGISTER_CONVERSION_DEFAULT(X, unsigned long); \
REGISTER_CONVERSION_DEFAULT(X, unsigned int); \
REGISTER_CONVERSION_DEFAULT(X, unsigned short); \
REGISTER_CONVERSION_DEFAULT(X, unsigned char); \
REGISTER_CONVERSION_DEFAULT(X, float); \
REGISTER_CONVERSION_DEFAULT(X, double);

__HELPER__(long long);
__HELPER__(long);
__HELPER__(int);
__HELPER__(short);
__HELPER__(char);
__HELPER__(unsigned long long);
__HELPER__(unsigned long);
__HELPER__(unsigned int);
__HELPER__(unsigned short);
__HELPER__(unsigned char);
__HELPER__(float);
__HELPER__(double);

#undef __HELPER__

DECLARE_DATA_TYPE(data_core::Data);

DECLARE_DATA_TEMPLATE_TYPE(std::shared_ptr, __I(typename T), __I(T));
DECLARE_DATA_TEMPLATE_TYPE(std::unique_ptr, __I(typename T), __I(T));
DECLARE_DATA_TEMPLATE_TYPE(std::weak_ptr, __I(typename T), __I(T));


DECLARE_DATA_SERIALIZER_FUNC(data_core::Data) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(data_core::Data);
    data_t temp ;
    deserealized->Serialize(temp);
    Write(offset, serealized, temp.data(), temp.size());
    return offset;
}
DECLARE_DATA_DESERIALIZER_FUNC(data_core::Data) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(data_core::Data);
    data_t temp = data_t(serealized.begin() + offset, serealized.end());
    offset += deserealized->Deserialize(temp);
    return offset;
}

DECLARE_DATA_SERIALIZABLE(data_core::Data);

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

DECLARE_DATA_SERIALIZABLE(std::string);

#define __HELPER__(X) REGISTER_CONVERSION_FUNCTION(X, std::string, { try { to = std::to_string(from); return true; } catch(const std::exception& e) { } return false; });

__HELPER__(long long);
__HELPER__(long);
__HELPER__(int);
__HELPER__(short);
__HELPER__(char);
__HELPER__(unsigned long long);
__HELPER__(unsigned long);
__HELPER__(unsigned int);
__HELPER__(unsigned short);
__HELPER__(unsigned char);
__HELPER__(float);
__HELPER__(double);

#undef __HELPER__

REGISTER_CONVERSION_FUNCTION(std::string, const char*, { try { to = from.c_str(); return true; } catch(const std::exception& e) { } return false; });

REGISTER_CONVERSION_FUNCTION(std::string, long long, { try { to = std::stoll(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, long, { try { to = std::stol(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, int, { try { to = std::stoi(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, short, { try { to = std::stoll(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, char, { try { to = (char)std::stoll(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, unsigned long long, { try { to = std::stoull(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, unsigned long, { try { to = std::stoul(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, unsigned int, { try { to = std::stoul(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, unsigned short, { try { to = std::stoull(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, unsigned char, { try { to = (char)std::stoull(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, float, { try { to = std::stof(from.c_str()); return true; } catch(const std::exception& e) { } return false; });
REGISTER_CONVERSION_FUNCTION(std::string, double, { try { to = std::stod(from.c_str()); return true; } catch(const std::exception& e) { } return false; });


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
    for(auto& el : deserealized)
        SerWrite(offset, serealized, &el);
    return offset;
}
DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(std::list, __I(typename T), __I(T)) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(std::list<T>);
    deserealized->resize(Read<int>(offset, serealized));
    for(auto& el : deserealized)
        DserRead(offset, serealized, &el);
    return offset;
}

// std::unordered_map

DECLARE_DATA_TEMPLATE_TYPE(std::unordered_map, __I(typename KEY, typename VALUE), __I(KEY, VALUE))
DECLARE_DATA_SERIALIZER_TEMPLATE_FUNC(std::unordered_map, __I(typename KEY, typename VALUE), __I(KEY, VALUE)) {
    DECLARE_DATA_SERIALIZER_DEFAULT_HEADER(std::unordered_map<__I(KEY, VALUE)>);
    Write(offset, serealized, (int)deserealized->size());
    for(auto& el : deserealized)
    {
        SerWrite(offset, serealized, &el.first);
        SerWrite(offset, serealized, &el.second);
    }
    return offset;
}
DECLARE_DATA_DESERIALIZER_TEMPLATE_FUNC(std::unordered_map, __I(typename KEY, typename VALUE), __I(KEY, VALUE)) {
    DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(std::unordered_map<__I(KEY, VALUE)>);
    deserealized->resize(Read<int>(offset, serealized));
    for(auto& el : deserealized)
    {
        DserRead(offset, serealized, &el.first);
        DserRead(offset, serealized, &el.second);
    }
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
    Write(offset, serealized, data_core::Helper<__HELPER__::tuple_type>::ID()); \
    Write(offset, serealized, (int)(std::tuple_size_v<__HELPER__::tuple_type>)); \
    auto tpl = ((const __HELPER__*)deserealized)->make_tuple();  \
    std::apply([&] (auto&&... args) { (SerWrite(offset, serealized, &args), ...); }, tpl); \
    return offset; \
} DECLARE_DATA_DESERIALIZER_FUNC(name) { DECLARE_DATA_DESERIALIZER_DEFAULT_HEADER(name); \
    struct __HELPER__ : name { using tuple_type = decltype(std::tuple(__VA_ARGS__)); auto make_tuple() { return ser::__make_tuple_helper__(__VA_ARGS__); } }; \
    if(Read<size_t>(offset, serealized) != data_core::Helper<__HELPER__::tuple_type>::ID()) throw std::invalid_argument("uncorrect tuple uid"); \
    if(Read<int>(offset, serealized) != std::tuple_size_v<__HELPER__::tuple_type>) throw std::invalid_argument("uncorrect tuple size"); \
    auto tpl = ((__HELPER__*)deserealized)->make_tuple(); \
    std::apply([&] (auto&&... args) { (DserRead(offset, serealized, args), ...); }, tpl); \
    return offset; \
}




