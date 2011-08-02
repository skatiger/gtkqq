#include <gqqconfig.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <qqtypes.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <stdlib.h>

//
// Private members
//
typedef struct{
    QQInfo *info;
    GHashTable *ht;

    GString *passwd;    //stored password
    GString *qqnum;     //current user's qq number
    GString *status;    //current user's status

    GString *lastuser;  //The last user's qq number
}GQQConfigPriv;

//
// Preperty ID
//
enum{
    GQQ_CONFIG_PROPERTY_0,
    GQQ_CONFIG_PROPERTY_INFO,
    GQQ_CONFIG_PROPERTY_PASSWD,
    GQQ_CONFIG_PROPERTY_UIN,
    GQQ_CONFIG_PROPERTY_STATUS,
    GQQ_CONFIG_PROPERTY_LASTUSER
};

static void gqq_config_init(GQQConfig *self);
static void gqq_config_class_init(GQQConfigClass *klass, gpointer data);

GType gqq_config_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    static GType type_id = 0;
    if (g_once_init_enter(&g_define_type_id__volatile)) {
            if(type_id == 0){
                    GTypeInfo type_info={
                            sizeof(GQQConfigClass),     /* class size */
                            NULL,                       /* base init*/
                            NULL,                       /* base finalize*/
                            /* class init */
                            (GClassInitFunc)gqq_config_class_init,
                            NULL,                       /* class finalize */
                            NULL,                       /* class data */
                                
                            sizeof(GQQConfig),          /* instance size */
                            0,                          /* prealloc bytes */
                            /* instance init */
                            (GInstanceInitFunc)gqq_config_init,
                            NULL                        /* value table */
                    };
                    type_id = g_type_register_static(
                                G_TYPE_OBJECT,
                                "GQQConfig",
                                &type_info,
                                0);
            }
            g_once_init_leave(&g_define_type_id__volatile, type_id);
    }
    return type_id;
}

GQQConfig* gqq_config_new(QQInfo *info)
{
    if(info == NULL){
        g_error("info == NULL (%s, %d)", __FILE__, __LINE__);
        return NULL;
    }

    GQQConfig *cfg = (GQQConfig*)g_object_new(GQQ_TYPE_CONFIG, "info", info);
    return cfg;
}

/*
 * The getter.
 */
static void gqq_config_getter(GObject *object, guint property_id,  
                                    GValue *value, GParamSpec *pspec)
{
        if(object == NULL || value == NULL || property_id < 0){
                return;
        }
        
        g_debug("GQQConfig getter: %s (%s, %d)", pspec -> name, __FILE__, __LINE__); 
        GQQConfig *obj = G_TYPE_CHECK_INSTANCE_CAST(
                                        object, gqq_config_get_type(), GQQConfig);
        GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(
                                    obj, gqq_config_get_type(), GQQConfigPriv);
        
        switch (property_id)
        {
        case GQQ_CONFIG_PROPERTY_INFO:
            g_value_set_pointer(value, priv -> info);
            break;
        case GQQ_CONFIG_PROPERTY_PASSWD:
            g_value_set_static_string(value, priv -> passwd -> str);
            break;
        case GQQ_CONFIG_PROPERTY_STATUS:
            g_value_set_static_string(value, priv -> status -> str);
            break;
        case GQQ_CONFIG_PROPERTY_UIN:
            g_value_set_static_string(value, priv -> qqnum -> str);
            break;
        case GQQ_CONFIG_PROPERTY_LASTUSER:
            g_value_set_static_string(value, priv -> lastuser -> str);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
        }
}

/*
 * The setter.
 */
