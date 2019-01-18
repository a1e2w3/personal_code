/**
 * @file string_utils.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 13:13:23
 * @brief 
 *  
 **/
 
#include "string_utils.h"

#include <sstream>
 
namespace wrpc {
 
std::string trim_chars(const std::string& str, const std::string& chars_to_trim) {
    if (str.empty()) {
        return str;
    }
    size_t first = str.find_first_not_of(chars_to_trim);
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(chars_to_trim);
    if (last == std::string::npos) {
        return "";
    }
    return str.substr(first, (last - first + 1));
}

std::string trim(const std::string& str) {
    return trim_chars(str, " \n\r\t");
}

int split_2_kv(const std::string& line, char delimiter, std::string* key, std::string* value) {
    if (NULL == key || NULL == value) {
        return -1;
    }
    int pos = line.find(delimiter);
    if (pos == std::string::npos) {
        return -1;
    }
    *key = line.substr(0, pos);
    *value = line.substr(pos + 1);
    return 0;
}

int split_2_kv(const std::string& line, const std::string& delimiter,
        std::string* key, std::string* value) {
    if (NULL == key || NULL == value) {
        return -1;
    }
    int pos = line.find(delimiter);
    if (pos == std::string::npos) {
        return -1;
    }
    *key = line.substr(0, pos);
    *value = line.substr(pos + delimiter.length());
    return 0;
}

size_t string_split(const std::string& src_str, char delimiter,
        std::vector<std::string>* out_vec) {
    if (NULL == out_vec) {
        return 0;
    }
    out_vec->clear();
    std::string::size_type start = 0;
    std::string::size_type size = src_str.length();
    while (start != std::string::npos && start < size) {
        std::string::size_type pos = src_str.find_first_of(delimiter, start);
        if (pos == std::string::npos) {
            std::string token = src_str.substr(start);
            out_vec->push_back(token);
            start = pos;
        } else {
            std::string token = src_str.substr(start, pos - start);
            out_vec->push_back(token);
            start = pos + 1;
        }
    }
    return out_vec->size();
}

size_t string_split(const std::string& src_str, const std::string& delimiter,
        std::vector<std::string>* out_vec) {
    if (src_str.empty() || delimiter.empty() || NULL == out_vec) {
        return 0;
    }
    size_t start = 0;
    size_t src_len = src_str.size();
    size_t deli_len = delimiter.size();
    size_t token_cnt = 0;
    while (start != std::string::npos && start < src_len) {
        const char* cursor = src_str.c_str() + start;
        size_t pos = src_str.find(delimiter, start);
        if (pos == std::string::npos) {
            out_vec->push_back(src_str.substr(start));
            start = pos;
        } else {
            out_vec->push_back(src_str.substr(start, pos - start));
            start = pos + deli_len;
        }
        ++token_cnt;
    }
    return token_cnt;
}

static std::string string_join_part(const std::vector<std::string>& splits, const std::string& delimiter,
        size_t start, size_t end) {
    size_t splits_len = splits.size();
    if (start >= splits_len || start >= end) {
        return "";
    }
    std::ostringstream oss;
    oss << splits[start];
    size_t real_end = std::min(splits_len, end);
    for (size_t i = start + 1; i < real_end; ++i) {
        oss << delimiter << splits[i];
    }
    return oss.str();
}

std::string string_join(const std::vector<std::string>& splits, const std::string& delimiter) {
    return string_join_part(splits, delimiter, 0, splits.size());
}

std::string string_replace(const std::string& src_str,
        const std::string& target, const std::string& replace_to) {
    if (src_str.empty() || target.empty()) {
        return src_str;
    }
    std::ostringstream oss;
    size_t start = 0;
    size_t src_len = src_str.size();
    size_t target_len = target.size();
    while (start != std::string::npos && start < src_len) {
        const char* cursor = src_str.c_str() + start;
        size_t pos = src_str.find(target, start);
        if (pos == std::string::npos) {
            oss.write(cursor, src_len - start);
            start = pos;
        } else {
            oss.write(cursor, pos - start);
            oss << replace_to;
            start = pos + target_len;
        }
    }
    return oss.str();
}

int string_count(const std::string& src, const std::string& target) {
    int count = 0;
    size_t pos = src.find(target, 0); // fist occurrence
    while (pos != std::string::npos) {
        count++;
        pos = src.find(target, pos + 1);
    }
    return count;
}

bool string_start_with(const std::string& src_str, const std::string& prefix) {
    if (prefix.length() > src_str.length()) {
        return false;
    }
    for (size_t i = 0; i < prefix.length(); ++i) {
        if (src_str[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

