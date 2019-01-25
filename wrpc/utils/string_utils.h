/**
 * @file string_utils.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 13:13:20
 * @brief 
 *  
 **/
 
#ifndef WRPC_UTILS_STRING_UTILS_H_
#define WRPC_UTILS_STRING_UTILS_H_
 
#include <string>
#include <vector>

namespace wrpc {

template<class T, class Enable = void>
struct to_string_impl {
    inline std::string operator() (T&& val) {
        std::ostringstream oss;
        oss << std::forward<T>(val);
        return oss.str();
    }
};

// 对nullptr_t特化
template<>
struct to_string_impl<std::nullptr_t> {
    inline std::string operator() (std::nullptr_t) {
        return std::string();
    }
};

// 对std::string类型偏特化, 直接构造
template<class T>
struct to_string_impl<T, typename std::enable_if<
        std::is_convertible<T&&, std::string>::value
        && !std::is_pointer<typename std::decay<T>::type>::value, void>::type> {
    inline std::string operator() (T&& val) {
        return std::string(std::forward<T>(val));
    }
};

// 对char*类型偏特化, 判断null并直接构造
template<class T>
struct to_string_impl<T, typename std::enable_if<
        std::is_convertible<T&&, std::string>::value
        && std::is_pointer<typename std::decay<T>::type>::value, void>::type> {
    inline std::string operator() (T&& val) {
        return val == nullptr ? std::string() : std::string(std::forward<T>(val));
    }
};

// 对数字类型偏特化, 调std::to_string
template<class T>
struct to_string_impl<T, typename std::enable_if<std::is_arithmetic<typename std::decay<T>::type>::value, void>::type> {
    inline std::string operator() (T&& val) {
        return std::to_string(val);
    }
};

/**
 * @brief 对象转string
 *
 * @param [in] val : T&&
 *        待转换的对象
 *
 * @return std::string
 * @retval 转换后的字符串
 *
 */
template<class T>
inline std::string to_string(T&& val) {
    return to_string_impl<T>()(std::forward<T>(val));
}
 
/**
 * @brief trim字符串前后的特殊字符
 *        只trim字符串开头和结尾，不删除中间的特殊字符
 *
 * @param [in] str : const std::string&
 *        待trim的源字符串
 * @param [in] chars_to_trim : const std::string &
 *        trim的特殊字符序列, 包含序列中的任一字符都会被trim掉
 *
 * @return std::string
 * @retval trim后的字符串
 *
 */
std::string trim_chars(const std::string& str, const std::string& chars_to_trim);

/**
 * @brief trim字符串前后的空白字符("\t \r\n")
 *        直接 return trim_chars(str, "\t \r\n")
 *
 * @param [in] str : const std::string&
 *        待trim的源字符串
 *
 * @return std::string
 * @retval trim后的字符串
 *
 */
std::string trim(const std::string& str);

/**
 * @brief 将\t分隔的一行字符串切割成key-value对, 第一列为key, 后面为value
 *
 * @param [in] line : const std::string &
 *        要切割的字符串
 * @param [in] delimiter : char
 *        分隔字符
 * @param [out] key : std::string *
 *        存放\t分隔的第一列，作为key
 * @param [out] value : std::string *
 *        存放第一列之后的子串，作为value
 *
 * @return int
 * @retval -1 on '\t' was found and (NULL!=key && NULL!=value)
 *          0 on success
 */
int split_2_kv(const std::string& line, char delimiter,
        std::string* key, std::string* value);

/**
 * @brief 将\t分隔的一行字符串切割成key-value对, 第一列为key, 后面为value
 *
 * @param [in] line : const std::string &
 *        要切割的字符串
 * @param [in] delimiter : const std::string &
 *        分隔字符串
 * @param [out] key : std::string *
 *        存放\t分隔的第一列，作为key
 * @param [out] value : std::string *
 *        存放第一列之后的子串，作为value
 *
 * @return int
 * @retval -1 on '\t' was found and (NULL!=key && NULL!=value)
 *          0 on success
 */
int split_2_kv(const std::string& line, const std::string& delimiter,
        std::string* key, std::string* value);

/**
 * @brief 字符串切分
 *
 * @param [in] src_str : const std::string &
 *        要切分的字符串
 * @param [in] delimiter : char
 *        分隔字符
 * @param [out] out_vec : std::vector<std::string> *
 *        存放切分后的子串
 *
 * @return size_t
 * @retval 切分后的子串个数
 */
size_t string_split(const std::string& src_str, char delimiter, std::vector<std::string>* out_vec);

/**
 * @brief 字符串切分
 *
 * @param [in] src_str : const std::string &
 *        要切分的字符串
 * @param [in] delimiter : const std::string &
 *        分隔字符串
 * @param [out] out_vec : std::vector<std::string> *
 *        存放切分后的子串
 *
 * @return size_t
 * @retval 切分后的子串个数
 */
size_t string_split(const std::string& src_str, const std::string& delimiter,
        std::vector<std::string>* out_vec);

/**
 * @brief 字符串连接
 *
 * @param [in] splits : const std::vector<std::string> &
 *        要连接的字符串列表
 * @param [in] delimiter : const std::string &
 *        分隔字符串
 *
 * @return std::string
 * @retval 连接后的字符串
 */
std::string string_join(const std::vector<std::string>& splits, const std::string& delimiter);

/**
 * @brief 实现字符串替换
 *        将src_str中的所有target子串替换成replace_to
 *
 * @param src_str : const std::string &
 *        源字符串
 * @param target : const std::string &
 *        替换目标
 * @param replace_to : const std::string &
 *        替换目标要被替换成的字符串
 *
 * @return std::string
 * @retval 替换后的字符串
 *
 */
std::string string_replace(const std::string& src_str,
        const std::string& target, const std::string& replace_to);

/**
 * @brief 字符串出现次数统计
 *        统计src中出现的target的次数
 *
 * @param src : const std::string &
 *        源字符串
 * @param target : const std::string &
 *        被查找字符串
 *
 * @return int
 * @retval 出现的次数，未出现为0
 *
 */
int string_count(const std::string& src, const std::string& target);

/**
 * @brief 判断字符串src_str是不是以prefix前缀开始
 *
 * @param [in] src_str : const std::string &
 *        要匹配的字符串
 * @param [in] prefix : const std::string &
 *        要匹配的前缀
 *
 * @return bool
 * @retval true if 'src_str' is start with 'prefix'
 */
bool string_start_with(const std::string& src_str, const std::string& prefix);
 
} // end namespace wrpc
 
#endif // WRPC_UTILS_STRING_UTILS_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

