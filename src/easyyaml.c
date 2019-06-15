#include <stdio.h>
#include <yaml.h>
#include <errno.h>
#include <stdarg.h>

#include "config.h"
#include "easyyaml.h"


/// Local function declarations.

static int    parse (yaml_parser_t * parser, easyyaml_schema * ys, void * cfg);
static int    scan_tok (yaml_parser_t * parser, yaml_token_t * token);
static int    rec_parse (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * c);
static int    rec_parse_obj (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * c);
static int    rec_parse_obj_varkeys (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * cfg);
static int    rec_parse_obj_fixedkeys (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * cfg);
static int    rec_parse_list (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * c);
static char * tok_to_str (int tok);
static char * stack_render_rec (easyyaml_stack * stack, char * buf, char * buf_of);
static int    error_handler (int err_code, const void * data, const char * reason, const char * errmsg_fmt, ...);

/// Logger, log level and error handler intialisation.

static int logger_loglevel = EASYYAML_LOG_LEVEL_ERROR;
static void (*alt_logger)(int level, const char * fmt) = NULL;
static int (*alt_errhandler)(int err_code, const void * data, const char * reason, const char * errmsg_fmt) = NULL;


/// Set the log level (used only by the default logger).

void easyyaml_set_loglevel (int loglevel)
{
  logger_loglevel = loglevel;
}


/// Replace the logger.

void easyyaml_set_logger (void (*logger)(int, const char *))
{
  alt_logger = logger;
}


/// Replace the error handler.

void easyyaml_set_errhandler (int (*handler)(int, const void *, const char *, const char *))
{
  alt_errhandler = handler;
}


/// Open and parse the YAML file.

int easyyaml_parse_file (const char * filename, easyyaml_schema * ys, void * cfg)
{
  FILE * fh = fopen(filename, "r");
  if (fh == NULL)
    return error_handler(EASYYAML_ERROR_FILEOPEN, filename, strerror(errno), "error opening config file (%s)", strerror(errno));

  yaml_parser_t parser;
  int par_init_retval = yaml_parser_initialize(&parser);
  if (par_init_retval == 0) {
    fclose(fh);
    return error_handler(EASYYAML_ERROR_LIBYAML_INIT, &par_init_retval,
                         "yaml_parser_initialize() returned error",
                         "could not initialise libyaml parser (yaml_parser_initialize() returned %d)", par_init_retval);
  }
  yaml_parser_set_input_file(&parser, fh);

  int parse_retval = parse(&parser, ys, cfg);

  yaml_parser_delete(&parser);
  fclose(fh);

  return parse_retval;
}


/// Parse the zero byte terminated YAML string.

int easyyaml_parse_string (const char * input_string, easyyaml_schema * ys, void * cfg)
{
  yaml_parser_t parser;
  int par_init_retval = yaml_parser_initialize(&parser);
  if (par_init_retval == 0)
    return error_handler(EASYYAML_ERROR_LIBYAML_INIT, &par_init_retval,
                         "yaml_parser_initialize() returned error",
                         "could not initialise libyaml parser (yaml_parser_initialize() returned %d)", par_init_retval);
  yaml_parser_set_input_string(&parser, (const unsigned char *) input_string, strlen(input_string));

  int retval = parse(&parser, ys, cfg);

  yaml_parser_delete(&parser);

  return retval;
}


/// Parse the YAML. Called from \ref easyyaml_parse_file or
/// \ref easyyaml_parse_string to complete the parsing of the source.

