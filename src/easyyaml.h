#ifndef EASYYAML_INCLUDED
#define EASYYAML_INCLUDED


#define EASYYAML_SUCCESS                      0x00000000
#define EASYYAML_ERROR_FILEOPEN               0x00001001
#define EASYYAML_ERROR_LIBYAML_INIT           0x00005002
#define EASYYAML_ERROR_LIBYAML_SCAN           0x00005003
#define EASYYAML_ERROR_PARSE_UNEXPECTED       0x00001004
#define EASYYAML_ERROR_SCHEMA_NOCHILDREN      0x00002005
#define EASYYAML_ERROR_SCHEMA_UNEXPECTED_KEY  0x00002006
#define EASYYAML_ERROR_SCHEMA_MANDATES_STRING 0x00002007
#define EASYYAML_ERROR_SCHEMA_MANDATES_INT    0x00002008
#define EASYYAML_ERROR_SCHEMA_MANDATES_MAP    0x00002009
#define EASYYAML_ERROR_SCHEMA_MANDATES_LIST   0x0000200a
#define EASYYAML_ERROR_SCHEMA_INVALID         0x0000200b

#define EASYYAML_ERROR_FATAL_BITS             0x00001000
#define EASYYAML_ERROR_SCHEMA_BITS            0x00002000
#define EASYYAML_ERROR_LIBYAML_BITS           0x00004000


#define EASYYAML_LOG_LEVEL_NONE  0x0000
#define EASYYAML_LOG_LEVEL_ERROR 0x0100
#define EASYYAML_LOG_LEVEL_TRACE 0x0200


#define EASYYAML_SCHEMA_END 0x0
#define EASYYAML_SCHEMA_INT 0x1
#define EASYYAML_SCHEMA_STR 0x2
#define EASYYAML_SCHEMA_MAP 0x4
#define EASYYAML_SCHEMA_LST 0x8


typedef struct easyyaml_stack_st easyyaml_stack;
typedef struct easyyaml_schema_st easyyaml_schema;


typedef struct easyyaml_stack_st {
  char *           key;
  easyyaml_stack * prev;
} easyyaml_stack;


typedef struct easyyaml_schema_st {
  char * key;
  int    type;
  void * data;
  char * descr;
} easyyaml_schema;


extern void   easyyaml_set_loglevel (int loglevel);
extern void   easyyaml_set_logger (void (*logger)(int, const char *));
extern void   easyyaml_set_errhandler (int (*handler)(int, const void *, const char *, const char *));
extern void   easyyaml_log (int level, const char *, ...);
extern int    easyyaml_parse_file (const char * filename, easyyaml_schema * ys, void * cfg);
extern int    easyyaml_parse_string (const char * input_string, easyyaml_schema * ys, void * cfg);
extern char * easyyaml_stack_path (easyyaml_stack * stack);


#define EASYYAML_SCHEMA(name)              easyyaml_schema name[] = {
#define EASYYAML_STR(name, handler, descr) { name, EASYYAML_SCHEMA_STR, handler, descr }
#define EASYYAML_INT(name, handler, descr) { name, EASYYAML_SCHEMA_INT, handler, descr }
#define EASYYAML_MAP(name, child, descr)   { name, EASYYAML_SCHEMA_MAP, child,   descr }
#define EASYYAML_LST(name, child, descr)   { name, EASYYAML_SCHEMA_LST, child,   descr }
#define EASYYAML_END()                     { 0, 0, 0 } }


#endif // EASYYAML_INCLUDED
