
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
    SQLTable *table = new SQLTable(this, s->FQN(), s);

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
    return shared_ptr<SQLTable>(table);
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

    // Process the node and add necessary columns
    if (curr_type->IsUnion() || !hasChildren) {
        // only a "tag" required if union
        AddColumn(field_path, curr_type);
    }

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
    if (columns_by_fp.find(fp) == columns_by_fp.end()) {
        // do our thing
        shared_ptr<Column> column = make_shared<Column>();
        column->index = columns.size();
        column->name = fp.join("_");
        column->field_path = fp;
        column->coltype = t;
        columns.push_back(column);
        columns_by_fp[fp] = column;
        columns_by_name[column->name] = column;
    }
    return columns_by_fp[fp].get();
}

const SQLTable::Column *SQLTable::ColumnAt(size_t index) const {
    return columns[index].get();
}

const SQLTable::Column *SQLTable::ColumnFor(const string &name) const {
    if (columns_by_name.find(name) == columns_by_name.end()) return nullptr;
    return columns_by_name[name].get();
}

const SQLTable::Column *SQLTable::ColumnFor(const FieldPath &fp) const {
    if (columns_by_fp.find(fp) == columns_by_fp.end()) return nullptr;
    return columns_by_fp[fp].get();
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
        if (ftype->FQN() == "int") {
            sql << "INT" << " " << endl;
        } else if (ftype->FQN() == "long") {
            sql << "INT64" << " " << endl;
        } else if (ftype->FQN() == "double") {
            sql << "DOUBLE" << " " << endl;
        } else if (ftype->FQN() == "string") {
            sql << "TEXT" << " " << endl;
        } else if (ftype->FQN() == "datetime") {
            sql << "DATETIME" << " " << endl;
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
void WriteLiteral(const Literal *lit, ostream &out) {
    assert(lit != nullptr && "Expected literal value");
    if (lit->LitType() == LiteralType::String) {
        out << '"' << lit->AsString() << '"';
    } else {
        out << lit->AsString();
    }
}

string SQLTable::InsertionSQL(const Value *entity) const {
    stringstream col_sql, val_sql;

    int ncols = 0;
    FieldPath fp;
    DFSWalkValue(entity, fp, 
    [this, ncols, &col_sql, &val_sql](int index, const string *key, const Value *value, FieldPath &fp) mutable {
        const Column *col = ColumnFor(fp);
        if (col == nullptr) return false;
        if (ncols++ > 0) {
            col_sql << ", ";
            val_sql << ", ";
        }
        col_sql << col->Name();
        WriteLiteral(Literal::From(value), val_sql);
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
    FieldPath fp;
    DFSWalkValue(entity, fp, 
    [this, ncols, &sql](int index, const string *key, const Value *value, FieldPath &fp) mutable {
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
    return sql.str();
#if 0
void upsertIntoTable(NSArray *columns,
                     NSArray *values,
                     NSString *where_clause)
{
    // do an update
    for (NSInteger i = 0, first = -1, count = columns.count;i < count;i++)
    {
        id value = [values objectAtIndex:i];
        if (value && value != [NSNull null])
        {
            if (first >= 0)
                [sql_str appendString:@", "];
            [sql_str appendFormat:@"%@=%@", [columns objectAtIndex:i], value];
            first = i;
        }
    }
    f (where_clause)
        [sql_str appendFormat:@" WHERE %@", where_clause];
    sqlite3_stmt *update_sql = prepare_sql(database, sql_str, NO);
    int step_result = sqlite3_step(update_sql);
    if (step_result != SQLITE_DONE)
    {
        NSLog(@"Row Update Error: %s", sqlite3_errmsg(database));
        assert(NO && "Could not perform row insertion");
    }
    int numChanges = sqlite3_changes(database);
    CLOSE_SQL(update_sql);
    if (numChanges <= 0)    // no rows updated so insert
    {
        // do an insert
        NSMutableString *columns_str = [NSMutableString string];
        NSMutableString *values_str = [NSMutableString string];
        for (NSInteger i = 0, first = -1, count = columns.count;i < count;i++)
        {
            id value = [values objectAtIndex:i];
            if (value && value != [NSNull null])
            {
                if (first >= 0)
                {
                    [columns_str appendString:@", "];
                    [values_str appendString:@", "];
                }
                [columns_str appendString:[columns objectAtIndex:i]];
                [values_str appendFormat:@"%@", value];
                first = i;
            }
        }
        NSMutableString *sql_str = [NSMutableString stringWithFormat:@"INSERT OR REPLACE INTO '%@' (%@) VALUES (%@)",tableName, columns_str, values_str];
        sqlite3_stmt *insert_sql = prepare_sql(database, sql_str, NO);
        int step_result = sqlite3_step(insert_sql);
        if (step_result != SQLITE_DONE)
        {
            NSLog(@"Row Insertion Error: %s", sqlite3_errmsg(database));
            assert(NO && "Could not perform row insertion");
        }
        CLOSE_SQL(insert_sql);
    }
}
#endif
    return sql.str();

}

string SQLTable::DeletionSQL(const Value *key) const {
    stringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS '" << table_name << "' (" << endl;
    return sql.str();
}

END_NS