int parse (yaml_parser_t * parser, easyyaml_schema * ys, void * cfg)
{
  yaml_token_t token;
  int scan_tok_retval;

  if ((scan_tok_retval = scan_tok(parser, &token)) != EASYYAML_SUCCESS)
    return scan_tok_retval;

  if (token.type != YAML_STREAM_START_TOKEN) {
    int data[2] = {token.type, YAML_STREAM_START_TOKEN};
    int retval = error_handler(EASYYAML_ERROR_PARSE_UNEXPECTED, data,
                               "unexpected token at parse start",
                               "expected libyaml stream start after open but read %s",
                               tok_to_str(token.type));
    yaml_token_delete(&token);

    return retval;
  }
  yaml_token_delete(&token);

  if ((scan_tok_retval = scan_tok(parser, &token)) != EASYYAML_SUCCESS)
    return scan_tok_retval;

  if (token.type == YAML_STREAM_END_TOKEN) {
    yaml_token_delete(&token);

    return EASYYAML_SUCCESS;
  } else if (token.type == YAML_BLOCK_MAPPING_START_TOKEN) {
    yaml_token_delete(&token);

    easyyaml_stack stack;
    stack.key  = NULL;
    stack.prev = NULL;

    return rec_parse_obj(parser, ys, &stack, cfg);
  } else {
    int data[2] = {token.type, YAML_BLOCK_MAPPING_START_TOKEN};
    int retval = error_handler(EASYYAML_ERROR_PARSE_UNEXPECTED, data,
                               "unexpected token at parse start",
                               "expected libyaml block mapping start after stream start but read %s",
                               tok_to_str(token.type));
    yaml_token_delete(&token);

    return retval;
  }
}


/// Recursive parse of a YAML map.

int rec_parse_obj (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * cfg)
{
  while (1) {
    yaml_token_t token;
    int scan_tok_retval;

    if ((scan_tok_retval = scan_tok(parser, &token)) != EASYYAML_SUCCESS)
      return scan_tok_retval;

    if (token.type == YAML_BLOCK_END_TOKEN) {
      yaml_token_delete(&token);

      return EASYYAML_SUCCESS;
    } else if (ys->type == EASYYAML_SCHEMA_END) {
      yaml_token_delete(&token);

      char * stack_path = easyyaml_stack_path(stack);
      void * data[2] = {ys, stack_path};
      int retval = error_handler(EASYYAML_ERROR_SCHEMA_NOCHILDREN, data,
                                 "schema allows no children",
                                 "schema permits no children at %s", stack_path);

      if (retval != EASYYAML_SUCCESS)
        return retval;
    } else if (token.type == YAML_KEY_TOKEN) {
      yaml_token_delete(&token);

      if (ys[1].type == EASYYAML_SCHEMA_END && ys[0].key == NULL) {
        int retval = rec_parse_obj_varkeys (parser, ys, stack, cfg);

        if (retval != EASYYAML_SUCCESS)
          return retval;
      } else {
        int retval = rec_parse_obj_fixedkeys (parser, ys, stack, cfg);

        if (retval != EASYYAML_SUCCESS)
          return retval;
      }
    } else {
      char * stack_path = easyyaml_stack_path(stack);
      char * str_tok = tok_to_str(token.type);
      void * data[3] = {ys, stack_path, str_tok};
      int retval = error_handler(EASYYAML_ERROR_SCHEMA_UNEXPECTED_KEY, data,
                                 "unexpected key",
                                 "key %s unexpected while parsing map at %s",
                                 str_tok, stack_path);

      if (retval != EASYYAML_SUCCESS)
        return retval;
    }
  }
}


/// Recursive parse YAML map variable keys.

int rec_parse_obj_varkeys (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * cfg)
{
  yaml_token_t token;
  int scan_tok_retval;

  if ((scan_tok_retval = scan_tok(parser, &token)) != EASYYAML_SUCCESS)
    return scan_tok_retval;

  if (token.type != YAML_SCALAR_TOKEN) {
    yaml_token_delete(&token);

    int data[2] = {token.type, YAML_SCALAR_TOKEN};
    int retval = error_handler(EASYYAML_ERROR_PARSE_UNEXPECTED, data,
                               "unexpected token parsing body",
                               "expected libyaml map variable key scalar but read %s at %s",
                               tok_to_str(token.type), easyyaml_stack_path(stack));
    if (retval != EASYYAML_SUCCESS)
      return retval;
  }

  yaml_token_t token2;

  if ((scan_tok_retval = scan_tok(parser, &token2)) != EASYYAML_SUCCESS) {
    yaml_token_delete(&token);
    return scan_tok_retval;
  }

  if (token2.type != YAML_VALUE_TOKEN) {
    yaml_token_delete(&token2);
    yaml_token_delete(&token);

    int data[2] = {token2.type, YAML_VALUE_TOKEN};
    int retval = error_handler(EASYYAML_ERROR_PARSE_UNEXPECTED, data,
                               "unexpected token parsing body",
                               "expected libyaml map variable key value but read %s at %s",
                               tok_to_str(token2.type), easyyaml_stack_path(stack));
    if (retval != EASYYAML_SUCCESS)
      return retval;
  }
  yaml_token_delete(&token2);

  easyyaml_stack stack2;
  stack2.key  = (char *) token.data.scalar.value;
  stack2.prev = stack;

  int retval = rec_parse(parser, ys, &stack2, cfg);
  yaml_token_delete(&token);

  return retval;
}