static void gqq_config_setter(GObject *object, guint property_id,  
                                 const GValue *value, GParamSpec *pspec)
{
        if(object == NULL || value == NULL || property_id < 0){
                return;
        }
        g_debug("GQQConfig setter: %s (%s, %d)", pspec -> name, __FILE__, __LINE__); 
        GQQConfig *obj = G_TYPE_CHECK_INSTANCE_CAST(
                                        object, gqq_config_get_type(), GQQConfig);
        GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(
                                    obj, gqq_config_get_type(), GQQConfigPriv);
        
        switch (property_id)
        {
        case GQQ_CONFIG_PROPERTY_INFO:
            priv -> info = g_value_get_pointer(value);
            break;
        case GQQ_CONFIG_PROPERTY_PASSWD:
            g_string_truncate(priv -> passwd, 0);
            g_string_append(priv -> passwd, g_value_get_string(value));
            break;
        case GQQ_CONFIG_PROPERTY_STATUS:
            g_string_truncate(priv -> status, 0);
            g_string_append(priv -> status, g_value_get_string(value));
            break;
        case GQQ_CONFIG_PROPERTY_UIN:
            g_string_truncate(priv -> qqnum , 0);
            g_string_append(priv -> qqnum , g_value_get_string(value));
            break;
        case GQQ_CONFIG_PROPERTY_LASTUSER:
            g_string_truncate(priv -> lastuser, 0);
            g_string_append(priv -> lastuser, g_value_get_string(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
        }
}

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_string(v)   (char*) g_value_get_string (v)
#else 
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#endif

//
// Marshal for signal 
// The callback type is:
//      void (*handler)(gpointer instance, const gchar *key, const gchar *value
//                      , gpointer user_data)
//
static void g_cclosure_user_marshal_VOID__STRING_STRING (GClosure     *closure,
                                              GValue       *return_value G_GNUC_UNUSED,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint G_GNUC_UNUSED,
                                              gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__STRING_STRING)(gpointer     data1,
                                                   gpointer     arg_1,
                                                   gpointer     arg_2,
                                                   gpointer     data2);
  register GMarshalFunc_VOID__STRING_STRING callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__STRING_STRING)(marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_string(param_values + 1),
            g_marshal_value_peek_string(param_values + 2),
            data2);
}

//
// Default handler of the 'gtkqq-config-changed' signal
// Just do nothing.
//
static void signal_default_handler(gpointer instance
                                    , const gchar *key
                                    , const GVariant *value
                                    , gpointer usr_data)
{
    //do nothing.
    return;
}
static void gqq_config_init(GQQConfig *self)
{
    GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(self
                                        , GQQ_TYPE_CONFIG, GQQConfigPriv);

    //
    // Store the configuration items.
    // The key is the item name, which is string.
    // The value is GVariant type.
    //
    priv -> ht = g_hash_table_new_full(g_str_hash, g_str_equal
                                , (GDestroyNotify)g_free
                                , (GDestroyNotify)g_free);
    priv -> passwd = g_string_new(NULL);
    priv -> lastuser = g_string_new(NULL);
    priv -> qqnum = g_string_new(NULL);
    priv -> status = g_string_new(NULL);
}

