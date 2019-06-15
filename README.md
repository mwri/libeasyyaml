# libeasyyaml [![Build Status](https://travis-ci.org/mwri/libeasyyaml.svg?branch=master)](https://travis-ci.org/mwri/libeasyyaml) [![Coverage Status](https://coveralls.io/repos/github/mwri/libeasyyaml/badge.svg?branch=master)](https://coveralls.io/github/mwri/libeasyyaml?branch=master)

Allows you to define a schema for, and parse a YAML file, simply and easily.
For each element of the YAML file

## Contents

1. [Examples](#examples).
   1. [Hello world](#hello-world).
   2. [Hello tiny](#hello-tiny).
   3. [Hello universe](#hello-universe).
      1. [Logging](#logging).
2. [Single callback schemas](#single-callback-schemas)
3. [Error handling](#error-handling).
4. [Build](#build).
5. [API](#api).
   1. [Functions](#functions).
      1. [easyyaml_set_loglevel](#easyyaml_set_loglevel).
      2. [easyyaml_set_logger](#easyyaml_set_logger).
      3. [easyyaml_set_errhandler](#easyyaml_set_errhandler).
      4. [easyyaml_log](#easyyaml_log).
      5. [easyyaml_parse_file](#easyyaml_parse_file).
      6. [easyyaml_parse_string](#easyyaml_parse_string).
      7. [easyyaml_stack_path](#easyyaml_stack_path).
   2. [Macros and defines](#macros-and-defines).
      1. [Return codes](#return-codes).
      2. [Log levels](#log-levels).
      3. [Schema definition](#schema-definition).

## Examples

Here's a 'hello world' example, followed by the simplest possible 'hello tiny'
and a 'hello universe' which mops up any still unimplained features.

### Hello world

Here's a simple "hello world" example (skip to [hello_tiny](#hello-tiny) if you
would rather look at an even simpler case) to illustrate what libeasyyaml gives
you, take this simple example YAML file:

```yaml
version: 1.2.7
restapi:
  port: 80
  ssl: false
  base-path: /api
```

...and let's say you want to parse it into this C structure:

```c
struct hello_config {
  int    valid;
  char * version;
  int    svr_port;
  int    svr_ssl;
  char * svr_base_path;
};
```

...and enforce a schema so that errors are reported if unrecognised or out
of place keys are used, etc.

The first thing we must have to do this is a schema, which means building a
`easyyaml_schema` structure which can be passed to *libeasyyaml*, and various
helpers and macros are provided to make this easy.

The schema for the above is returned by this `schema` function:

```c
static easyyaml_schema * schema ()
{
  static EASYYAML_SCHEMA(restapi_ys)
    EASYYAML_INT("port",      &ey_handle_svr_port,      "TCP port"     ),
    EASYYAML_STR("ssl",       &ey_handle_svr_ssl,       "SSL enabled"  ),
    EASYYAML_STR("base-path", &ey_handle_svr_base_path, "URL base path"),
    EASYYAML_END();

  static EASYYAML_SCHEMA(ys)
    EASYYAML_STR("version", &ey_handle_version, "Configuration version"),
    EASYYAML_MAP("restapi", restapi_ys,         "RESTful API restapi"  ),
    EASYYAML_END();

  return ys;
}
```

Here `static EASYYAML_SCHEMA(ys)` declares a static `easyyaml_schema` array
called `ys` which is ended by `EASYYAML_END()`, and all the bits inbetween are
allowed 'elements'. In this case we allow a sub map `restapi` (by way of the
`EASYYAML_MAP("restapi", restapi_ys, "RESTful API restapi")` declaration)
and a KVP (key value pair) with a string value `version` (by way of the
`EASYYAML_STR("version", &ey_handle_version, "Configuration version")`
declaration).

The `restapi_ys`, you've probably already figured this out, is a pointer to
another schema, the schema for everything under `restapi`, which in this case
allows for a `port` integer value and two string values `ssl` and `base-path`.

The `&ey_handle_version` reference is for a function that will put the
value for the `version` KVP into the `hello_config` structure, and can be
defined simply like this:

```c
static void ey_handle_version (easyyaml_stack * stack, char * val, struct hello_config * cfg)
{
  cfg->version = (char *) malloc(strlen(val) + 1);
  strcpy(cfg->version, val);
}
```

The `easyyaml_stack * stack` value will be explained a little later (in the
[hello universe](#hello-universe) example), but for this case all we need is
`val`, which will be set to the value from the YAML file (i.e. *"1.2.7"* from
the example file above) and `cfg`, which is a pointer to your `hello_config`
structure.

Assuming you had callbacks for not just `ey_handle_version` but all the other
YAML schema elements above, you could then simply parse your YAML file into
a `hello_config` structure like this:

```c
struct hello_config cfg;
easyyaml_parse_file(yaml_filename, schema(), &cfg);
```

The [ey_hello_world.c](examples/ey_hello_world.c) is a full working example of
the above, the same schema is used, error checking added which is left out in
aid of simplicity and clarity here.

The function `ey_handle_svr_port` to parse the port can be implemented as follows:

```c
static void ey_handle_svr_port (easyyaml_stack * stack, int val, struct hello_config * cfg)
{
  cfg->svr_port = val;
}
```

The function `ey_handle_svr_ssl` can be implemented as follows:

static void ey_handle_svr_ssl (easyyaml_stack * stack, char * val, struct hello_config * cfg)
{
  if (strcmp(val, "true") == 0) {
    cfg->svr_ssl = 1;
  } else if (strcmp(val, "false") == 0) {
    cfg->svr_ssl = 0;
  }
}

The function `ey_handle_svr_base_path` can be implemented as follows:

static void ey_handle_svr_base_path (easyyaml_stack * stack, char * val, struct hello_config * cfg)
{
  cfg->svr_base_path = (char *) malloc(strlen(val) + 1);
  strcpy(cfg->svr_base_path, val);
}

Note that any string values MUST be copied, as they are allocated by `libyaml`
and will be freed after the callback invocation.

### Hello tiny

Hello tiny is more or less the simplest possible example:

```c
static void set_count (easyyaml_stack * stack, int val, int * countp)
{
  *countp = val;
}

int main (int argc, char ** argv)
{
  int count;

  static EASYYAML_SCHEMA(schema)
    EASYYAML_INT("count", &set_count, "Count"),
    EASYYAML_END();

  if (easyyaml_parse_file(argv[1], schema, &count) != 0) {
    fprintf(stderr, "ey_hello_world: YAML read/parse failed, sorry\n");
    exit(1);
  }

  printf("ey_hello_world: count = %d\n", count);

  exit(0);
}
```

See [ey_hello_tiny.c](examples/ey_hello_tiny.c) for a full compilable working
example.

### Hello universe

Two really obvious things haven't been covered, which are lists, and keys
which don't have a fixed name, for example imagine the following YAML file
which extends the previous [hello world](#hello-world) example by adding
some users with passwords and access privileges:

```yaml
version: 1.2.7
restapi:
  port: 80
  ssl: false
  base-path: /api
users:
  michael:
    password: qwerty
    access:
      - admin
  john:
    password: asdfgh
    access:
      - read
  molly:
    password: zxcvb
    access:
      - write
      - read
```

Clearly under `users` the keys are supposed to be variable, not fixed, so how
do we define a schema for this, and how do we implement the callback for it?

The full schema for the above is:

```c
static easyyaml_schema * schema ()
{
  static EASYYAML_SCHEMA(server_ys)
    EASYYAML_INT("port",      &ey_handle_svr_port,      "TCP port"     ),
    EASYYAML_STR("ssl",       &ey_handle_svr_ssl,       "SSL enabled"  ),
    EASYYAML_STR("base-path", &ey_handle_svr_base_path, "URL base path"),
    EASYYAML_END();

  static EASYYAML_SCHEMA(user_access_ys)
    EASYYAML_STR(NULL, &ey_handle_user_access, "User access privileges"),
    EASYYAML_END();

  static EASYYAML_SCHEMA(user_ys)
    EASYYAML_STR("password", &ey_handle_user_password, "Password"              ),
    EASYYAML_LST("access",   user_access_ys,           "User access privileges"),
    EASYYAML_END();

  static EASYYAML_SCHEMA(users_ys)
    EASYYAML_MAP(NULL, user_ys, "User"),
    EASYYAML_END();

  static EASYYAML_SCHEMA(ys)
    EASYYAML_STR("version", &ey_handle_version, "Configuration version"),
    EASYYAML_MAP("restapi", server_ys,          "RESTful API server"   ),
    EASYYAML_MAP("users",   users_ys,           "Users"                ),
    EASYYAML_END();

  return ys;
}
```

The variable key bit is:

```c
  static EASYYAML_SCHEMA(users_ys)
    EASYYAML_MAP(NULL, user_ys, "User"),
    EASYYAML_END();
```

Pretty obviously where the name would go `NULL` is given instead.

To parse a password for a user though, what's new is that we need to know what
user the password is for, because the `ey_handle_user_password` callback will
get called for the password of each user, and `val` will just contain the value
of the password.

This is where the mysterious `easyyaml_stack * stack` argument passed to all
the callbacks comes in. The `stack` is essentially a linked list of the parent
elements of the YAML parse right the way back to the root, so we can look to
see that the parent element was *"michael"* for example, or some other user.

Here's a callback implementation for `ey_handle_user_password` taken from
[ey_hello_world.c](examples/ey_hello_world.c):

```c
static void ey_handle_user_password (easyyaml_stack * stack, char * val, hello_config * cfg)
{
  for (int i = 0; i < 10; i++) {
    if (cfg->users[i].username == NULL) {
      cfg->users[i].username = (char *) malloc(strlen(stack->prev->key) + 1);
      strcpy(cfg->users[i].username, stack->prev->key);
    }
    if (strcmp(cfg->users[i].username, stack->prev->key) == 0) {
      cfg->users[i].password = (char *) malloc(strlen(val) + 1);
      strcpy(cfg->users[i].password, val);
      return;
    }
  }
  fprintf(stderr, "ey_hello_universe: reading configuration, run out of space for new user \"%s\"\n", stack->prev->key);
  cfg->valid = 0;
}
```

In the above, the first time it is called with the example YAML content, will
be for *"michael"*, and in this case `val` will be *"qwerty"*.

The value of `stack->key` will be *"password"* and the value of `stack->prev->key`
will be *"michael"*. Also `stack->prev->prev->key` will be *"users"* and
`stack->prev->prev->prev->key` will be `NULL`.

The implementation above depends on the definition of the `hello_config` structure
so see [ey_hello_universe.c](examples/ey_hello_universe.c) for full disclosure. It
has an different (extended) definition to `hello_config` in
[ey_hello_world.c](examples/ey_hello_world.c).

#### Logging

By default `libeasyyaml` will log errors to `stderr` and you can increase the
log level to a TRACE level by calling `easyyaml_set_loglevel(EASYYAML_LOG_LEVEL_TRACE)`.
or switch it off with `easyyaml_set_loglevel(EASYYAML_LOG_LEVEL_NONE)`.

Switching it off is a pretty horrible idea though as if a parse fails you'll have no
idea why.

You can replace the logger though which is more useful, and means you can format the
logging as you please, send it to a file instead, invoke your favourite log library, whatever.

To do this call `easyyaml_set_logger(logger)` where logger is the replacement log
function. This log function will be called for all log events of any level. The
'hello universe' example implements a custom logger as follows:

```c
static void ey_logger  (int level, const char * msg)
{
  char * level_str = level == EASYYAML_LOG_LEVEL_ERROR
    ? "error"
    : "trace";

  // if (level > EASYYAML_LOG_LEVEL_ERROR)
  //   return;

  fprintf(stderr, "ey_hello_universe: %s: libeasyyaml: %s\n", level_str, msg);

  return;
}
```

The `level` argument will be `EASYYAML_LOG_LEVEL_ERROR` or `EASYYAML_LOG_LEVEL_TRACE`
and your custom logger must implement any level discrimination itself (the `libeasyyaml`
log level applies only to the default logger).

In this case all log events are printed to `stderr` with some prefixing.

## Single callback parsing

An alternative strategy for parsing, if you prefer it, is providing the exact same
callback function everywhere in your schema, for example the [hello world](#hello-world)
schema would become:

```c
static easyyaml_schema * schema ()
{
  static EASYYAML_SCHEMA(restapi_ys)
    EASYYAML_INT("port",      &ey_callback, "TCP port"     ),
    EASYYAML_STR("ssl",       &ey_callback, "SSL enabled"  ),
    EASYYAML_STR("base-path", &ey_callback, "URL base path"),
    EASYYAML_END();

  static EASYYAML_SCHEMA(ys)
    EASYYAML_STR("version", &ey_callback, "Configuration version"),
    EASYYAML_MAP("restapi", restapi_ys,   "RESTful API restapi"  ),
    EASYYAML_END();

  return ys;
}
```

The callback would then have to interogate the `stack` to get the full path and context
and do the right thing. Also remember that you would have to treat everything as a string
in this case, because an integer can be a string, but a string can't be an integer!

Using the [easyyaml_stack_path](#easyyaml_stack_path) function to convert the stack to
a handy comparable string the callback would look something like:

```c
static void ey_callback (easyyaml_stack * stack, char * val, hello_config * cfg)
{
  char * path = easyyaml_stack_path(stack);

  if (strcmp(path, "/version") == 0) {
    ...
  } else if (strcmp(path, "/restapi/port") == 0) {
    ...
  } else if (strcmp(path, "/restapi/ssl") == 0) {
    ...
  } else if (strcmp(path, "/restapi/base-path") == 0) {
    ...
  } else {
    ...
  }
}
```

## Error handling

As well as replacing the logger, the error handler can also be replaced. Note that
the default error handler is responsible for logging errors, so if you replace the
error handler replacing the logger is redundant; your error handler must perform
logging if you still want any error logging.

To replace the error handler call `easyyaml_set_errhandler(errhandler)` where
`errhandler` is a function like this:

```c
int errhandler (int err_code, const void * data, const char * reason, const char * errmsg)
{
  easyyaml_log(EASYYAML_LOG_LEVEL_ERROR, errmsg);
  return err_code;
}
```

This example actually does just what the default error handler does, it logs
an error and returns the `err_code` value. The `err_code` value will be an
[return code](#return-codes) (but not `EASYYAML_SUCCESS`). *Some* errors can be
overridden, which means that if you return something else then that will be the
new error condition. This is really only useful to return a custom error code to
the original caller, or to return `EASYYAML_SUCCESS`, which means the error will
be quashed. This will in the immediate term cause libeasyyaml to carry on as if
the error did not occur.

If you return a custom error code, choose a value above 0xffff.

## Build

Running `libtoolize` followed by `autoreconf -i` followed by `./configure`
and finally `make` should result in a successful build.

You can run the [hello world](#hello-world) example like this:

```sh
LD_LIBRARY_PATH=./src/.libs ./examples/ey_hello_world examples/ey_hello_world.yml
```

The expected output is:

```text
ey_hello_universe: success, version 1.5.4 config (restapi SSL=false, port=80, URL=/api)
```

You can run the [hello universe](#hello-universe) example like this:

```sh
LD_LIBRARY_PATH=./src/.libs ./examples/ey_hello_universe ./examples/ey_hello_universe.yml
```

The expected output (after the TRACE logging emitted by the custom logger) is:

```text
ey_hello_universe: successfully parsed config (version 1.5.4)
ey_hello_universe: REST API (SSL=false, port=80, URL=/api)
ey_hello_universe:     Users:
ey_hello_universe:         michael (password qwerty)
ey_hello_universe:             admin = YES
ey_hello_universe:              read = YES
ey_hello_universe:             write = YES
ey_hello_universe:         john (password asdfgh)
ey_hello_universe:             admin = NO
ey_hello_universe:              read = YES
ey_hello_universe:             write = NO
ey_hello_universe:         molly (password zxcvb)
ey_hello_universe:             admin = NO
ey_hello_universe:              read = YES
ey_hello_universe:             write = YES
```

## API

### Functions

#### easyyaml_set_loglevel

Set the log level of the default logger:

```c
easyyaml_set_loglevel(level);
```

The `level` parameter can be `EASYYAML_LOG_LEVEL_NONE`, `EASYYAML_LOG_LEVEL_ERROR` or
`EASYYAML_LOG_LEVEL_TRACE`.

#### easyyaml_set_logger

Set the logger:

```
easyyaml_set_logger(logger)
```

The `logger` parameter is a function with this prototype:

```c
void logger (int level, const char * fmt, ...)
```

The function will be called regardless of the log level, and the `level` parameter
tells you what log level this log event is (one of `EASYYAML_LOG_LEVEL_NONE` or
`EASYYAML_LOG_LEVEL_ERROR`).

The `fmt` and following parameters are typical `printf` style and may be handled
with `va_start` and friends, as in the [ey_hello_universe.c](examples/ey_hello_universe.c)
example.

#### easyyaml_set_errhandler

Set the error handler:

```
easyyaml_set_errhandler(errhandler)
```

The `errhandler` function should look like this:

```c
int errhandler (int err_code, const void * data, const char * reason, const char * errmsg)
{
  easyyaml_log(EASYYAML_LOG_LEVEL_ERROR, errmsg);
  return err_code;
}
```
Note the default error handler is responsible for logging errors, so the replacement
error handler must log if error logging is still required, and could do so as in the
example above.

Some errors may be quashed / ignored by returning `EASYYAML_SUCCESS` instead of the
`err_code` value passed.

#### easyyaml_log

Log a message via the current logger. If this is still the default logger the message
will be emitted to `stderr`, otherwise it will result in a call to the custom logger:

```c
easyyaml_log(EASYYAML_LOG_LEVEL_TRACE, "the result of %s was %d", op_descr, res_num);
```

#### easyyaml_parse_file

Parse a YAML file:

```c
int result = easyyaml_parse_file(filename, schema, data);
```

The parameters are the filename, the schema and a pointer which will be passed to
any schema callbacks invoked.

The return value will be `EASYYAML_SUCCESS`, or some other value indicating an error
(see [return codes](#return-codes)).

#### easyyaml_parse_string

Almost the same as [easyyaml_parse_file](#easyyaml_parse_file) but this is to parse
a zero byte terminated string in memory:

```c
int result = easyyaml_parse_string(yaml_string, schema, data);
```

#### easyyaml_stack_path

Convert a `stack` into a description string:

```c
char * path = easyyaml_stack_path(stack);
```

The `stack` parameter is the first argument passed to all schema callbacks, and the
string returned will be a slash separated list of keys, such as *"/users/michael/password"*.

The buffer returned is static and will be overwritten by the next call to `easyyaml_stack_path`
so you must use it immediately or copy it if you retain it.

### Macros and defines

#### Return codes

Integer return codes will be one of:

| Value                                 | Description                                           |
|---------------------------------------|-------------------------------------------------------|
| EASYYAML_SUCCESS                      | Everything was fine                                   |
| EASYYAML_ERROR_FILEOPEN               | Opening the input file failed                         |
| EASYYAML_ERROR_LIBYAML_INIT           | An error occurred initialising a libyaml parser       |
| EASYYAML_ERROR_LIBYAML_SCAN           | An libyaml error occurred scanning for a token        |
| EASYYAML_ERROR_PARSE_UNEXPECTED       | An unexpected token was encountered                   |
| EASYYAML_ERROR_SCHEMA_NOCHILDREN      | Found a child where the schema allows for none        |
| EASYYAML_ERROR_SCHEMA_UNEXPECTED_KEY  | Found a key that the schema does not permit           |
| EASYYAML_ERROR_SCHEMA_MANDATES_STRING | Schema is for a string but something else was found   |
| EASYYAML_ERROR_SCHEMA_MANDATES_INT    | Schema is for an integer but something else was found |
| EASYYAML_ERROR_SCHEMA_MANDATES_MAP    | Schema is for a map but something else was found      |
| EASYYAML_ERROR_SCHEMA_MANDATES_LIST   | Schema is for a list but something else was found     |
| EASYYAML_ERROR_SCHEMA_INVALID         | Schema is for a invalid/corrupt (should not happen)   |

#### Log levels

Log levels will be one of:

  * EASYYAML_LOG_LEVEL_ERROR
  * EASYYAML_LOG_LEVEL_TRACE

#### Schema definition

| Macro                                  | Description                         |
|----------------------------------------|-------------------------------------|
| EASYYAML_SCHEMA(name)                  | Start of a schema (declares `name`) |
| EASYYAML_STR(name, handler, descr)     | A string value                      |
| EASYYAML_INT(name, handler, descr)     | A integer value                     |
| EASYYAML_MAP(name, child, descr)       | A map (with a key `name`)           |
| EASYYAML_LST(name, child, descr)       | A list (with a key `name`)          |
| EASYYAML_END()                         | Terminates a schema declaration     |

In all cases `name` is the name of the key within a map, and may be `NULL` if not a
map context (e.g. a list of strings) or if the name is not fixed.

In all cases `handler` is a callback (which may be NULL if no action is required), with
the following prototype:

```c
void handler (easyyaml_stack * stack, char * val, hello_config * cfg)
```

In all cases `child` is a pointer to another schema (declared with `EASYYAML_SCHEMA(name)`).

In all cases `descr` is a description of the YAML element, it is used in some error
messages and may be used in a future revision to generate documentation.
