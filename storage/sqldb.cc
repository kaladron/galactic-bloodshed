
#include <iostream>
#include <sstream>
#include <boost/range/combine.hpp>
#include "storage/storage.h"
#include "storage/sqldb.h"

START_NS

static void WriteLiteral(const Type * /* type */, const Literal *lit, ostream &out);

SQLDB::SQLDB(const string &path) : dbpath(path), dbhandle(nullptr) {
    db_status = sqlite3_open_v2(dbpath.c_str(), &dbhandle,
                                SQLITE_OPEN_CREATE |
                                    SQLITE_OPEN_READWRITE | 
                                    SQLITE_OPEN_SHAREDCACHE,
                                nullptr);
    std::cout << "DB Status: " << db_status << std::endl;
}

shared_ptr<SQLTable> SQLDB::EnsureTable(const Schema *s) {
    if (tables.find(s->FQN()) == tables.end()) {
        tables[s->FQN()] = processSchema(s);
    }
    return tables[s->FQN()];
}

shared_ptr<SQLTable> SQLDB::processSchema(const Schema *s) {
    // Add all top level fields to the node
    auto table = std::make_shared<SQLTable>(this, s->FQN(), s);

    // Now process table constraints
    for (auto constraint : s->GetConstraints()) {
        if (constraint->IsRequired()) {
            SQLTable::Column *col = const_cast<SQLTable::Column *>(table->ColumnFor(constraint->AsRequired().field_path));
            col->required = true;
        }
        else if (constraint->IsDefaultValue()) {
            const Constraint::DefaultValue &cval = constraint->AsDefaultValue();
            SQLTable::Column *col = const_cast<SQLTable::Column *>(table->ColumnFor(cval.field_path));
            if (cval.onread) {
                col->default_read_value = cval.value;
            } else {
                col->default_write_value = cval.value;
            }
        }
        else if (constraint->IsForeignKey()) {
            // const Constraint::ForeignKey &cval = constraint->AsForeignKey();
        }
    }

    // Kick off its creation
    if (!table->EnsureTable()) {
        std::cerr << "Could not create table (" << table->Name() << "): " << sqlite3_errmsg(dbhandle) << std::endl;
    }
    return table;
}

void SQLDB::CloseStatement(sqlite3_stmt *&stmt) {
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    stmt = nullptr;
}

sqlite3_stmt *SQLDB::PrepareSql(const string &sql_str) {
    sqlite3_stmt *stmt = nullptr;
    int result = sqlite3_prepare_v2(dbhandle, sql_str.c_str(), -1, &stmt, nullptr);
    // last_query = sql_str;
    if (log_queries) {
        std::cout << "Prepared Sql: " << sql_str << std::endl;
    }
    if (result != SQLITE_OK)
    {
        std::cerr << "Could not prepare sql: " << sqlite3_errmsg(dbhandle) << std::endl;
        return nullptr;
    }
    return stmt;
}


/********************************************************************/
/*                      All things SQLTable related.                */
/********************************************************************/

SQLTable::SQLTable(SQLDB *db_, const string &name, const Schema *s) 
        : db(db_), table_name(name), schema(s) {
    FieldPath fp;
    processType(schema->EntityType(), fp);
}

void SQLTable::processType(const Type *curr_type, FieldPath &field_path) {
    const string &fqn = curr_type->FQN();
    bool hasChildren = !curr_type->IsTypeFun() || fqn == "tuple";
    bool namedChildren = !curr_type->IsTypeFun() && fqn != "tuple";

    // Add a column for the current FP.  
    // Note: This is also required for non literal types because for:
    //  Unions - this will be a tag into the sub type
    //  Records - Will denote NULL or not
    // The other way to model records/unions is to make these a 1:1 relationship
    Column *newcolumn = nullptr;
    if (field_path.size() > 0) 
        newcolumn = const_cast<Column *>(AddColumn(field_path, curr_type));

    // Process children
    if (hasChildren) {
        for (int i = 0, s = curr_type->ChildCount(); i < s; i++) {
            NameTypePair ntp = curr_type->GetChild(i);
            string next_name = namedChildren ?
                               ntp.first :
                               string("_") + std::to_string(i);
            field_path.push_back(next_name);
            processType(ntp.second, field_path);
            field_path.pop_back();
        }
    }
    if (newcolumn) {
        newcolumn->endIndex = columns.size();
    }
}

bool SQLTable::HasColumn(const string &name) const {
    return columns_by_name.find(name) != columns_by_name.end();
}

