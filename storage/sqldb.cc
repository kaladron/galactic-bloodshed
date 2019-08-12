
#include <iostream>
#include <sstream>
#include <fmt/ostream.h>
#include <fmt/format.h>
#include <queue>
#include <boost/range/combine.hpp>
#include "storage/sqldb.h"

SQLDB::SQLDB(const string &path) : dbpath(path), dbhandle(nullptr) {
    db_status = sqlite3_open_v2(dbpath.c_str(), &dbhandle,
                                SQLITE_OPEN_CREATE |
                                    SQLITE_OPEN_READWRITE | 
                                    SQLITE_OPEN_SHAREDCACHE,
                                nullptr);
}

shared_ptr<SQLTable> SQLDB::EnsureTable(const Schema *s) {
    if (tables.find(s->Name()) == tables.end()) {
        tables[s->Name()] = processSchema(s);
    }
    return tables[s->Name()];
}

shared_ptr<SQLTable> SQLDB::processSchema(const Schema *s) {
    // Add all top level fields to the node
    SQLTable *table = new SQLTable(this, s->Name(), s);
    FieldPath fp;
    processType(table, s->EntityType(), fp);

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
    table->EnsureTable();
    return shared_ptr<SQLTable>(table);
}

void SQLDB::processType(SQLTable *table, const Type *curr_type,
                        FieldPath &field_path) {
    bool hasChildren = !curr_type->IsTypeFun() || curr_type->Name() == "tuple";
    bool namedChildren = !curr_type->IsTypeFun() && curr_type->Name() != "tuple";

    // Process the node and add necessary columns
    if (curr_type->IsUnion() || !hasChildren) {
        // only a "tag" required if union
        table->AddColumn(field_path, curr_type);
    }

    // Process children
    if (hasChildren) {
        for (int i = 0, s = curr_type->ChildCount(); i < s; i++) {
            NameTypePair ntp = curr_type->GetChild(i);
            string next_name = namedChildren ?
                               ntp.first :
                               string("_") + to_string(i);
            field_path.push_back(next_name);
            processType(table, ntp.second, field_path);
            field_path.pop_back();
        }
    }
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
        last_error = string("Could not prepare sql: ") + string(sqlite3_errmsg(dbhandle));
        return nullptr;
    }
    return stmt;
}


/********************************************************************/
/*                      All things SQLTable related.                */
/********************************************************************/

bool SQLTable::HasColumn(const string &name) const {
    return columns_by_name.find(name) != columns_by_name.end();
}

const SQLTable::Column *SQLTable::AddColumn(const FieldPath &fp, const Type *t) {
    if (columns_by_fp.find(fp) != columns_by_fp.end()) {
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
    string sql = CreationSQL();
    sqlite3_stmt *stmt = db->PrepareSql(sql);
    if (stmt == nullptr) return false;

    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE)
    {
        // last_error = string("Could not create table (%@): ") + table_name + string(sqlite3_errmsg(dbhandle));
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
string SQLTable::CreationSQL() const {
    stringstream sql;
    sql << "CREATE TABLE IF NOT EXIST '" << table_name << "' (" << endl;
    for (auto column : columns) {
        const Type *ftype = column->coltype;
        if (column->index > 0) sql << ", ";
        sql << column->name << " " << endl;
        // TODO - See how to generically:
        // 1. pass constraints
        // 2. pass default values
        // lgg
        if (ftype->Name() == "int") {
            sql << "INT" << " " << endl;
        } else if (ftype->Name() == "long") {
            sql << "INT64" << " " << endl;
        } else if (ftype->Name() == "double") {
            sql << "DOUBLE" << " " << endl;
        } else if (ftype->Name() == "string") {
            sql << "TEXT" << " " << endl;
        } else if (ftype->Name() == "datetime") {
            sql << "DATETIME" << " " << endl;
        } else {
            assert(false && "Invalid child type");
        }

        if (column->required) {
            sql << "NOT NULL " << endl;
        }

        // Get field constraints
        // for (auto constraint : schema->GetConstraints()) { }
        sql << "," << endl;
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

            sql << " REFERENCES " << cval.dst_schema->Name() << " (" << endl;
            sql << joinedColNamesFor(cval.dst_field_paths);
            sql << ")" << endl;
        }
    }

    sql << ")" << endl;
    return sql.str();
};
