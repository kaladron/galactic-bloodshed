
#ifndef STORAGE_FWDS_H
#define STORAGE_FWDS_H

#define START_NS namespace Storage {
#define END_NS }

#include <assert.h>
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
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
class Entity;
class Schema;
class Type;
class Value;
class FieldPath;
class MapValue;
class ListValue;
template <typename T>
class LeafValue;
class Collection;
template <typename CollectionType>
class Store;
class MemStore;
class SQLStore;
class MemCollection;
class SQLCollection;

END_NS

#endif

