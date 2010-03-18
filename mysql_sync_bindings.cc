/*
Copyright (C) 2010, Oleg Efimov <efimovov@gmail.com>

See license text in LICENSE file
*/
#include <mysql/mysql.h>

#include <v8.h>
#include <node.h>
#include <node_events.h>

// This 2 lines caused
// "Do not use namespace using-directives. Use using-declarations instead."
// [build/namespaces] [5] error in cpplint.py
using namespace v8;
using namespace node;

// Only for fixing some cpplint.py errors:
// Lines should be <= 80 characters long
// [whitespace/line_length] [2]
// Lines should very rarely be longer than 100 characters
// [whitespace/line_length] [4]
#define THREXC(str) ThrowException(String::New(str))

// static Persistent<String> affectedRows_symbol;
// static Persistent<String> connect_symbol;
// static Persistent<String> close_symbol;
// static Persistent<String> errno_symbol;
// static Persistent<String> error_symbol;
// static Persistent<String> escape_symbol;
// static Persistent<String> fetchResult_symbol;
// static Persistent<String> getInfo_symbol;
// static Persistent<String> lastInsertId_symbol;
// static Persistent<String> query_symbol;
// static Persistent<String> warningCount_symbol;

class MysqlDbSync : public EventEmitter {
  public:
    struct MysqlDbSyncInfo {
        uint64_t client_version;
        const char *client_info;
        uint64_t server_version;
        const char *server_info;
        const char *host_info;
        uint32_t proto_info;
    };

    static void Init(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(New);

        t->Inherit(EventEmitter::constructor_template);
        t->InstanceTemplate()->SetInternalFieldCount(1);

        // connect_symbol = NODE_PSYMBOL("affectedRows");
        // connect_symbol = NODE_PSYMBOL("connect");
        // close_symbol = NODE_PSYMBOL("close");
        // errno_symbol = NODE_PSYMBOL("errno");
        // error_symbol = NODE_PSYMBOL("error");
        // escape_symbol = NODE_PSYMBOL("escape");
        // fetchResult_symbol = NODE_PSYMBOL("fetchResult");
        // getInfo_symbol = NODE_PSYMBOL("getInfo");
        // lastInsertId_symbol = NODE_PSYMBOL("lastInsertId");
        // query_symbol = NODE_PSYMBOL("query");
        // warningCount_symbol = NODE_PSYMBOL("warningCount");

        NODE_SET_PROTOTYPE_METHOD(t, "affectedRows", AffectedRows);
        NODE_SET_PROTOTYPE_METHOD(t, "connect", Connect);
        NODE_SET_PROTOTYPE_METHOD(t, "close", Close);
        NODE_SET_PROTOTYPE_METHOD(t, "errno", Errno);
        NODE_SET_PROTOTYPE_METHOD(t, "error", Error);
        NODE_SET_PROTOTYPE_METHOD(t, "escape", Escape);
        NODE_SET_PROTOTYPE_METHOD(t, "fetchResult", FetchResult);
        NODE_SET_PROTOTYPE_METHOD(t, "getInfo", GetInfo);
        NODE_SET_PROTOTYPE_METHOD(t, "lastInsertId", LastInsertId);
        NODE_SET_PROTOTYPE_METHOD(t, "query", Query);
        NODE_SET_PROTOTYPE_METHOD(t, "warningCount", WarningCount);

        target->Set(String::NewSymbol("MysqlDbSync"), t->GetFunction());
    }

    bool Connect(const char* hostname,
                 const char* user,
                 const char* password,
                 const char* database,
                 uint32_t port,
                 const char* socket) {
        if (_conn) {
            return false;
        }

        _conn = mysql_init(NULL);

        if (!_conn) {
            return false;
        }

        if (!mysql_real_connect(_conn,
                                hostname,
                                user,
                                password,
                                database,
                                port,
                                socket,
                                0)) {
            return false;
        }

        return true;
    }

    void Close() {
        if (_conn) {
            mysql_close(_conn);
            _conn = NULL;
        }
    }

