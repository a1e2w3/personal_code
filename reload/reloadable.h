/**
 * @file reloadable.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-24 11:17:28
 * @brief 一个通用的基于双buffer的reload实现
 *        读取资源无锁，只有reload加锁
 **/
 
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

namespace common {

#define RET_SUCC 0
#define RET_LOAD_FAIL -1
#define RET_BUFFER_INUSE -2

typedef uint8_t version_t;

// default resource loader
// just new a Resource Object
template<typename Resource>
struct DefaultResourceLoader {
    Resource* operator ()(void *) const throw() {
        return new (std::nothrow) Resource();
    }
};

template<typename Resource>
class ResourceHandle;

// thread-safely
template<typename Resource, typename Deleter = std::default_delete<Resource> >
class Reloadable {
private:
    friend class ResourceHandle<Resource>;
    typedef uint32_t ref_count_t;
    // pointer type for internal
    typedef std::unique_ptr<Resource, Deleter> resource_ptr_type;

public:
    typedef Resource ResourceType;
    typedef std::function<Resource* (void*)> ResourceLoader;
    // pointer type for outside, with ref count
    typedef ResourceHandle<Resource> ResourcePointer;

    Reloadable() : _loader(DefaultResourceLoader<Resource>()) {}
    ~Reloadable() {}

    void set_resource_loader(const ResourceLoader& loader) {
        _loader = loader;
    }

    int init(void *params) {
        return load_reource(params, false);
    }

    int reload(void *params) {
        return load_reource(params, true);
    }

    int release_unused() {
        version_t version_to_release = _version.load() + 1;
        version_t index = version_to_release & 0x01;
        if (_refs[index].load() == 0 && !_resource_container[index]) {
            std::lock_guard<std::mutex> lock(_reload_mutex);
            // try release under lock
            return try_release_version(version_to_release);
        } else {
            // version in use or has been released already
            return RET_BUFFER_INUSE;
        }
    }

    ResourcePointer get_resource() {
    	ResourcePointer handler(this, _version.load());
        // 此处会出现两种情况
        // 1. _version被修改, 后面会检测到cur_ver != version
        // 2. _version未修改, cur_ver已被引用计数, 不用担心被释放

        // check version changed
        while (handler.version() != _version.load()) {
            // version被修改, 因cur_ver已增加引用计数, 无法被连续reload
            // 只用更新最新的version, ref后, 再释放原有的引用计数
            handler.rebind_version(_version.load());
        }
        return handler;
    }

private:
    int load_reource(void *params, bool is_reload) {
        std::lock_guard<std::mutex> lock(_reload_mutex);
        auto cur_version = _version.load();
        int32_t index_to_load = (is_reload ? cur_version + 1 : cur_version) & 0x01;
        if (_refs[index_to_load].load() != 0) {
            return RET_BUFFER_INUSE;
        }

        Resource *resource = _loader(params);
        if (nullptr == resource) {
            return RET_LOAD_FAIL;
        }

        _resource_container[index_to_load].reset(resource);
        _version.store(index_to_load);
        if (is_reload) {
            try_release_version((index_to_load + 1) & 0x01);
        }
        return RET_SUCC;
    }

    // should be under lock
    int try_release_version(int32_t version_to_release) {
        if (version_to_release == _version.load()) {
            return RET_BUFFER_INUSE;
        }
        int32_t index = version_to_release & 0x01;
        if (_refs[index].load() != 0 || _resource_container[index] == nullptr) {
            return RET_BUFFER_INUSE;
        }
        _resource_container[index].reset();
        return RET_SUCC;
    }

    ref_count_t ref_version(version_t version) {
        return ++_refs[version & 0x01];
    }
    ref_count_t unref_version(version_t version) {
        ref_count_t ref_count = --_refs[version & 0x01];
        release_unused();
        return ref_count;
    }

    Resource* get_ver_resource(version_t version) {
        return _resource_container[version & 0x01].get();
    }

    const Resource* get_ver_resource(version_t version) const {
        return _resource_container[version & 0x01].get();
    }

private:
    std::mutex _reload_mutex;
    ResourceLoader _loader;
    std::atomic<version_t> _version;
    std::atomic<ref_count_t> _refs[2];
    resource_ptr_type _resource_container[2]; // duel buffer
};

// thread-unsafe
// should not access one ResourceHandle in different threads
// ensure ResourceHandle will not been released while access pointer
template<typename Resource>
class ResourceHandle {
private:
    friend class Reloadable<Resource>;

    // acccess by Reloadable only
    ResourceHandle(Reloadable<Resource>* reloadable, version_t version)
       : _reloadable(reloadable),
         _version(version) {
        if (_reloadable != nullptr) {
            _reloadable->ref_version(_version);
        }
    }

public:
    typedef Resource ResourceType;

    ResourceHandle() : _reloadable(nullptr), _version(0) {}

    ResourceHandle(const ResourceHandle& other)
        : _reloadable(other._reloadable),
          _version(other._version) {
        if (_reloadable != nullptr) {
            _reloadable->ref_version(_version);
        }
    }

    ResourceHandle(ResourceHandle&& other)
        : _reloadable(other._reloadable),
          _version(other._version) {
        if (_reloadable != nullptr) {
            _reloadable->ref_version(_version);
        }
        other._reloadable = nullptr;
        other._version = 0;
    }

    ~ResourceHandle() {
        release();
    }

    ResourceHandle& operator =(const ResourceHandle& other) {
        if (this != &other) {
            // prevent copy from this
            release();
            _reloadable = other._reloadable;
            _version = other._version;
            if (_reloadable != nullptr) {
                _reloadable->ref_version(_version);
            }
        }
        return *this;
    }

    ResourceHandle& operator =(ResourceHandle&& other) {
        if (this != &other) {
            // prevent copy from this
            release();
            _reloadable = other._reloadable;
            _version = other._version;
            if (_reloadable != nullptr) {
                _reloadable->ref_version(_version);
            }
            other.release();
        }
        return *this;
    }

    void release() {
        if (_reloadable != nullptr) {
            _reloadable->unref_version(_version);
            _reloadable = nullptr;
        }
    }

    Resource* get() {
        return _reloadable == nullptr ? nullptr : _reloadable->get_ver_resource(_version);
    }

    const Resource* get() const {
        return _reloadable == nullptr ? nullptr : _reloadable->get_ver_resource(_version);
    }

    Resource* operator ->() {
        return get();
    }

    const Resource* operator ->() const {
        return get();
    }

    Resource& operator *() {
        return *get();
    }

    const Resource& operator *() const {
        return *get();
    }

    bool is_null() const {
        return get() == nullptr;
    }

    explicit operator bool() const noexcept {
        return is_null();
    }

private:
    version_t version() const { return _version; }

    void rebind_version(version_t new_version) {
        if (new_version != _version) {
            _reloadable->ref_version(new_version);
            _reloadable->unref_version(_version);
            _version = new_version;
        }
    }

private:
    Reloadable<Resource>* _reloadable;
    version_t _version;
};
 
} // end namespace common
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