/// Recursive parse YAML map fixed keys.

int rec_parse_obj_fixedkeys (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * cfg)
{
  yaml_token_t token;
  int scan_tok_retval;

  if ((scan_tok_retval = scan_tok(parser, &token)) != EASYYAML_SUCCESS)
    return scan_tok_retval;

  if (token.type != YAML_SCALAR_TOKEN) {
    int data[2] = {token.type, YAML_SCALAR_TOKEN};
    int retval = error_handler(EASYYAML_ERROR_PARSE_UNEXPECTED, data,
                               "unexpected token parsing body",
                               "expected libyaml map fixed key scalar but read %s at %s",
                               tok_to_str(token.type), easyyaml_stack_path(stack));
    if (retval != EASYYAML_SUCCESS) {
      yaml_token_delete(&token);
      return retval;
    }
  }

  for (easyyaml_schema * ys2 = ys; ys2->type != EASYYAML_SCHEMA_END; ys2++) {
    if (strcmp(ys2->key, (char *) token.data.scalar.value) == 0) {
      yaml_token_t token2;

      if ((scan_tok_retval = scan_tok(parser, &token2)) != EASYYAML_SUCCESS) {
        yaml_token_delete(&token);
        return scan_tok_retval;
      }

      if (token2.type != YAML_VALUE_TOKEN) {
        yaml_token_delete(&token2);

        int data[2] = {token2.type, YAML_VALUE_TOKEN};
        int retval = error_handler(EASYYAML_ERROR_PARSE_UNEXPECTED, data,
                                   "unexpected token parsing body",
                                   "expected libyaml map fixed key value but read %s at %s",
                                   tok_to_str(token2.type), easyyaml_stack_path(stack));
        if (retval != EASYYAML_SUCCESS) {
          yaml_token_delete(&token);
          yaml_token_delete(&token2);
          return retval;
        }
      }
      yaml_token_delete(&token);
      yaml_token_delete(&token2);

      easyyaml_stack stack2;
      stack2.key  = ys2->key;
      stack2.prev = stack;
      return rec_parse(parser, ys2, &stack2, cfg);
    }
  }

  char * stack_path = easyyaml_stack_path(stack);
  char * str_tok = tok_to_str(token.type);
  void * data[3] = {ys, stack_path, str_tok};
  int retval = error_handler(EASYYAML_ERROR_SCHEMA_UNEXPECTED_KEY, data,
                             "unexpected key",
                             "key %s unexpected while parsing map at %s",
                             str_tok, stack_path);
  yaml_token_delete(&token);

  return retval;
}


/// Recursive parse of a YAML list.

int rec_parse_list (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * cfg)
{
  while (1) {
    yaml_token_t token;
    int scan_tok_retval;

    if ((scan_tok_retval = scan_tok(parser, &token)) != EASYYAML_SUCCESS)
      return scan_tok_retval;

    if (token.type == YAML_BLOCK_END_TOKEN) {
      yaml_token_delete(&token);

      return EASYYAML_SUCCESS;
    }
    if (token.type != YAML_BLOCK_ENTRY_TOKEN) {
      int data[2] = {token.type, YAML_VALUE_TOKEN};
      int retval = error_handler(EASYYAML_ERROR_PARSE_UNEXPECTED, data,
                                 "unexpected token parsing body",
                                 "expected block entry while parsing list but read %s at %s",
                                 tok_to_str(token.type), easyyaml_stack_path(stack));
      if (retval != EASYYAML_SUCCESS) {
        yaml_token_delete(&token);
        return retval;
      }
    }
    yaml_token_delete(&token);

    for (easyyaml_schema * ys2 = ys; ys2->type != EASYYAML_SCHEMA_END; ys2++) {
      int rec_result = rec_parse(parser, ys2, stack, cfg);
      if (rec_result != EASYYAML_SUCCESS)
        return rec_result;
    }
  }
}