    MysqlDbSyncInfo GetInfo() {
        MysqlDbSyncInfo info;

        info.client_version = mysql_get_client_version();
        info.client_info = mysql_get_client_info();
        info.server_version = mysql_get_server_version(_conn);
        info.server_info = mysql_get_server_info(_conn);
        info.host_info = mysql_get_host_info(_conn);
        info.proto_info = mysql_get_proto_info(_conn);

        return info;
    }

    bool Query(const char* query, int length) {
        if (!_conn) {
            return false;
        }

        int r = mysql_real_query(_conn, query, length);

        if (r == 0) {
            return true;
        }

        return false;
    }

  protected:
    MYSQL *_conn;

    MysqlDbSync() {
        _conn = NULL;
    }

    ~MysqlDbSync() {
        if (_conn) mysql_close(_conn);
    }

    static Handle<Value> New(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = new MysqlDbSync();
        conn->Wrap(args.This());

        return args.This();
    }

    static Handle<Value> AffectedRows(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if (!conn->_conn) {
            return THREXC("Not connected");
        }

        my_ulonglong affected_rows = mysql_affected_rows(conn->_conn);

        if (affected_rows == -1) {
            return THREXC("Error occured in mysql_affected_rows()");
        }

        Local<Value> js_result = Integer::New(affected_rows);

        return scope.Close(js_result);
    }

    static Handle<Value> Connect(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn =
                    ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if ( (args.Length() < 3 || !args[0]->IsString()) ||
             (!args[1]->IsString() || !args[2]->IsString()) ) {
            return THREXC("Must give hostname, user and password as arguments");
        }

        String::Utf8Value hostname(args[0]->ToString());
        String::Utf8Value user(args[1]->ToString());
        String::Utf8Value password(args[2]->ToString());
        String::Utf8Value database(args[3]->ToString());
        uint32_t port = args[4]->IntegerValue();
        String::Utf8Value socket(args[5]->ToString());

        bool r = conn->Connect(*hostname,
                               *user,
                               *password,
                               *database,
                               port,
                               (args[5]->IsString() ? *socket : NULL));

        if (!r) {
            return scope.Close(False());
        }

        return scope.Close(True());
    }

    static Handle<Value> Close(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        conn->Close();

        return Undefined();
    }

    static Handle<Value> Errno(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if (!conn->_conn) {
            return THREXC("Not connected");
        }

        uint32_t errno = mysql_errno(conn->_conn);

        Local<Value> js_result = Integer::New(errno);

        return scope.Close(js_result);
    }

    static Handle<Value> Error(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if (!conn->_conn) {
            return THREXC("Not connected");
        }

        const char *error = mysql_error(conn->_conn);

        Local<Value> js_result = String::New(error);

        return scope.Close(js_result);
    }

    static Handle<Value> Escape(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if (!conn->_conn) {
            return THREXC("Not connected");
        }

        if (args.Length() == 0 || !args[0]->IsString()) {
            return THREXC("Nothing to escape");
        }

        String::Utf8Value str(args[0]);

        int len = args[0]->ToString()->Utf8Length();
        char *result = new char[2*len + 1];
        if (!result) {
            return THREXC("Not enough memory");
        }

        int length = mysql_real_escape_string(conn->_conn, result, *str, len);
        Local<Value> js_result = String::New(result, length);

        delete[] result;

        return scope.Close(js_result);
    }

