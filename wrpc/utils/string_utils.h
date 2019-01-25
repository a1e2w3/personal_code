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

// ��nullptr_t�ػ�
template<>
struct to_string_impl<std::nullptr_t> {
    inline std::string operator() (std::nullptr_t) {
        return std::string();
    }
};

// ��std::string����ƫ�ػ�, ֱ�ӹ���
template<class T>
struct to_string_impl<T, typename std::enable_if<
        std::is_convertible<T&&, std::string>::value
        && !std::is_pointer<typename std::decay<T>::type>::value, void>::type> {
    inline std::string operator() (T&& val) {
        return std::string(std::forward<T>(val));
    }
};

// ��char*����ƫ�ػ�, �ж�null��ֱ�ӹ���
template<class T>
struct to_string_impl<T, typename std::enable_if<
        std::is_convertible<T&&, std::string>::value
        && std::is_pointer<typename std::decay<T>::type>::value, void>::type> {
    inline std::string operator() (T&& val) {
        return val == nullptr ? std::string() : std::string(std::forward<T>(val));
    }
};

// ����������ƫ�ػ�, ��std::to_string
template<class T>
struct to_string_impl<T, typename std::enable_if<std::is_arithmetic<typename std::decay<T>::type>::value, void>::type> {
    inline std::string operator() (T&& val) {
        return std::to_string(val);
    }
};

/**
 * @brief ����תstring
 *
 * @param [in] val : T&&
 *        ��ת���Ķ���
 *
 * @return std::string
 * @retval ת������ַ���
 *
 */
template<class T>
inline std::string to_string(T&& val) {
    return to_string_impl<T>()(std::forward<T>(val));
}
 
/**
 * @brief trim�ַ���ǰ��������ַ�
 *        ֻtrim�ַ�����ͷ�ͽ�β����ɾ���м�������ַ�
 *
 * @param [in] str : const std::string&
 *        ��trim��Դ�ַ���
 * @param [in] chars_to_trim : const std::string &
 *        trim�������ַ�����, ���������е���һ�ַ����ᱻtrim��
 *
 * @return std::string
 * @retval trim����ַ���
 *
 */
std::string trim_chars(const std::string& str, const std::string& chars_to_trim);

/**
 * @brief trim�ַ���ǰ��Ŀհ��ַ�("\t \r\n")
 *        ֱ�� return trim_chars(str, "\t \r\n")
 *
 * @param [in] str : const std::string&
 *        ��trim��Դ�ַ���
 *
 * @return std::string
 * @retval trim����ַ���
 *
 */
std::string trim(const std::string& str);

/**
 * @brief ��\t�ָ���һ���ַ����и��key-value��, ��һ��Ϊkey, ����Ϊvalue
 *
 * @param [in] line : const std::string &
 *        Ҫ�и���ַ���
 * @param [in] delimiter : char
 *        �ָ��ַ�
 * @param [out] key : std::string *
 *        ���\t�ָ��ĵ�һ�У���Ϊkey
 * @param [out] value : std::string *
 *        ��ŵ�һ��֮����Ӵ�����Ϊvalue
 *
 * @return int
 * @retval -1 on '\t' was found and (NULL!=key && NULL!=value)
 *          0 on success
 */
int split_2_kv(const std::string& line, char delimiter,
        std::string* key, std::string* value);

/**
 * @brief ��\t�ָ���һ���ַ����и��key-value��, ��һ��Ϊkey, ����Ϊvalue
 *
 * @param [in] line : const std::string &
 *        Ҫ�и���ַ���
 * @param [in] delimiter : const std::string &
 *        �ָ��ַ���
 * @param [out] key : std::string *
 *        ���\t�ָ��ĵ�һ�У���Ϊkey
 * @param [out] value : std::string *
 *        ��ŵ�һ��֮����Ӵ�����Ϊvalue
 *
 * @return int
 * @retval -1 on '\t' was found and (NULL!=key && NULL!=value)
 *          0 on success
 */
int split_2_kv(const std::string& line, const std::string& delimiter,
        std::string* key, std::string* value);

/**
 * @brief �ַ����з�
 *
 * @param [in] src_str : const std::string &
 *        Ҫ�зֵ��ַ���
 * @param [in] delimiter : char
 *        �ָ��ַ�
 * @param [out] out_vec : std::vector<std::string> *
 *        ����зֺ���Ӵ�
 *
 * @return size_t
 * @retval �зֺ���Ӵ�����
 */
size_t string_split(const std::string& src_str, char delimiter, std::vector<std::string>* out_vec);

/**
 * @brief �ַ����з�
 *
 * @param [in] src_str : const std::string &
 *        Ҫ�зֵ��ַ���
 * @param [in] delimiter : const std::string &
 *        �ָ��ַ���
 * @param [out] out_vec : std::vector<std::string> *
 *        ����зֺ���Ӵ�
 *
 * @return size_t
 * @retval �зֺ���Ӵ�����
 */
size_t string_split(const std::string& src_str, const std::string& delimiter,
        std::vector<std::string>* out_vec);

/**
 * @brief �ַ�������
 *
 * @param [in] splits : const std::vector<std::string> &
 *        Ҫ���ӵ��ַ����б�
 * @param [in] delimiter : const std::string &
 *        �ָ��ַ���
 *
 * @return std::string
 * @retval ���Ӻ���ַ���
 */
std::string string_join(const std::vector<std::string>& splits, const std::string& delimiter);

/**
 * @brief ʵ���ַ����滻
 *        ��src_str�е�����target�Ӵ��滻��replace_to
 *
 * @param src_str : const std::string &
 *        Դ�ַ���
 * @param target : const std::string &
 *        �滻Ŀ��
 * @param replace_to : const std::string &
 *        �滻Ŀ��Ҫ���滻�ɵ��ַ���
 *
 * @return std::string
 * @retval �滻����ַ���
 *
 */
std::string string_replace(const std::string& src_str,
        const std::string& target, const std::string& replace_to);

/**
 * @brief �ַ������ִ���ͳ��
 *        ͳ��src�г��ֵ�target�Ĵ���
 *
 * @param src : const std::string &
 *        Դ�ַ���
 * @param target : const std::string &
 *        �������ַ���
 *
 * @return int
 * @retval ���ֵĴ�����δ����Ϊ0
 *
 */
int string_count(const std::string& src, const std::string& target);

/**
 * @brief �ж��ַ���src_str�ǲ�����prefixǰ׺��ʼ
 *
 * @param [in] src_str : const std::string &
 *        Ҫƥ����ַ���
 * @param [in] prefix : const std::string &
 *        Ҫƥ���ǰ׺
 *
 * @return bool
 * @retval true if 'src_str' is start with 'prefix'
 */
bool string_start_with(const std::string& src_str, const std::string& prefix);
 
} // end namespace wrpc
 
#endif // WRPC_UTILS_STRING_UTILS_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