/// Recursive parse of a YAML something (could be anything in this context).

int rec_parse (yaml_parser_t * parser, easyyaml_schema * ys, easyyaml_stack * stack, void * cfg)
{
  yaml_token_t token;
  int scan_tok_retval;

  if ((scan_tok_retval = scan_tok(parser, &token)) != EASYYAML_SUCCESS)
    return scan_tok_retval;

  if (ys->type == EASYYAML_SCHEMA_STR) {
    if (token.type == YAML_SCALAR_TOKEN) {
      if (ys->data != NULL)
        ((void (*)(easyyaml_stack *, char *, void *)) ys->data)(stack, (char *) token.data.scalar.value, cfg);
    } else {
      char * stack_path = easyyaml_stack_path(stack);
      char * str_tok = tok_to_str(token.type);
      void * data[3] = {ys, stack_path, str_tok};
      int retval = error_handler(EASYYAML_ERROR_SCHEMA_MANDATES_STRING, data,
                                 "string mandated by schema",
                                 "%s (%s) must be a string at %s",
                                 ys->key, ys->descr, stack_path);
      if (retval != EASYYAML_SUCCESS) {
        yaml_token_delete(&token);
        return retval;
      }
    }
  } else if (ys->type == EASYYAML_SCHEMA_INT) {
    if (token.type == YAML_SCALAR_TOKEN) {
      if (ys->data != NULL)
        ((void (*)(easyyaml_stack *, int, void *)) ys->data)(stack, atoi((char *) token.data.scalar.value), cfg);
    } else {
      char * stack_path = easyyaml_stack_path(stack);
      char * str_tok = tok_to_str(token.type);
      void * data[3] = {ys, stack_path, str_tok};
      int retval = error_handler(EASYYAML_ERROR_SCHEMA_MANDATES_INT, data,
                                 "integer mandated by schema",
                                 "%s (%s) must be an integer at %s",
                                 ys->key, ys->descr, stack_path);
      if (retval != EASYYAML_SUCCESS) {
        yaml_token_delete(&token);
        return retval;
      }
    }
  } else if (ys->type == EASYYAML_SCHEMA_MAP) {
    if (token.type == YAML_BLOCK_MAPPING_START_TOKEN) {
      int rec_result = rec_parse_obj(parser, ys->data, stack, cfg);
      if (rec_result != EASYYAML_SUCCESS)
        return rec_result;
    } else {
      char * stack_path = easyyaml_stack_path(stack);
      char * str_tok = tok_to_str(token.type);
      void * data[3] = {ys, stack_path, str_tok};
      int retval = error_handler(EASYYAML_ERROR_SCHEMA_MANDATES_MAP, data,
                                 "map mandated by schema",
                                 "%s (%s) must be a map at %s",
                                 ys->key, ys->descr, stack_path);
      if (retval != EASYYAML_SUCCESS) {
        yaml_token_delete(&token);
        return retval;
      }
    }
  } else if (ys->type == EASYYAML_SCHEMA_LST) {
    if (token.type == YAML_BLOCK_SEQUENCE_START_TOKEN) {
      int rec_result = rec_parse_list(parser, ys->data, stack, cfg);
      if (rec_result != EASYYAML_SUCCESS)
        return rec_result;
    } else {
      char * stack_path = easyyaml_stack_path(stack);
      char * str_tok = tok_to_str(token.type);
      void * data[3] = {ys, stack_path, str_tok};
      int retval = error_handler(EASYYAML_ERROR_SCHEMA_MANDATES_LIST, data,
                                 "list mandated by schema",
                                 "%s (%s) must be a list at %s",
                                 ys->key, ys->descr, stack_path);
      if (retval != EASYYAML_SUCCESS) {
        yaml_token_delete(&token);
        return retval;
      }
    }
  } else {
    char * stack_path = easyyaml_stack_path(stack);
    void * data[2] = {ys, stack_path};
    int retval = error_handler(EASYYAML_ERROR_SCHEMA_INVALID, data,
                               "schema invalid",
                               "schema has invalid/corrupt type %d at %s",
                               ys->type, stack_path);
    if (retval != EASYYAML_SUCCESS) {
      yaml_token_delete(&token);
      return retval;
    }
  }

  yaml_token_delete(&token);

  return EASYYAML_SUCCESS;
}


