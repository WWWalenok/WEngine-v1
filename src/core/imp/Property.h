#include <functional>
#include <utility>
#include "data.h"


template<typename T>
class ReadOnlyProperty {
private:
    std::function<T()> getter_;

public:
    ReadOnlyProperty(const ReadOnlyProperty<T>&) = delete;
    ReadOnlyProperty<T>& operator=(const ReadOnlyProperty<T>&) = delete;
    explicit ReadOnlyProperty(std::function<T()> getter) : getter_(getter) {}
    
    virtual operator T() const { return getter_(); }
};

template<typename T>
class WriteOnlyProperty {
private:
    std::function<void(const T&)> setter_;
public:
    WriteOnlyProperty(const WriteOnlyProperty<T>&) = delete;
    WriteOnlyProperty<T>& operator=(const WriteOnlyProperty<T>&) = delete;
    explicit WriteOnlyProperty(std::function<void(const T&)> setter) : setter_(setter) {}
    
    virtual WriteOnlyProperty<T>& operator=(const T& value) { if(setter_) setter_(value); return *this; }
};

template<typename T>
class Property{
    std::function<T()> getter_;
    std::function<void(const T&)> setter_;
public:
    Property(const Property<T>&) = delete;
    Property<T>& operator=(const Property<T>&) = delete;
    explicit Property(std::function<T()> getter, std::function<void(const T&)> setter) : getter_(getter), setter_(setter) {}

    virtual operator T() const { return getter_(); }
    virtual Property<T>& operator=(const T& value) { if(setter_) setter_(value); return *this; }

};

template<typename T>
class WeakReadOnlyProperty
{
private:
    ReadOnlyProperty<T>* _prop;

public:
    WeakReadOnlyProperty() : _prop(nullptr) {}
    WeakReadOnlyProperty(ReadOnlyProperty<T>& prop) : _prop(&prop) {}
    WeakReadOnlyProperty(ReadOnlyProperty<T>* prop) : _prop(prop) {}
    WeakReadOnlyProperty(const WeakReadOnlyProperty<T>& prop) : _prop(prop._prop) {}
    WeakReadOnlyProperty<T>& operator=(ReadOnlyProperty<T>& prop) {_prop = &prop; return *this; }
    WeakReadOnlyProperty<T>& operator=(ReadOnlyProperty<T>* prop) {_prop = prop; return *this; }
    WeakReadOnlyProperty<T>& operator=(const WeakReadOnlyProperty<T>& prop) {_prop = prop._prop; return *this; }
    
    operator T() const { if(!_prop) throw std::runtime_error("try get non binded WeakReadOnlyProperty"); return (*_prop); }
};

template<typename T>
class WeakWriteOnlyProperty
{
private:
    WriteOnlyProperty<T>* _prop;

public:
    WeakWriteOnlyProperty() : _prop(nullptr) {}
    WeakWriteOnlyProperty(WriteOnlyProperty<T>& prop) : _prop(&prop) {}
    WeakWriteOnlyProperty(WriteOnlyProperty<T> prop) : _prop(prop) {}
    WeakWriteOnlyProperty(const WeakWriteOnlyProperty<T>& prop) : _prop(prop._prop) {}
    WeakWriteOnlyProperty<T>& operator=(WriteOnlyProperty<T>& prop) {_prop = &prop; return *this; }
    WeakWriteOnlyProperty<T>& operator=(WriteOnlyProperty<T>* prop) {_prop = prop; return *this; }
    WeakWriteOnlyProperty<T>& operator=(const WeakWriteOnlyProperty<T>& prop) {_prop = prop._prop; return *this; }
    
    WeakWriteOnlyProperty<T>& operator=(const T& value) { if(!_prop) throw std::runtime_error("try set non binded WeakWriteOnlyProperty");  (*_prop) = value; return *this; }
};

template<typename T>
class WeakProperty
{
private:
    Property<T>* _prop;

public:
    WeakProperty() : _prop(nullptr) {}
    WeakProperty(Property<T>& prop) : _prop(&prop) {}
    WeakProperty(Property<T>* prop) : _prop(prop) {}
    WeakProperty(const WeakProperty<T>& prop) : _prop(prop._prop) {}
    WeakProperty<T>& operator=(Property<T>& prop) {_prop = &prop; return *this; }
    WeakProperty<T>& operator=(Property<T>* prop) {_prop = prop; return *this; }
    WeakProperty<T>& operator=(const WeakProperty<T>& prop) {_prop = prop._prop; return *this; }
    
    operator T() const { if(!_prop) throw std::runtime_error("try get non binded WeakProperty"); return (*_prop); }
    WeakProperty<T>& operator=(const T& value) { if(!_prop) throw std::runtime_error("try set non binded WeakProperty"); (*_prop) = value; return *this; }
};