
#include <gtest/gtest.h>
#include "storage/storage.h"

using std::cout;
using std::endl;
using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static const Type *StringType = registry->GetType("string");
static const Type *Int64Type = registry->GetType("int64");
static const Type *Int32Type = registry->GetType("int32");
static const Type *Int16Type = registry->GetType("int16");
static const Type *UInt64Type = registry->GetType("uint64");
static const Type *UInt32Type = registry->GetType("uint32");
static const Type *UInt16Type = registry->GetType("uint16");
static const Type *UInt8Type = registry->GetType("uint8");
static const Type *DoubleType = registry->GetType("double");
static const Type *BoolType = registry->GetType("bool");

// How do we want to test schemas?
// 1. Define a schema - say from a string or a file
// 2. Do write/read/update/delete/get tests

static shared_ptr<Store> setupTestDb(const char *dbpath = "/vagrant/gamedata.tests.db");
static void RegisterSchemas();

TEST(Schemas, GameSchemas) {
    auto store = setupTestDb();
}

//////////////////  Helper methods

shared_ptr<Store> setupTestDb(const char *dbpath) {
    remove(dbpath);
    RegisterSchemas();
    return std::make_shared<SQLStore>(dbpath);
}

void RegisterSchemas() {
    auto PlanetConditionsType = new Type("PlanetConditions", {
        { "rtemp", Int32Type },
        { "temp", Int32Type, },
        { "methane", Int32Type }, 
        { "oxygen", Int32Type, }, 
        { "co2", Int32Type },
        { "hydrogen", Int32Type }, 
        { "nitrogen", Int32Type }, 
        { "sulfur", Int32Type },
        { "helium", Int32Type }, 
        { "other", Int32Type }, 
        { "toxic", Int32Type }
    });

    auto PlanetType = new Type("Planet", {
        { "planet_id", Int32Type },
        { "Maxx", UInt16Type },
        { "Maxy", UInt16Type },
        { "star_id", Int32Type },
        { "planet_order", Int16Type },
        { "name", StringType },
        { "xpos", DoubleType },
        { "ypos", DoubleType },
        { "popn", Int64Type },
        { "troops", Int64Type },
        { "maxpopn", Int64Type },
        { "total_resources", Int64Type },
        // { "slaved_to", Ref("Player") },
        { "type", UInt8Type }, // TODO enums
        { "expltimer", UInt8Type },
        { "explored", UInt8Type },
        { "conditions", PlanetConditionsType }
    });

    auto SectorType = new Type("Sector", {
        { "planet", Type(PlanetType) },
        { "xpos", Int32Type },
        { "ypos", Int32Type },
        { "eff", Int32Type },
        { "fert", Int32Type },
        { "mobilization", Int32Type },
        { "crystals", Int32Type },
        { "resource", Int32Type },
        { "popn", Int64Type },
        { "troops", Int64Type },
        { "owner", Int32Type },
        { "race", Int32Type },
        { "type", Int32Type },
        { "condition", Int32Type },
    });
    registry->Add(PlanetConditionsType);
    registry->Add(PlanetType);
    registry->Add(SectorType);

    // registry->Add(new Schema("Planet", PlanetType, { FieldPath("id") }) ->AddConstraint(null));
    // registry->Add(new Schema("Person", PersonType, { FieldPath("id") }));
};
