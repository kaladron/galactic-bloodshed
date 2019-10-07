
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
using StrongType = std::shared_ptr<Type>;
using WeakType = std::weak_ptr<Type>;

using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::vector;
using std::list;
using std::optional;
using std::map;
using std::pair;

using std::ostream;
using std::stringbuf;
using std::string;
using std::stringstream;
using StringVector = vector<string>;

END_NS

#endif

