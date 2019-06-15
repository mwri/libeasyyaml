/// \file
/// \brief Simple 'hello universe' use example for libeasyyaml.
///
/// This example shows how to parse a simple YAML config file for a
/// mythical RESTful API service, and the config will look like this:
///
/// version: 1.2.7
/// restapi:
///   port: 80
///   ssl: false
///   base-path: /api
///
/// In this case the version is handled as a string so that it can be
/// a 3 part semantic version, but you could parse it as an integer or
/// float too if it suited.


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

#include "easyyaml.h"


/// The structure the YAML file will be parsed into.

struct hello_config {
  int    valid;
  char * version;
  int    svr_port;
  int    svr_ssl;
  char * svr_base_path;
};


/// Declare callbacks.

static void ey_handle_version (easyyaml_stack * stack, char * val, struct hello_config * cfg);
static void ey_handle_svr_port (easyyaml_stack * stack, int val, struct hello_config * cfg);
static void ey_handle_svr_ssl (easyyaml_stack * stack, char * val, struct hello_config * cfg);
static void ey_handle_svr_base_path (easyyaml_stack * stack, char * val, struct hello_config * cfg);


/// YAML schema.

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


/// Schema parsing callbacks.

static void ey_handle_version (easyyaml_stack * stack, char * val, struct hello_config * cfg)
{
  cfg->version = (char *) malloc(strlen(val) + 1);
  strcpy(cfg->version, val);
}

static void ey_handle_svr_port (easyyaml_stack * stack, int val, struct hello_config * cfg)
{
  cfg->svr_port = val;
}

static void ey_handle_svr_ssl (easyyaml_stack * stack, char * val, struct hello_config * cfg)
{
  if (strcmp(val, "true") == 0) {
    cfg->svr_ssl = 1;
  } else if (strcmp(val, "false") == 0) {
    cfg->svr_ssl = 0;
  } else {
    fprintf(stderr, "ey_hello_universe: reading configuration, /restapi/ssl not value (must be 'true' or 'false')\n");
    cfg->valid = 0;
    cfg->svr_port = -1;
  }
}

static void ey_handle_svr_base_path (easyyaml_stack * stack, char * val, struct hello_config * cfg)
{
  cfg->svr_base_path = (char *) malloc(strlen(val) + 1);
  strcpy(cfg->svr_base_path, val);
}


/// Parse the file and return the resultant configuration structure.

int main (int argc, char ** argv)
{
  // easyyaml_set_loglevel(EASYYAML_LOG_LEVEL_TRACE);

  struct hello_config * cfg = malloc(sizeof(struct hello_config));

  cfg->valid = 1;

  // For unit tests.
  char * filename = argv[1];
  if (filename == NULL) {
    filename = (char *) malloc(strlen(argv[0]) + strlen(".yml") + 1);
    sprintf(filename, "%s.yml", argv[0]);
    printf("%s\n", filename);
  }

  if (easyyaml_parse_file(filename, schema(), cfg) != EASYYAML_SUCCESS || !cfg->valid) {
    fprintf(stderr, "ey_hello_universe: reading configuration failed, sorry\n");
    exit(1);
  }

  printf("ey_hello_universe: success, version %s config (restapi SSL=%s, port=%d, URL=%s)\n",
         cfg->version, (cfg->svr_ssl ? "true" : "false"), cfg->svr_port, cfg->svr_base_path
         );

  exit(0);
}
