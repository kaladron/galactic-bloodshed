
#include <iostream>
#include <sstream>
#include <boost/range/combine.hpp>
#include "storage/storage.h"
#include "storage/sqldb.h"

START_NS

SQLDB::SQLDB(const string &path) : dbpath(path), dbhandle(nullptr) {
    db_status = sqlite3_open_v2(dbpath.c_str(), &dbhandle,
                                SQLITE_OPEN_CREATE |
                                    SQLITE_OPEN_READWRITE | 
                                    SQLITE_OPEN_SHAREDCACHE,
                                nullptr);
    cout << "DB Status: " << db_status << endl;
}

shared_ptr<SQLTable> SQLDB::EnsureTable(const Schema *s) {
    if (tables.find(s->FQN()) == tables.end()) {
        tables[s->FQN()] = processSchema(s);
    }
    return tables[s->FQN()];
}

shared_ptr<SQLTable> SQLDB::processSchema(const Schema *s) {
    // Add all top level fields to the node
    auto table = make_shared<SQLTable>(this, s->FQN(), s);

    // Now process table constraints
    for (auto constraint : s->GetConstraints()) {
        if (constraint->IsRequired()) {
            const SQLTable::Column *col = table->ColumnFor(constraint->AsRequired().field_path);
            ((SQLTable::Column *)col)->required = true;
        }
        else if (constraint->IsDefaultValue()) {
            const Constraint::DefaultValue &cval = constraint->AsDefaultValue();
            const SQLTable::Column *col = table->ColumnFor(cval.field_path);
            if (cval.onread) {
                ((SQLTable::Column *)col)->default_read_value = cval.value;
            } else {
                ((SQLTable::Column *)col)->default_write_value = cval.value;
            }
        }
        else if (constraint->IsForeignKey()) {
            const Constraint::ForeignKey &cval = constraint->AsForeignKey();
        }
    }

    // Kick off its creation
    if (!table->EnsureTable()) {
        cerr << "Could not create table (" << table->Name() << "): " << sqlite3_errmsg(dbhandle) << endl;
    }
    return table;
}

void SQLDB::CloseStatement(sqlite3_stmt *&stmt) {
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    stmt=NULL;
}

