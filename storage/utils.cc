
#include "storage/utils.h"

START_NS

int Comparer<std::string>::operator()(const std::string &first,
                                      const std::string &second) const {
    return first.compare(second);
}

std::string joinStrings(const std::vector<std::string> &input, const std::string &delim) {
    std::stringstream out;
    int i = 0;
    for (auto s : input) {
        if (i++ > 0) out << delim;
        out << s;
    }
    return out.str();
}

void splitString(const std::string &str,
                 const std::string &delim,
                 std::vector<std::string> &tokens,
                 bool ignore_empty) {
    // GUILTY - https://stackoverflow.com/questions/14265581/parse-split-a-std::string-in-c-using-std::string-delimiter-standard-c
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!ignore_empty || !token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
}

END_NS
