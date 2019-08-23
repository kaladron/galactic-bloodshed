
#ifndef UTILS_H
#define UTILS_H

#include "storage/fwds.h"
#include <sstream>

START_NS

// HOW ARE ALL THESE NOT STDLIB?
template <typename T>
struct Comparer {
    int operator()(const T &first, const T &second) const {
        return first - second;
    }
};

#define COMPARER(T) template <>struct Comparer<T> { \
    int operator()(const T &first, const T &second) const;  \
};

COMPARER(std::string)

extern std::string joinStrings(const std::vector<std::string> &input,
                               const std::string &delim);

// TODO: Make this an interator instead
extern void splitString(const std::string &input,
                        const std::string &delim,
                        std::vector<std::string> &out,
                        bool ignore_empty = true);

template <typename IteratorType, typename CmpType>
int IterCompare(IteratorType fbegin, IteratorType fend,
                IteratorType sbegin, IteratorType send,
                const CmpType &comparator) {
                // const function <int(const V&, const V&)> &comparator) {
                // const int (*comparator)(const V &a, const V &b)) {
    auto it1 = fbegin;
    auto it2 = sbegin;
    for (;it1 != fend && it2 != send; it1++, it2++) {
        auto a = *it1;
        auto b = *it2;
        int cmp = comparator(*it1, *it2);
        if (cmp != 0) return cmp;
    }
    if (it1 == fend && it2 == send) return 0;
    else if (it1 == fend) return -1;
    return 1;
}

END_NS

#endif