const SQLTable::Column *SQLTable::AddColumn(const FieldPath &fp, const Type *t) {
    auto it = columns_by_fp.find(fp);
    std::cout << "Adding column: " << fp.join() << ", Found: " << (it == columns_by_fp.end()) << std::endl;
    if (it == columns_by_fp.end()) {
        // do our thing
        Column *column = new Column();
        column->endIndex = column->index = columns.size();
        column->name = fp.join("_");
        column->field_path = fp;
        column->coltype = t;
        columns.push_back(column);
        columns_by_fp[fp] = column;
        columns_by_name[column->name] = column;
    }
    return columns_by_fp[fp];
}

const SQLTable::Column *SQLTable::ColumnAt(size_t index) const {
    return columns[index];
}

const SQLTable::Column *SQLTable::ColumnFor(const string &name) const {
    auto it = columns_by_name.find(name);
    if (it == columns_by_name.end()) return nullptr;
    return it->second;
}

const SQLTable::Column *SQLTable::ColumnFor(const FieldPath &fp) const {
    auto it = columns_by_fp.find(fp);
    if (it == columns_by_fp.end()) return nullptr;
    return it->second;
}

bool SQLTable::EnsureTable() {
    string sql = TableCreationSQL();
    sqlite3_stmt *stmt = db->PrepareSql(sql);
    if (stmt == nullptr) return false;

    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        return false;
    }
    db->CloseStatement(stmt);
    return true;
}

string SQLTable::joinedColNamesFor(const list <FieldPath> &field_paths) const {
    stringstream out;
    int i = 0;
    for (auto fp : field_paths) {
        if (i++ > 0) out << ", ";
        const Column *col = ColumnFor(fp);
        out << col->name;
    }
    return out.str();
}

/**
 * Returns the sql to create this table.
 */