static void gqq_config_class_init(GQQConfigClass *klass, gpointer data)
{
    //add the private members.
    g_type_class_add_private(klass, sizeof(GQQConfigPriv));
    
    G_OBJECT_CLASS(klass)-> get_property = gqq_config_getter;
    G_OBJECT_CLASS(klass)-> set_property = gqq_config_setter;

    //install the 'gtkqq-config-changed' signal
    klass -> signal_default_handler = signal_default_handler;
    klass -> changed_signal_id = 
            g_signal_new("gtkqq-config-changed"
                , G_TYPE_FROM_CLASS(klass) 
                , G_SIGNAL_RUN_LAST     //run after the default handler
                , G_STRUCT_OFFSET(GQQConfigClass, signal_default_handler)
                , NULL, NULL            //no used
                , g_cclosure_user_marshal_VOID__STRING_STRING
                , G_TYPE_NONE
                , 2, G_TYPE_STRING, G_TYPE_STRING
                );
    //install the info property
    GParamSpec *pspec;
    pspec = g_param_spec_pointer("info"
                                , "QQInfo"
                                , "The pointer to the global instance of QQInfo"
                                , G_PARAM_READABLE | G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
    g_object_class_install_property(G_OBJECT_CLASS(klass)
                                    , GQQ_CONFIG_PROPERTY_INFO, pspec);

    //install the passwd property
    pspec = g_param_spec_string("passwd"
                                , "password"
                                , "The password of current user."
                                , ""
                                , G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(G_OBJECT_CLASS(klass)
                                    , GQQ_CONFIG_PROPERTY_PASSWD, pspec);

    //install the qq number property
    pspec = g_param_spec_string("qqnum"
                                , "qq number"
                                , "The qq number of current user."
                                , ""
                                , G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(G_OBJECT_CLASS(klass)
                                    , GQQ_CONFIG_PROPERTY_UIN, pspec);

    //install the status property
    pspec = g_param_spec_string("status"
                                , "status"
                                , "The status of current user."
                                , ""
                                , G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(G_OBJECT_CLASS(klass)
                                    , GQQ_CONFIG_PROPERTY_STATUS, pspec);

    //install the lastuser property
    pspec = g_param_spec_string("lastuser"
                                , "last user's qq number "
                                , "The qq number of the last user who uses this program."
                                , ""
                                , G_PARAM_READABLE | G_PARAM_WRITABLE);
    g_object_class_install_property(G_OBJECT_CLASS(klass)
                                    , GQQ_CONFIG_PROPERTY_LASTUSER, pspec);
}

gint gqq_config_load_last(GQQConfig *cfg)
{
    if(cfg == NULL){
        return -1;
    }
    if(!g_file_test(CONFIGDIR, G_FILE_TEST_EXISTS)){
        /*
         * The program is first run. Create the configure dir.
         */
        g_debug("Config mkdir "CONFIGDIR" (%s, %d)", __FILE__, __LINE__);
        if(-1 == g_mkdir(CONFIGDIR, 0777)){
            g_error("Create config dir %s error!(%s, %d)"
                            , CONFIGDIR, __FILE__, __LINE__);
            return -1;
        }
    }

    gchar buf[500];
    GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(
                                cfg, gqq_config_get_type(), GQQConfigPriv);
    //read lastuser
    g_snprintf(buf, 500, "%s/lastuser", CONFIGDIR);
    g_debug("Config open %s (%s, %d)", buf, __FILE__, __LINE__);
    if(!g_file_test(buf, G_FILE_TEST_EXISTS)){
        //First run
        return 0; 
    }
    gint fd = g_open(buf, O_RDONLY);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    gsize len = (gsize)read(fd, buf, 500);
    g_string_truncate(priv -> lastuser, 0);
    g_string_append_len(priv -> lastuser, buf, len);
    close(fd);

    g_string_truncate(priv -> qqnum, 0);
    g_string_append(priv -> qqnum, priv -> lastuser -> str);

    g_debug("Config: read lastuser. %s (%s, %d)", priv -> qqnum -> str
                                        , __FILE__, __LINE__);
    return gqq_config_load(cfg, priv -> lastuser);
}
gint gqq_config_load(GQQConfig *cfg, GString *qqnum)
{
    if(cfg == NULL || qqnum == NULL){
        return -1;
    }
    if(!g_file_test(CONFIGDIR, G_FILE_TEST_EXISTS)){
        /*
         * The program is first run. Create the configure dir.
         */
        if(-1 == g_mkdir(CONFIGDIR, 0777)){
            g_error("Create config dir %s error!(%s, %d)"
                            , CONFIGDIR, __FILE__, __LINE__);
            return -1;
        }
    }

    gchar *begin, *end;
    gchar *key, *value, *eq, *nl;

    gchar buf[500];
    gsize len;
    g_snprintf(buf, 500, "%s/%s", CONFIGDIR, qqnum -> str);
    if(!g_file_test(buf, G_FILE_TEST_EXISTS)){
        /*
         * This is the first time that this user uses this program.
         */
        if(-1 == g_mkdir(buf, 0777)){
            g_error("Create user config dir %s error!(%s, %d)"
                            , buf, __FILE__, __LINE__);
            return -1;
        }
        g_snprintf(buf, 500, "%s/%s/%s", CONFIGDIR, qqnum -> str, "faces");
        if(-1 == g_mkdir(buf, 0777)){
            g_error("Create dir %s error!(%s, %d)", buf, __FILE__, __LINE__);
            return -1;
        }
        // There is nothing to read. Just return.
        return 0;
    }
   
    GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(
                                cfg, gqq_config_get_type(), GQQConfigPriv);

    //read config
    g_snprintf(buf, 500, "%s/%s/config", CONFIGDIR, qqnum -> str);
    gint fd = g_open(buf, O_RDONLY);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    GString *confstr = g_string_new(NULL);
    while(TRUE){
        len = read(fd, buf, 500);
        if(len <= 0){
            break;
        }
        g_string_append_len(confstr, buf, len);
    }
    close(fd);
    eq = confstr -> str;
    while(TRUE){
        if(confstr -> len <=0){
            break;
        }
        key = eq;
        while(*eq != '\0' && *eq != '=') ++eq;
        if(*eq == '\0'){
            break;
        }
        nl = eq;
        while(*nl != '\0' && *nl != '\n') ++nl;
        if(*nl == '\n'){
            break;
        }
        value = eq + 1;
        *eq = '\0';
        *nl = '\0';
        gqq_config_set_str(cfg, g_strdup(key + 2), g_strdup(value));
        eq = nl + 1; 
    }
    g_debug("Config: read config %s.(%s, %d)", confstr -> str
                            , __FILE__, __LINE__);
    g_string_free(confstr, TRUE);

    //read .passwd
    g_snprintf(buf, 500, "%s/%s/.passwd", CONFIGDIR, qqnum -> str);
    fd = g_open(buf, O_RDONLY);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    len = (gsize)read(fd, buf, 500);
    buf[len] = '\0';
    g_string_truncate(priv -> passwd, 0);
    guchar *depw = g_base64_decode(buf, &len);
    g_string_append_len(priv -> passwd, (const gchar *)depw, len);
    g_free(depw);
    close(fd);

    //read buddies
    g_snprintf(buf, 500, "%s/%s/buddies", CONFIGDIR, qqnum -> str);
    fd = g_open(buf, O_RDONLY);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    GString *tmpstr = g_string_new(NULL);
    while(TRUE){
        len = read(fd, buf, 500);
        if(len <= 0){
            break;
        }
        g_string_append_len(tmpstr, buf, len);
    }
    close(fd);
    begin = tmpstr -> str;
    QQBuddy *bdy = NULL;
    while(TRUE){
        end = g_strstr_len(begin, -1, "\n\r");
        if(end == NULL){
            break;
        }
        *end = '\0';
        bdy = qq_buddy_new_from_string(begin);
        g_ptr_array_add(priv -> info -> buddies, bdy);
        begin = end + 2;
    }
    g_string_free(tmpstr, TRUE);
    
    //read groups
    g_snprintf(buf, 500, "%s/%s/groups", CONFIGDIR, qqnum -> str);
    fd = g_open(buf, O_RDONLY);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    tmpstr = g_string_new(NULL);
    while(TRUE){
        len = read(fd, buf, 500);
        if(len <= 0){
            break;
        }
        g_string_append_len(tmpstr, buf, len);
    }
    close(fd);
    begin = tmpstr -> str;
    QQGroup *grp = NULL;
    while(TRUE){
        end = g_strstr_len(begin, -1, "\n\r}\n\r");
        if(end == NULL){
            break;
        }
        *end = '\0';
        grp = qq_group_new_from_string(begin);
        g_ptr_array_add(priv -> info -> groups, grp);
        begin = end + 5;
    }
    g_string_free(tmpstr, TRUE);
    
    //read categories
    g_snprintf(buf, 500, "%s/%s/categories", CONFIGDIR, qqnum -> str);
    fd = g_open(buf, O_RDONLY);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    tmpstr = g_string_new(NULL);
    while(TRUE){
        len = read(fd, buf, 500);
        if(len <= 0){
            break;
        }
        g_string_append_len(tmpstr, buf, len);
    }
    close(fd);
    begin = tmpstr -> str;
    QQCategory *cate = NULL;
    while(TRUE){
        end = g_strstr_len(begin, -1, "\n\r}\n\r");
        if(end == NULL){
            break;
        }
        *end = '\0';
        cate = qq_category_new_from_string(priv -> info, begin);
        g_ptr_array_add(priv -> info -> categories, cate);
        begin = end + 5;
    }
    g_string_free(tmpstr, TRUE);

    return 0;
}

//
// restart when interupt by signal.
//
static gsize safe_write(gint fd, const gchar *buf, gsize len)
{
    if(fd < 0 || buf == NULL || len <=0){
        return 0;
    }

    gsize haswritten = 0;
    gsize re;
    while(haswritten < len){
        re = (gsize)write(fd, buf + haswritten, len - haswritten);
        if(re == -1){
            g_warning("Write to file error. %s (%s, %d)"
                    , strerror(errno), __FILE__, __LINE__);
            return -1;
        }
        haswritten += re;
    }
    return haswritten;
}

static void ht_foreach(gpointer key, gpointer value, gpointer usrdata)
{
    if(key == NULL || value == NULL || usrdata == NULL){
        return;
    }
    const gchar *kstr = (const gchar *)key;
    const gchar *vstr = (const gchar *)value;
    GString *str = (GString*)usrdata;

    g_string_append_printf(str, "%s=%s\n", kstr, vstr); 
    return;
}
gint gqq_config_save(GQQConfig *cfg)
{
    if(cfg == NULL){
        return -1;
    }
    GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(
                                cfg, gqq_config_get_type(), GQQConfigPriv);
    GHashTable *ht = priv -> ht;
    QQInfo *info = priv -> info;
    gchar buf[500];

    if(info -> me -> qqnumber -> len <= 0){
        // Save nothing.
        return 0; 
    }


    g_snprintf(buf, 500, "%s/%s", CONFIGDIR, info -> me -> qqnumber -> str);
    if(!g_file_test(buf, G_FILE_TEST_EXISTS)){
        /*
         * This is the first time that this user uses this program.
         */
        if(-1 == g_mkdir(buf, 0777)){
            g_error("Create user config dir %s error!(%s, %d)"
                            , buf, __FILE__, __LINE__);
            return -1;
        }
        g_snprintf(buf, 500, "%s/%s/%s", CONFIGDIR
                                , info -> me -> qqnumber -> str, "faces");
        if(-1 == g_mkdir(buf, 0777)){
            g_error("Create dir %s error!(%s, %d)", buf, __FILE__, __LINE__);
            return -1;
        }
        // There is nothing to read. Just return.
        return 0;
    }
   
    if(info -> me == NULL){
        g_debug("No configuration to save. (%s, %d)", __FILE__, __LINE__);
        return 0;
    }
    
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    
    //write config
    g_snprintf(buf, 500, "%s/%s/config", CONFIGDIR, info -> me -> qqnumber -> str);
    gint fd = g_open(buf, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    GString *configstr = g_string_new(NULL);
    g_hash_table_foreach(ht, ht_foreach, configstr);
    if(safe_write(fd, configstr -> str, configstr -> len)
                != configstr -> len){
        g_warning("Write to config error!!(%s, %d)", __FILE__, __LINE__);
    }
    g_string_free(configstr, TRUE);
    close(fd);

    //write .passwd 
    g_snprintf(buf, 500, "%s/%s/.passwd", CONFIGDIR
                                    , info -> me -> qqnumber -> str);
    fd = g_open(buf, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    //Base64 encode
    gchar *b64pw = g_base64_encode((const guchar *)priv -> passwd -> str
                                    , priv -> passwd -> len);
    if(safe_write(fd, b64pw, (gsize)strlen(b64pw)) < 0){
        g_warning("Write to .passwd error!!(%s, %d)", __FILE__, __LINE__);
    }
    g_free(b64pw);
    close(fd);

    //write lastuser
    g_snprintf(buf, 500, "%s/lastuser", CONFIGDIR);
    fd = g_open(buf, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    if(safe_write(fd, priv -> lastuser -> str, priv -> lastuser -> len)
                            != priv -> lastuser -> len){
        g_warning("Write to lastuser error!!(%s, %d)", __FILE__, __LINE__);
    }
    close(fd);

    GString *tmp = NULL;
    guint i;
    //write buddies
    g_snprintf(buf, 500, "%s/%s/buddies", CONFIGDIR
                            , info -> me -> qqnumber -> str);
    fd = g_open(buf, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    QQBuddy *bdy = NULL;
    for(i = 0; i < info -> buddies -> len; ++i){
        bdy = (QQBuddy*)g_ptr_array_index(info -> buddies, i); 
        tmp = qq_buddy_tostring(bdy);
        safe_write(fd, tmp -> str, tmp -> len);
        g_string_free(tmp, TRUE);
    }
    close(fd);

    //write groups
    g_snprintf(buf, 500, "%s/%s/groups", CONFIGDIR
                                        , info -> me -> qqnumber -> str);
    fd = g_open(buf, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    QQGroup *grp= NULL;
    for(i = 0; i < info -> groups -> len; ++i){
        grp = (QQGroup*)g_ptr_array_index(info -> groups, i); 
        tmp = qq_group_tostring(grp);
        safe_write(fd, tmp -> str, tmp -> len);
        g_string_free(tmp, TRUE);
    }
    close(fd);

    //write categories
    g_snprintf(buf, 500, "%s/%s/categories", CONFIGDIR
                                , info -> me -> qqnumber -> str);
    fd = g_open(buf, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if(fd == -1){
        g_error("Open file %s error! %s (%s, %d)", buf, strerror(errno)
                                                , __FILE__, __LINE__);
        return -1;
    }
    QQCategory *cate= NULL;
    for(i = 0; i < info -> categories -> len; ++i){
        cate = (QQCategory*)g_ptr_array_index(info -> categories, i); 
        tmp = qq_category_tostring(cate);
        safe_write(fd, tmp -> str, tmp -> len);
        g_string_free(tmp, TRUE);
    }
    close(fd);

    return 0;
}
gint gqq_config_get_str(GQQConfig *cfg, const gchar *key, const gchar **value)
{
    if(cfg == NULL || key == NULL || value == NULL){
        return -1;
    }

    GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(
                                cfg, gqq_config_get_type(), GQQConfigPriv);
        
    const gchar *v = (const gchar *)g_hash_table_lookup(priv -> ht, key);
    *value = v;
    return 0;
}
gint gqq_config_get_int(GQQConfig *cfg, const gchar *key, gint *value)
{
    if(cfg == NULL || key == NULL || value == NULL){
        return -1;
    }
    const gchar *vstr = NULL;
    if(gqq_config_get_str(cfg, key, &vstr) == -1){
        return -1;
    }
    gchar *end;    
    glong vint = strtol(vstr, &end, 10);
    if(vstr == end){
        return -1;
    }
    *value = (gint)vint;
    return 0;

}
gint gqq_config_get_bool(GQQConfig *cfg, const gchar *key, gboolean *value)
{
    if(cfg == NULL || key == NULL || value == NULL){
        return -1;
    }

    const gchar *vstr = NULL;
    if(gqq_config_get_str(cfg, key, &vstr) == -1){
        return -1;
    }
    if(g_ascii_strcasecmp(vstr, "true") == 0){
        *value = TRUE;
    }else if(g_ascii_strcasecmp(vstr, "false") == 0){
        *value = FALSE;
    }
    return 0;

}
gint gqq_config_set_str(GQQConfig *cfg, const gchar *key, const gchar *value)
{
    if(cfg == NULL || key == NULL || value == NULL){
        return -1;
    }

    GQQConfigPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE(
                                cfg, gqq_config_get_type(), GQQConfigPriv);
        
    g_hash_table_replace(priv -> ht, g_strdup(key), g_strdup(value));
    g_signal_emit_by_name(cfg, "gtkqq-config-changed", key, value);
    return 0;
}
gint gqq_config_set_int(GQQConfig *cfg, const gchar *key, gint value)
{
    if(cfg == NULL || key == NULL){
        return -1;
    }
    gchar buf[100];
    g_snprintf(buf, 100, "%d", value);
    return gqq_config_set_str(cfg, key, buf);

}
gint gqq_config_set_bool(GQQConfig *cfg, const gchar *key, gboolean value)
{
    if(cfg == NULL || key == NULL){
        return -1;
    }
    if(value){
        return gqq_config_set_str(cfg, key, "true");
    }else{
        return gqq_config_set_str(cfg, key, "false");
    }
}
