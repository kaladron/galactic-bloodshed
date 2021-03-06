
#include <gtest/gtest.h>
#include "storage/storage.h"

using std::cout;
using std::endl;
using std::make_shared;
using namespace Storage;

static shared_ptr<Registry> registry((new Registry())->Add(DefaultCTypes()));
static auto StringType = registry->GetType("string");
static auto Int64Type = registry->GetType("int64");
static auto Int32Type = registry->GetType("int32");
static auto Int16Type = registry->GetType("int16");
static auto UInt64Type = registry->GetType("uint64");
static auto UInt32Type = registry->GetType("uint32");
static auto UInt16Type = registry->GetType("uint16");
static auto UInt8Type = registry->GetType("uint8");
static auto DoubleType = registry->GetType("double");
static auto BoolType = registry->GetType("bool");

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
    auto PlanetConditionsType = make_shared<Type>("PlanetConditions", Type::ProductType({
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
    }));

    auto PlanetType = make_shared<Type>("Planet", Type::ProductType({
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
    }));
    registry->Add(PlanetConditionsType);
    registry->Add(PlanetType);

    auto SectorType = make_shared<Type>("Sector", Type::ProductType({
        { "planet", make_shared<Type>("", Type::RefType(PlanetType)) },
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
    }));
    registry->Add(SectorType);

    // registry->Add(new Schema("Planet", PlanetType, { FieldPath("id") }) ->AddConstraint(null));
    // registry->Add(new Schema("Person", PersonType, { FieldPath("id") }));
};