/// Wrapper for yaml_parser_scan, with logging added.

int scan_tok (yaml_parser_t * parser, yaml_token_t * token)
{
  int scan_tok_retval = yaml_parser_scan(parser, token);

  if (scan_tok_retval != 0)
    return EASYYAML_SUCCESS;

  int retval = error_handler(EASYYAML_ERROR_LIBYAML_SCAN, &scan_tok_retval,
                             "yaml_parser_scan() returned error",
                             "error scanning token (yaml_parser_scan() returned %d)",
                             scan_tok_retval);

  easyyaml_log(EASYYAML_LOG_LEVEL_TRACE, "YAML scan read token %s", tok_to_str(token->type));

  return retval;
}


/// Return a string representing the given libyaml token.

char * easyyaml_stack_path (easyyaml_stack * stack)
{
  static char buf[MAX_STACKPATH_LEN];

  if (stack->key == NULL)
    snprintf(buf, MAX_STACKPATH_LEN, "/");
  else
    stack_render_rec(stack, buf, buf + MAX_STACKPATH_LEN);

  return buf;
}


/// Recursively render the stack, called by \ref easyyaml_stack_path.

char * stack_render_rec (easyyaml_stack * stack, char * buf, char * buf_of)
{
  if (stack->key == NULL)
    return buf;

  snprintf(stack_render_rec(stack->prev, buf, buf_of), buf_of - buf, "/%s", stack->key);

  return buf + strlen(buf);
}


/// Return a string representing the given libyaml token.

char * tok_to_str (int tok)
{
  static char buf[12];

  switch (tok) {
  case YAML_STREAM_START_TOKEN:
    return "YAML_STREAM_START_TOKEN";
    break;

  case YAML_STREAM_END_TOKEN:
    return "YAML_STREAM_END_TOKEN";
    break;

  case YAML_BLOCK_SEQUENCE_START_TOKEN:
    return "YAML_BLOCK_SEQUENCE_START_TOKEN";
    break;

  case YAML_BLOCK_MAPPING_START_TOKEN:
    return "YAML_BLOCK_MAPPING_START_TOKEN";
    break;

  case YAML_BLOCK_END_TOKEN:
    return "YAML_BLOCK_END_TOKEN";
    break;

  case YAML_BLOCK_ENTRY_TOKEN:
    return "YAML_BLOCK_ENTRY_TOKEN";
    break;

  case YAML_KEY_TOKEN:
    return "YAML_KEY_TOKEN";
    break;

  case YAML_VALUE_TOKEN:
    return "YAML_VALUE_TOKEN";
    break;

  case YAML_SCALAR_TOKEN:
    return "YAML_SCALAR_TOKEN";
    break;

  default:
    snprintf(buf, 12, "UNKNOWN %d", tok);
    return buf;
    break;
  }
}


/// Log something (the library logs at the 'error' and 'trace' levels).

void easyyaml_log (int level, const char * fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  if (alt_logger == NULL) {
    if (level > logger_loglevel)
      return;

    fprintf(stderr, "libeasyyaml: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    return;
  }

  char msg[MAX_LOGMSG_LEN];
  vsnprintf(msg, MAX_LOGMSG_LEN, fmt, args);

  alt_logger(level, msg);
}


/// Handle an error.

int error_handler (int err_code, const void * data, const char * reason, const char * errmsg_fmt, ...)
{
  va_list args;
  va_start(args, errmsg_fmt);
  char errmsg[MAX_LOGMSG_LEN];
  vsnprintf(errmsg, MAX_LOGMSG_LEN, errmsg_fmt, args);

  if (alt_errhandler == NULL) {
    easyyaml_log(EASYYAML_LOG_LEVEL_ERROR, errmsg);
    return err_code;
  }

  if ((err_code & EASYYAML_ERROR_FATAL_BITS) == 0)
    return alt_errhandler(err_code, data, reason, errmsg);

  alt_errhandler(err_code, data, reason, errmsg);
  return err_code;
}
