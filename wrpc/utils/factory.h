/**
 * @file register.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-06 20:54:11
 * @brief 一个通用的工厂类
 *  
 **/
 
#ifndef WRPC_UTILS_FACTORY_H_
#define WRPC_UTILS_FACTORY_H_
 
#include <functional>
#include <map>
#include <memory>
#include <utility>
 
namespace wrpc {

template<class Strategy, class... Args>
using StrategyCreator = std::function<Strategy* (Args&&...)>;

template<class Strategy>
using StrategyDeleter = std::function<void (Strategy*)>;

template<class Strategy, class... Args>
class StrategyFactory {
private:
    StrategyFactory() {}
    ~StrategyFactory() {}

    using CreatorType = StrategyCreator<Strategy, Args...>;
    using DeleterType = StrategyDeleter<Strategy>;
    using FactoryPair = std::pair<CreatorType, DeleterType>;
    using StrategyCreatorMap = std::map<std::string, FactoryPair>;
    using DefaultDeleter = std::default_delete<Strategy>;

public:
    using StrategyUniquePtr = std::unique_ptr<Strategy, DeleterType>;
    using StrategySharedPtr = std::shared_ptr<Strategy>;

public:
    static StrategyFactory& get_instance() {
        static StrategyFactory s_instance;
        return s_instance;
    }

    bool register_creator(const std::string& name,
                const StrategyCreator<Strategy, Args...>& creator) {
        return register_creator(name, creator, DefaultDeleter());
    }

    bool register_creator(const std::string& name,
            const StrategyCreator<Strategy, Args...>& creator,
            const StrategyDeleter<Strategy>& deleter) {
        auto insert_ret = _creator_map.emplace(name, std::make_pair(creator, deleter));
        return insert_ret.second;
    }

    bool is_registered(const std::string& name) {
        return _creator_map.find(name) != _creator_map.end();
    }

    // new_instance获取的实例，必须用destroy_instance才能安全释放
    // 如需智能指针，使用make_unique或make_shared，而不要直接使用new_instance返回的指针给智能指针赋值
    Strategy* new_instance(const std::string& name, Args&&... args) {
        auto iter = _creator_map.find(name);
        if (iter == _creator_map.end()) {
            return nullptr;
        } else {
            return iter->second.first(std::forward<Args>(args)...);
        }
    }

    StrategyUniquePtr make_unique(const std::string& name, Args&&... args) {
        auto iter = _creator_map.find(name);
        if (iter == _creator_map.end()) {
            return StrategyUniquePtr();
        } else {
            return StrategyUniquePtr(
                    iter->second.first(std::forward<Args>(args)...),
                    iter->second.second);
        }
    }

    StrategySharedPtr make_shared(const std::string& name, Args&&... args) {
        auto iter = _creator_map.find(name);
        if (iter == _creator_map.end()) {
            return StrategySharedPtr();
        } else {
            return StrategySharedPtr(
                    iter->second.first(std::forward<Args>(args)...),
                    iter->second.second);
        }
    }

    void destroy_instance(const std::string& name, Strategy* instance) {
        if (nullptr == instance) {
            return;
        }

        auto iter = _creator_map.find(name);
        if (iter == _creator_map.end()) {
            delete instance;
        } else {
            iter->second.second(instance);
        }
    }

private:
    StrategyCreatorMap _creator_map;
};

} // end namespace wrpc

#define REGISTER_STRATEGY(base, clazz, name, id) \
    static base * create_##name##_##id##_strategy() { \
        return new clazz(); \
    } \
    void __attribute__ ((constructor)) resister_##name##_2_##id##_factory() { \
        ::wrpc::StrategyFactory< base >::get_instance().register_creator( \
                #name, create_##name##_##id##_strategy); \
    } \
    //static bool s_##name##_##id##_registered = \
    //    ::wrpc::StrategyFactory< base >::get_instance().register_creator( \
    //    #name, create_##name##_##id##_strategy); \

#define REGISTER_STRATEGY_NAMELY(base, clazz, name, id) \
    static base * create_##name##_##id##_strategy_namely(const std::string &st_name) { \
        return new clazz(st_name); \
    } \
    void __attribute__ ((constructor)) resister_##name##_2_##id##_factory_namely() { \
        ::wrpc::StrategyFactory< base, const std::string& >::get_instance().register_creator( \
                #name, create_##name##_##id##_strategy_namely); \
    } \
    //static bool s_##name##_##id##_registered = \
    //    ::wrpc::StrategyFactory< base, const std::string& >::get_instance().register_creator( \
    //    #name, create_##name##_##id##_strategy); \
 
#endif // WRPC_UTILS_FACTORY_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