string SQLTable::TableCreationSQL() const {
    stringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS '" << table_name << "' (" << std::endl;
    for (auto column : columns) {
        const Type *ftype = column->coltype;
        if (column->index > 0) sql << ",";
        sql << column->name << " ";
        // TODO - See how to generically:
        // 1. pass constraints
        // 2. pass default values
        // lgg
        auto fqn = ftype->FQN();
        if (fqn == "bool") {
            sql << "BOOLEAN" << " " << std::endl;
        } else if (fqn == "int8" || fqn == "int16" || fqn == "int32" ||
                   fqn == "uint8" || fqn == "uint16" || fqn == "uint32") {
            sql << "INT" << " " << std::endl;
        } else if (fqn == "int64" || fqn == "uint64") {
            sql << "INT64" << " " << std::endl;
        } else if (fqn == "float" || fqn == "double") {
            sql << "REAL" << " " << std::endl;
        } else if (fqn == "string") {
            sql << "TEXT" << " " << std::endl;
        } else if (ftype->IsRecord()) {
            sql << "BOOLEAN" << " " << std::endl;
        } else if (ftype->IsUnion()) {
            sql << "INT8" << " " << std::endl;
        } else {
            assert(false && "Invalid child type");
        }

        if (column->required) {
            sql << "NOT NULL " << std::endl;
        }

        // Get field constraints
        // for (auto constraint : schema->GetConstraints()) { }
        // sql << "," << std::endl;
    }

    // Setup the primary key constraint
    sql << ", PRIMARY KEY (";
    int colindex = 0;
    for (auto fp : schema->KeyFields()) {
        if (colindex++ > 0) sql << ", ";
        const Column *col = ColumnFor(fp);
        if (!col) {
            sql << "NULL";  // an error
        } else {
            sql << col->Name();
        }
    }
    sql << ")" << std::endl;


    // Now add table constraints
    for (auto constraint : schema->GetConstraints()) {
        if (constraint->IsUniqueness()) {
            const Constraint::Uniqueness &cval = constraint->AsUniqueness();
            sql << ", UNIQUE (";
            sql << joinedColNamesFor(cval.field_paths);
            sql << ")" << std::endl;
        }
        else if (constraint->IsForeignKey()) {
            const Constraint::ForeignKey &cval = constraint->AsForeignKey();
            sql << ", FOREIGN KEY " << " (";
            sql << joinedColNamesFor(cval.src_field_paths);
            sql << ")" << std::endl;

            sql << " REFERENCES " << cval.dst_schema->FQN() << " (" << std::endl;
            sql << joinedColNamesFor(cval.dst_field_paths);
            sql << ")" << std::endl;
        }
    }

    sql << ")" << std::endl;
    return sql.str();
};

// Get the key from the entity
bool SQLTable::Put(StrongValue entity) const {
    StrongValue key = schema->GetKey(*entity);
    if (!key) {
        // we need to generate a key - let the DB do it
        string sql = InsertionSQL(entity.get());
        sqlite3_stmt *stmt = db->PrepareSql(sql);
        if (stmt == nullptr) return false;
        int result = sqlite3_step(stmt);
        if (result != SQLITE_DONE)
        {
            return false;
        }
        db->CloseStatement(stmt);
    } else {
        string sql = UpsertionSQL(key.get(), entity.get());
        sqlite3_stmt *stmt = db->PrepareSql(sql);
        if (stmt == nullptr) return false;
        int result = sqlite3_step(stmt);
        if (result != SQLITE_DONE)
        {
            return false;
        }
        db->CloseStatement(stmt);
    }
    return true;
}

/**
 * TODO - LOTS TO DO WRT ESCAPING AND QUOTING ETC.
 */
void WriteLiteral(const Type * /* type */, const Literal *lit, ostream &out) {
    if (lit == nullptr) {
        assert(false && "Expected literal value");
    }
    if (lit->LitType() == LiteralType::String) {
        out << '"' << lit->AsString() << '"';
    } else {
        out << lit->AsString();
    }
}

string SQLTable::InsertionSQL(const Value *entity) const {
    stringstream col_sql, val_sql;

    int ncols = 0;
    MatchTypeAndValue(schema->EntityType(), entity, [this, &ncols, &col_sql, &val_sql]
            (const Type *type, const Value *value, int /* index */, const string * /* key */, FieldPath &fp) {
        if (fp.size() > 0) {
            const Column *col = ColumnFor(fp);
            if (col == nullptr) return false;
            if (ncols++ > 0) {
                col_sql << ", ";
                val_sql << ", ";
            }

            col_sql << col->Name();
            if (type->IsRecord()) {
                // then write "exists/not exists" flag
                val_sql << (value != nullptr);
            } else if (type->IsUnion()) {
                const UnionValue *uv = dynamic_cast<const UnionValue *>(value);
                val_sql << (uv ? uv->Tag() : -1);
            } else if (value != nullptr) {
                auto lit = Literal::From(value);
                WriteLiteral(type, lit, val_sql);
            } else {
                val_sql << "NULL";
            }
        }
        return true;
    });

    stringstream sql;
    sql << "INSERT OR REPLACE INTO '" << table_name << "' (" << col_sql.str() << ") VALUES (" << val_sql.str() << ")";
    return sql.str();
}

string SQLTable::UpsertionSQL(const Value * /*key*/, const Value * /*entity*/) const {
    stringstream sql;
    sql << "UPDATE '" << table_name << " SET ";
    /*
    int ncols = 0;
    DFSWalkValue(entity, [this, ncols, &sql]
    (const Value *value,int index, const string *key, FieldPath &fp) mutable {
        const Column *col = ColumnFor(fp);
        if (col == nullptr) return false;
        if (ncols++ > 0) sql << ", ";
        sql << col->Name() << " = ";
        WriteLiteral(Literal::From(value), sql);
        return true;
    });

    sql << " WHERE ";
    // for each key field set it as a where clause
    int ki = 0;
    for (auto fp : schema->KeyFields()) {
        auto keypart = Literal::From(key->Get(ki));
        if (ki++ > 0) sql << " AND ";
        sql << ColumnFor(fp) << " = ";
        WriteLiteral(keypart, sql);
    }
    */
    return sql.str();

}

string SQLTable::DeletionSQL(const Value * /* key */) const {
    stringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS '" << table_name << "' (" << std::endl;
    return sql.str();
}

StrongValue SQLTable::Get(StrongValue key) const {
    string sql = GetSQL(key.get());
    sqlite3_stmt *stmt = db->PrepareSql(sql);
    if (stmt == nullptr) return nullptr;

    int result = sqlite3_step(stmt);
    StrongValue output;
    if (result == SQLITE_ROW)
    {
        // convert resultset into output result
        auto colcount = sqlite3_column_count(stmt);
        assert(columns.size() == colcount && "Number of columns returned does not match num columns in schema");
        output = resultSetToValue(stmt, true, schema->EntityType(), 0, colcount - 1);
    }
    db->CloseStatement(stmt);
    return output;
}

StrongValue SQLTable::resultSetToValue(sqlite3_stmt *stmt, bool is_root, const Type *currType,
                                int startCol, int endCol) const {

    if (is_root) {
        assert(currType->IsRecord() && "Only record types allowed at the root level");
    }
    if (currType->IsRecord()) {
        bool value = is_root || sqlite3_column_int(stmt, startCol) != 0;
        StrongValue output;
        if (value) {
            output = std::make_shared<MapValue>();
            for (int currCol = startCol+1;currCol <= endCol;) {
                const Column *col = ColumnAt(currCol);
                auto key = col->FP().back();
                auto childvalue = resultSetToValue(stmt, false, col->GetType(), currCol, col->endIndex - 1);
                output->Set(key, childvalue);
                currCol = col->endIndex;
            }
        }
        return output;
    } else if (currType->IsUnion()) {
        int tag = sqlite3_column_int(stmt, startCol);
        StrongValue output;
        if (tag >= 0) {
            // find where child starts for this tag
            int childStart = startCol + 1;
            auto childtype = currType->GetChild(tag);
            const Column *childCol = ColumnAt(childStart);
            for (int i = 0;i < tag;i++) {
                childStart = childCol->endIndex;
                childCol = ColumnAt(childStart);
            }
            // create the corresponding tag
            StrongValue data = resultSetToValue(stmt, false, childtype.second, childStart, childCol->endIndex - 1);
            output = std::make_shared<UnionValue>(tag, data);
        }
        return output;
    } else {
        // literal
        auto fqn = currType->FQN();
        if (fqn == "bool") {
            auto value = sqlite3_column_int(stmt, startCol) != 0;
            return BoolBoxer(value);
        } else if (fqn == "int8") {
            int8_t value = sqlite3_column_int(stmt, startCol);
            return Int8Boxer(value);
        } else if (fqn == "int16") {
            int16_t value = sqlite3_column_int(stmt, startCol);
            return Int16Boxer(value);
        } else if (fqn == "int32") {
            int32_t value = sqlite3_column_int(stmt, startCol);
            return Int32Boxer(value);
        } else if (fqn == "int64") {
            int32_t value = sqlite3_column_int64(stmt, startCol);
            return Int64Boxer(value);
        } else if (fqn == "uint8") {
            uint8_t value = sqlite3_column_int(stmt, startCol);
            return UInt8Boxer(value);
        } else if (fqn == "uint16") {
            uint16_t value = sqlite3_column_int(stmt, startCol);
            return UInt16Boxer(value);
        } else if (fqn == "uint32") {
            uint32_t value = sqlite3_column_int(stmt, startCol);
            return UInt32Boxer(value);
        } else if (fqn == "uint64") {
            uint32_t value = sqlite3_column_int64(stmt, startCol);
            return UInt64Boxer(value);
        } else if (fqn == "float") {
            float value = sqlite3_column_double(stmt, startCol);
            return FloatBoxer(value);
        } else if (fqn == "double") {
            double value = sqlite3_column_double(stmt, startCol);
            return DoubleBoxer(value);
        } else if (fqn == "string") {
            auto value = sqlite3_column_text(stmt, startCol);
            if (value) {
                string s((const char *)value);
                return StringBoxer(s);
            }
        } else {
            assert(false && "Invalid child type");
        }
    }
    return StrongValue();
}

string SQLTable::GetSQL(const Value *key) const {
    stringstream sql;
    sql << "SELECT * from '" << table_name << "' WHERE ";
    const auto &keyfields = schema->KeyFields();
    int nKeyFields = keyfields.size();
    if (nKeyFields == 1) {
    } else {
        assert(key->IsIndexed() && "Key needs indexable children to match keyfields in schema");
        assert(keyfields.size() == key->ChildCount() && "Number of key fields do not match provided key length");
    }
    for (int i = 0;i < nKeyFields;i++) {
        const auto &keyfield = keyfields[i];
        const Column *col = ColumnFor(keyfield);
        sql << col->Name() << " = ";

        const Value *key_value = key;
        if (key->IsIndexed()) {
            // This allows us to pass a key as a "single" value
            // if our key is not a composite key
            key_value = key->Get(i).get();
        }
        // Write the value
        auto litval = Literal::From(key_value);
        if (litval) {
            WriteLiteral(col->GetType(), litval, sql);
        } else {
            assert(false && "TBD");
        }
    }
    return sql.str();
}

END_NS