    static Handle<Value> FetchResult(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        MYSQL_RES *result = mysql_use_result(conn->_conn);

        if (!result) {
            return scope.Close(False());
        }

        MYSQL_FIELD *fields = mysql_fetch_fields(result);
        uint32_t num_fields = mysql_num_fields(result);
        MYSQL_ROW result_row;
        // Only use this with
        // mysql_store_result() instead of mysql_use_result()
        // my_ulonglong num_rows = mysql_num_rows(result);
        int i = 0, j = 0;

        Local<Array> js_result = Array::New();
        Local<Object> js_result_row;
        Local<Value> js_field;

        i = 0;
        while ( result_row = mysql_fetch_row(result) ) {
            js_result_row = Object::New();

            for ( j = 0; j < num_fields; j++ ) {
                switch (fields[j].type) {
                  MYSQL_TYPE_BIT:

                  MYSQL_TYPE_TINY:
                  MYSQL_TYPE_SHORT:
                  MYSQL_TYPE_LONG:

                  MYSQL_TYPE_LONGLONG:
                  MYSQL_TYPE_INT24:
                    js_field = String::New(result_row[j])->ToInteger();
                    break;
                  MYSQL_TYPE_DECIMAL:
                  MYSQL_TYPE_FLOAT:
                  MYSQL_TYPE_DOUBLE:
                    js_field = String::New(result_row[j])->ToNumber();
                    break;
                  // TODO(Sannis): Handle other types, dates in first order
                  /*  MYSQL_TYPE_NULL,   MYSQL_TYPE_TIMESTAMP,
                    MYSQL_TYPE_DATE,   MYSQL_TYPE_TIME,
                    MYSQL_TYPE_DATETIME, MYSQL_TYPE_YEAR,
                    MYSQL_TYPE_NEWDATE, MYSQL_TYPE_VARCHAR,*/
                    /*MYSQL_TYPE_NEWDECIMAL=246,
                    MYSQL_TYPE_ENUM=247,
                    MYSQL_TYPE_SET=248,
                    MYSQL_TYPE_TINY_BLOB=249,
                    MYSQL_TYPE_MEDIUM_BLOB=250,
                    MYSQL_TYPE_LONG_BLOB=251,
                    MYSQL_TYPE_BLOB=252,*/
                  MYSQL_TYPE_VAR_STRING:
                  MYSQL_TYPE_STRING:
                    js_field = String::New(result_row[j]);
                    break;
                    /*MYSQL_TYPE_GEOMETRY=255*/
                  default:
                    js_field = String::New(result_row[j]);
                }

                js_result_row->Set(String::New(fields[j].name), js_field);
            }

            js_result->Set(Integer::New(i), js_result_row);

            i++;
        }

        return scope.Close(js_result);
    }

    static Handle<Value> GetInfo(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        MysqlDbSyncInfo info = conn->GetInfo();

        Local<Object> js_result = Object::New();

        js_result->Set(String::New("client_version"),
                       Integer::New(info.client_version));

        js_result->Set(String::New("client_info"),
                       String::New(info.client_info));

        js_result->Set(String::New("server_version"),
                       Integer::New(info.server_version));

        js_result->Set(String::New("server_info"),
                       String::New(info.server_info));

        js_result->Set(String::New("host_info"),
                       String::New(info.host_info));

        js_result->Set(String::New("proto_info"),
                       Integer::New(info.proto_info));

        return scope.Close(js_result);
    }

    static Handle<Value> LastInsertId(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if (!conn->_conn) {
            return THREXC("Not connected");
        }

        MYSQL_RES *result;
        my_ulonglong insert_id = 0;

        if ( (result = mysql_store_result(conn->_conn)) == 0 &&
             mysql_field_count(conn->_conn) == 0 &&
             mysql_insert_id(conn->_conn) != 0) {
            insert_id = mysql_insert_id(conn->_conn);
        }

        Local<Value> js_result = Integer::New(insert_id);

        return scope.Close(js_result);
    }

    static Handle<Value> Query(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if (args.Length() == 0 || !args[0]->IsString()) {
            return THREXC("First arg of MysqlDbSync.query() must be a string");
        }

        String::Utf8Value query(args[0]->ToString());

        bool r = conn->Query(*query, query.length());

        if (!r) {
            return scope.Close(False());
        }

        return scope.Close(True());
    }

    static Handle<Value> WarningCount(const Arguments& args) {
        HandleScope scope;

        MysqlDbSync *conn = ObjectWrap::Unwrap<MysqlDbSync>(args.This());

        if (!conn->_conn) {
            return THREXC("Not connected");
        }

        uint32_t warning_count = mysql_warning_count(conn->_conn);

        Local<Value> js_result = Integer::New(warning_count);

        return scope.Close(js_result);
    }
};

extern "C" void init(Handle<Object> target) {
    MysqlDbSync::Init(target);
}
