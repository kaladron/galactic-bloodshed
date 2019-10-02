
#ifndef STORAGE_FWDS_H
#define STORAGE_FWDS_H

#define START_NS namespace Storage {
#define END_NS }

#include <assert.h>
#include <variant>
#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

START_NS

class Constraint;
class Schema;
class Registry;
class Type;
class Value;
class FieldPath;
class MapValue;
class ListValue;
template <typename T> struct Comparer;
class Collection;
class Store;
class MemStore;
class SQLStore;
class MemCollection;
class SQLCollection;
class DefaultTypes;
class Literal;
template <typename T> class TypedLiteral;
using StrongValue = std::shared_ptr<Value>;
using WeakValue = std::weak_ptr<Value>;
using StrongLiteral = std::shared_ptr<Literal>;
using WeakLiteral = std::weak_ptr<Literal>;

template <typename T> using shared_ptr = std::shared_ptr<T>;
template <typename T> using weak_ptr = std::weak_ptr<T>;
template <typename T> using unique_ptr = std::unique_ptr<T>;
template <typename T> using vector = std::vector<T>;
template <typename T> using list = std::list<T>;
template <typename T> using optional = std::optional<T>;
template <typename K, typename V> using map = std::map<K,V>;
template <typename A, typename B> using pair = std::pair<A,B>;

using ostream = std::ostream;
using stringbuf = std::stringbuf;
using string = std::string;
using stringstream = std::stringstream;
using StringVector = vector<string>;
using NameTypePair = pair<string, weak_ptr<Type>>;
using NameTypeVector = vector<NameTypePair>;
using TypeVector = vector<weak_ptr<Type>>;

END_NS

#endif

