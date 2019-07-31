// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef NAMEGEN_H
#define NAMEGEN_H

#include <string>

class NameGenerator {
public:
    virtual ~NameGenerator() {} ;
    class Iterator {
        private:
            NameGenerator *namegen;

        public:
            /*
            typedef std::string             value_type;
            typedef void                    difference_type;
            typedef std::string *           pointer;
            typedef std::string &           reference;
            typedef std::input_iterator_tag iterator_category;
            */

            explicit Iterator(NameGenerator *parent) : namegen(parent) { }

            const std::string operator*() const { return namegen ? namegen->current() : ""; }
            Iterator& operator++() { // preincrement
                if (namegen != nullptr) {
                    if (!namegen->next()) {
                        namegen = nullptr;
                    }
                }
                return *this;
            }
            friend bool operator==(Iterator const& lhs, Iterator const& rhs) {
                return lhs.namegen == rhs.namegen;
            }
            friend bool operator!=(Iterator const& lhs, Iterator const& rhs) {
                return !(lhs==rhs);
            }

            class PostIncResult {
                std::string value;
                public:
                    PostIncResult(const std::string &val) : value(val) {}
                    const std::string & operator*() { return value; }
            };

            PostIncResult operator++(int) {
                PostIncResult ret(namegen ? namegen->current() : "");
                ++*this;
                return ret;
            }
    };
                                          
protected:
    virtual bool next() = 0;
    virtual const std::string &current() = 0;

    auto begin() { return new Iterator(this); }
    auto end() { return new Iterator(nullptr); }
};

class SequentialNameGenerator : NameGenerator {
public:
    SequentialNameGenerator(int startval = 1,
                            const std::string &pref = std::string(""),
                            const std::string &suff = std::string(""),
                            const std::string &nf = std::string("")) 
        : currval(startval),
        prefix(pref),
        suffix(suff),
        numberformat(nf) {}
    virtual bool next();
    virtual const std::string &current() { return current_value; }

private:
    std::string current_value;
    int currval;
    std::string prefix;
    std::string suffix;
    std::string numberformat;
};

class IterativeNameGenerator : NameGenerator {
public:
    IterativeNameGenerator(std::string::iterator start, std::string::iterator end) : head(start), tail(end) {}
    virtual bool next();
    virtual const std::string &current() { return current_value; }

private:
    std::string::iterator head;
    std::string::iterator tail;
    std::string current_value;
};

#endif  // gameobj_h

