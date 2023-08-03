#include "quickjs.h"


#define countof(x) (sizeof(x) / sizeof((x)[0]))

struct JSString {
    JSRefCountHeader header; /* must come first, 32-bit */
    uint32_t len : 31;
    uint8_t is_wide_char : 1; /* 0 = 8 bits, 1 = 16 bits characters */
    uint32_t hash : 30;
    uint8_t atom_type : 2; /* != 0 if atom, JS_ATOM_TYPE_x */
    uint32_t hash_next; /* atom_index for JS_ATOM_TYPE_SYMBOL */
#ifdef DUMP_LEAKS
    struct list_head link; /* string list */
#endif
    union {
        uint8_t str8[0]; /* 8 bit strings will get an extra null terminator */
        uint16_t str16[0];
    } u;
};

typedef struct JSString JSString;

struct RobotStatusRust {
    char* name;
    char* config;
    struct RobotStatusRust* next_robot_status;
    struct RobotStatusRust* prev_robot_status;
};

struct RustCallback {
        void* dmon;
        uint64_t string_buffer_size;
};

struct RustCallback * create_daemon_connector_impl();

static JSValue js_help(JSContext *ctx,
    int argc, JSValueConst *argv)
{
    return JS_NewString(ctx, "Welcome to ThunderWorld");
}

static JSValue js_create_daemon_connector(JSContext *ctx, 
    JSValueConst this_val,
    int argc, JSValueConst *argv)
{
    struct RustCallback *dcon = create_daemon_connector_impl();
    JSValue connector;
    connector.u.ptr = dcon;
    connector.tag = JS_TAG_BENDER_OBJECT;
    return connector;
}

static JSValue js_connect_to_daemon(JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv) {
    struct RustCallback *rc = (struct RustCallback *)argv[0].u.ptr;
    int port_val;
    JSValue val_string = JS_ToString(ctx, argv[1]);
    struct JSString* str_val = JS_VALUE_GET_STRING(val_string);
    const char* server_val = str_val->u.str8;
    if (JS_ToInt32(ctx, &port_val, argv[2]))
        return JS_EXCEPTION;
    printf("connecting to %s : %i" , server_val, port_val);
    int connection_state = connect_to_daemon(rc, server_val, port_val);
    JSValue jv;
    jv.tag = JS_TAG_INT;
    jv.u.int32 = connection_state;
    return jv;
};

static JSValue js_login(JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv) {
    struct RustCallback *rc = (struct RustCallback *)argv[0].u.ptr;    
    JSValue user_str = JS_ToString(ctx, argv[1]);
    struct JSString* user_jstr = JS_VALUE_GET_STRING(user_str);
    const char* user_cval = user_jstr->u.str8;

    JSValue passwd_str = JS_ToString(ctx, argv[2]);
    struct JSString* passwd_jstr = JS_VALUE_GET_STRING(passwd_str);
    const char* passwd_cval = passwd_jstr->u.str8;
    int reply = login(rc, user_cval, passwd_cval);
    JSValue jv;
    jv.tag = JS_TAG_INT;
    jv.u.int32 = reply;
    return jv;
    }

static JSValue js_gates_status(JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv) {
    struct RustCallback *rc = (struct RustCallback *)argv[0].u.ptr; 
    JSValue server_str = JS_ToString(ctx, argv[1]);
    struct JSString* server_jstr = JS_VALUE_GET_STRING(server_str);
    const char* server_cval = server_jstr->u.str8;

    JSValue reg_str = JS_ToString(ctx, argv[2]);
    struct JSString* reg_jstr = JS_VALUE_GET_STRING(reg_str);
    const char* reg_cval = reg_jstr->u.str8;
    struct RobotStatusRust slist = { NULL, NULL, NULL, NULL}; 
    gates_status(rc, server_cval, reg_cval, &slist);
    struct RobotStatusRust *slist_next = &slist;
    JSValue jgates = JS_NewArray(ctx);
    uint32_t i =0;
    while (slist_next != NULL) {

       if (slist_next->config ==NULL) 
           printf("config %s null",  slist_next->name);    

      JSValue val_array = JS_NewArray(ctx);    
      JSValue val_name = JS_NewString(ctx, slist_next->name);
      JSValue val_config = JS_NewString(ctx, slist_next->config);

      JS_CreateDataPropertyUint32(ctx, jgates, i, val_array, 0);
      JS_CreateDataPropertyUint32(ctx, val_array, 0, val_name, 0);
      JS_CreateDataPropertyUint32(ctx, val_array, 1, val_config, 0);
      i++;
      slist_next = slist_next->next_robot_status;
    }
    return jgates;
    }

static JSValue js_gateway_config(JSContext *ctx, 
    JSValueConst this_val, 
    int argc, JSValueConst *argv) {
    struct RustCallback *rc = (struct RustCallback *)argv[0].u.ptr; 
    JSValue server_str = JS_ToString(ctx, argv[1]);
    struct JSString* server_jstr = JS_VALUE_GET_STRING(server_str);
    const char* server_cval = server_jstr->u.str8;
    JSValue gatename_str = JS_ToString(ctx, argv[2]);
    struct JSString* gatename_jstr = JS_VALUE_GET_STRING(gatename_str);
    const char* gatename_cval = gatename_jstr->u.str8;
    struct RobotStatusRust slist = { NULL, NULL, NULL, NULL};
    char* config_buf = (char *) malloc(65535);
    memset(config_buf, 0, 65535);
    gateway_config_impl(rc, server_cval, gatename_cval, config_buf);
    printf("gateway %s", config_buf);
    JSValue config_val= JS_NewString(ctx, config_buf);
    free(config_buf);
    return config_val;
    }

static const JSCFunctionListEntry js_bmonmate_funcs[] = {
    JS_CFUNC_DEF( "help", 0, js_help ),
    JS_CFUNC_DEF( "create_daemon_connector", 0, js_create_daemon_connector ),
    JS_CFUNC_DEF( "connect_to_daemon", 3, js_connect_to_daemon ),
    JS_CFUNC_DEF( "login", 3, js_login ),
    JS_CFUNC_DEF( "gates_status", 3, js_gates_status ),
    JS_CFUNC_DEF( "gateway_config", 3, js_gateway_config ),
};

static int js_bmonmate_init(JSContext *ctx, JSModuleDef *m)
{
    return JS_SetModuleExportList(ctx, m, js_bmonmate_funcs,
                                  countof(js_bmonmate_funcs));
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_bmonmate
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_bmonmate_init);
    if (!m)
        return NULL;
    JS_AddModuleExportList(ctx, m, js_bmonmate_funcs, countof(js_bmonmate_funcs));
    return m;
}
