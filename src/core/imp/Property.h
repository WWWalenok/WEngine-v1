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


template<typename T>
class ReactiveProperty {
private:
    std::function<T()> getter_;
    std::function<void(const T&)> setter_;
    std::vector<ReactiveProperty<T>*> subscribers_;
    ReactiveProperty<T>* owner_ = nullptr;

public:
    // Запрет копирования
    ReactiveProperty(const ReactiveProperty<T>&) = delete;
    ReactiveProperty<T>& operator=(const ReactiveProperty<T>&) = delete;
    
    // Конструкторы
    ReactiveProperty(std::function<T()> getter, std::function<void(const T&)> setter) 
        : getter_(std::move(getter)), setter_(std::move(setter)) {}
        
    explicit ReactiveProperty(std::function<T()> getter) 
        : getter_(std::move(getter)), setter_(nullptr) {}
    
    explicit ReactiveProperty(T initial_value) 
        : getter_([value = initial_value]() { return value; }), 
          setter_([value = initial_value](const T& new_val) mutable { value = new_val; }) {}

    // Основные операторы
    operator T() const { 
        return getter_(); 
    }
    
    ReactiveProperty<T>& operator=(const T& value) {
        if (setter_) {
            setter_(value);
            notifySubscribers();
        }
        return *this;
    }
    
    void subscribe(ReactiveProperty<T>* subscriber) {
        if (subscriber == this) throw std::logic_error("try create selfsubscribe");
        if (subscriber->owner_ != nullptr)  throw std::logic_error("try subscribe already subscribed property");
        if (std::find(subscribers_.begin(), subscribers_.end(), subscriber) == subscribers_.end()) {
            subscriber->owner_ = this;
            subscribers_.push_back(subscriber);
            
        }
    }

    void subscribe(ReactiveProperty<T>& subscriber) {
        subscribe(&subscriber);
    }
    
    void unsubscribe(ReactiveProperty<T>& subscriber) {
        if (subscriber->owner_ != this) return;
        if (auto it = std::find(subscribers_.begin(), subscribers_.end(), &subscriber) != subscribers_.end()) {
            subscriber->owner_ = nullptr;
            subscribers_.erase(it);
        }
    }
    
    void unsubscribeThis() {
        if(owner_)
            owner_->unsubscribe(this);
    }
    
    void unsubscribeAll() {
        for (auto* subscriber : subscribers_) {
            if (subscriber) {
                subscriber->ownewr_ = nullptr;
            }
        }
        subscribers_.clear();
    }
    
    void forceNotify() {
        notifySubscribers();
    }
    
    size_t subscriberCount() const {
        return subscribers_.size();
    }

private:
    void onSourceUpdated(const T& source_value) {
        if (setter_) {
            setter_(source_value);
            notifySubscribers();
        }
    }
    
    void notifySubscribers() {
        if (subscribers_.empty()) return;

        T current_value = getter_();
        
        for (auto* subscriber : subscribers_) {
            if (subscriber) {
                subscriber->onSourceUpdated(current_value);
            }
        }
    }
};

template<typename T>
class WeakReactiveProperty {
private:
    ReactiveProperty<T>* _prop;

public:
    WeakReactiveProperty() : _prop(nullptr) {}
    WeakReactiveProperty(ReactiveProperty<T>& prop) : _prop(&prop) {}
    WeakReactiveProperty(ReactiveProperty<T>* prop) : _prop(prop) {}
    WeakReactiveProperty(const WeakReactiveProperty<T>& other) : _prop(other._prop) {}
    
    WeakReactiveProperty<T>& operator=(ReactiveProperty<T>& prop) { 
        _prop = &prop; 
        return *this; 
    }
    WeakReactiveProperty<T>& operator=(ReactiveProperty<T>* prop) { 
        _prop = prop; 
        return *this; 
    }
    WeakReactiveProperty<T>& operator=(const WeakReactiveProperty<T>& other) { 
        _prop = other._prop; 
        return *this; 
    }
    
    bool isBound() const { return _prop != nullptr; }
    
    operator T() const { 
        if (!_prop) throw std::runtime_error("Accessing unbound WeakReactiveProperty");
        return *_prop; 
    }
    
    WeakReactiveProperty<T>& operator=(const T& value) { 
        if (!_prop) throw std::runtime_error("Setting unbound WeakReactiveProperty");
        *_prop = value; 
        return *this; 
    }
};