sqlite3_stmt *SQLDB::PrepareSql(const string &sql_str) {
    sqlite3_stmt *stmt = NULL;
    int result = sqlite3_prepare_v2(dbhandle, sql_str.c_str(), -1, &stmt, NULL);
    // last_query = sql_str;
    if (log_queries) {
        cout << "Prepared Sql: " << sql_str << endl;
    }
    if (result != SQLITE_OK)
    {
        cerr << "Could not prepare sql: " << sqlite3_errmsg(dbhandle) << endl;
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
    if (field_path.size() > 0) 
        AddColumn(field_path, curr_type);

    // Process children
    if (hasChildren) {
        for (int i = 0, s = curr_type->ChildCount(); i < s; i++) {
            NameTypePair ntp = curr_type->GetChild(i);
            string next_name = namedChildren ?
                               ntp.first :
                               string("_") + to_string(i);
            field_path.push_back(next_name);
            processType(ntp.second, field_path);
            field_path.pop_back();
        }
    }
}

bool SQLTable::HasColumn(const string &name) const {
    return columns_by_name.find(name) != columns_by_name.end();
}

const SQLTable::Column *SQLTable::AddColumn(const FieldPath &fp, const Type *t) {
    auto it = columns_by_fp.find(fp);
    cout << "Adding column: " << fp.join() << ", Found: " << (it == columns_by_fp.end()) << endl;
    if (it == columns_by_fp.end()) {
        // do our thing
        Column *column = new Column();
        column->index = columns.size();
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
    sql << "CREATE TABLE IF NOT EXISTS '" << table_name << "' (" << endl;
    for (auto column : columns) {
        const Type *ftype = column->coltype;
        if (column->index > 0) sql << ",";
        sql << column->name << " ";
        // TODO - See how to generically:
        // 1. pass constraints
        // 2. pass default values
        // lgg
        if (ftype->FQN() == "bool") {
            sql << "BOOLEAN" << " " << endl;
        } else if (ftype->FQN() == "int") {
            sql << "INT" << " " << endl;
        } else if (ftype->FQN() == "long") {
            sql << "INT64" << " " << endl;
        } else if (ftype->FQN() == "double") {
            sql << "DOUBLE" << " " << endl;
        } else if (ftype->FQN() == "string") {
            sql << "TEXT" << " " << endl;
        } else if (ftype->FQN() == "datetime") {
            sql << "DATETIME" << " " << endl;
        } else if (ftype->IsRecord()) {
            sql << "BOOLEAN" << " " << endl;
        } else if (ftype->IsUnion()) {
            sql << "INT8" << " " << endl;
        } else {
            assert(false && "Invalid child type");
        }

        if (column->required) {
            sql << "NOT NULL " << endl;
        }

        // Get field constraints
        // for (auto constraint : schema->GetConstraints()) { }
        // sql << "," << endl;
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
    sql << ")" << endl;


    // Now add table constraints
    int fkey_count = 0;
    for (auto constraint : schema->GetConstraints()) {
        if (constraint->IsUniqueness()) {
            const Constraint::Uniqueness &cval = constraint->AsUniqueness();
            sql << ", UNIQUE (";
            sql << joinedColNamesFor(cval.field_paths);
            sql << ")" << endl;
        }
        else if (constraint->IsForeignKey()) {
            const Constraint::ForeignKey &cval = constraint->AsForeignKey();
            sql << ", FOREIGN KEY " << " (";
            sql << joinedColNamesFor(cval.src_field_paths);
            sql << ")" << endl;

            sql << " REFERENCES " << cval.dst_schema->FQN() << " (" << endl;
            sql << joinedColNamesFor(cval.dst_field_paths);
            sql << ")" << endl;
        }
    }

    sql << ")" << endl;
    return sql.str();
};

// Get the key from the entity
bool SQLTable::Put(Value *entity) const {
    const Value *key = schema->GetKey(*entity);
    if (key == nullptr) {
        // we need to generate a key - let the DB do it
        string sql = InsertionSQL(entity);
        sqlite3_stmt *stmt = db->PrepareSql(sql);
        if (stmt == nullptr) return false;
        int result = sqlite3_step(stmt);
        if (result != SQLITE_DONE)
        {
            return false;
        }
        db->CloseStatement(stmt);
    } else {
        string sql = UpsertionSQL(key, entity);
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
void WriteLiteral(const Type *type, const Literal *lit, ostream &out) {
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
            (const Type *type, const Value *value, int index, const string *key, FieldPath &fp) {
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

string SQLTable::UpsertionSQL(const Value *key, const Value *entity) const {
    stringstream sql;
    int ncols = 0;
    sql << "UPDATE '" << table_name << " SET ";
    /*
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

string SQLTable::DeletionSQL(const Value *key) const {
    stringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS '" << table_name << "' (" << endl;
    return sql.str();
}

bool SQLTable::Get(const Value &key, Value &output) const {
    string sql = GetSQL(key);
    sqlite3_stmt *stmt = db->PrepareSql(sql);
    if (stmt == nullptr) return false;

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        return false;
    }
    // convert resultset into output result
    db->CloseStatement(stmt);
    return true;
}

string SQLTable::GetSQL(const Value &key) const {
    stringstream sql;
    sql << "SELECT * from '" << table_name << "' WHERE ";
    const auto &keyfields = schema->KeyFields();
    int nKeyFields = keyfields.size();
    if (nKeyFields == 1) {
    } else {
        assert(key.IsIndexed() && "Key needs indexable children to match keyfields in schema");
        assert(keyfields.size() == key.ChildCount() && "Number of key fields do not match provided key length");
    }
    for (int i = 0;i < nKeyFields;i++) {
        const auto &keyfield = keyfields[i];
        const Column *col = ColumnFor(keyfield);
        sql << col->Name() << " = ";

        const Value *key_value = &key;
        if (key.IsIndexed()) {
            // This allows us to pass a key as a "single" value
            // if our key is not a composite key
            key_value = key.Get(i);
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
