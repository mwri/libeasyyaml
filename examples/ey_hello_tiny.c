/// \file
/// \brief Simple 'hello world' use example for libeasyyaml.
///
/// This example shows an absolute bare bones starter YAML file parse
/// example.
///
/// The YAML file to be parsed will have a single value called 'count'
/// which will take an integer value, so it will look like:
///
/// count: 10
///
/// For a more realistic example where a YAML file involving different
/// data types is parsed into a structure, see 'ey_hello_universe'.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "easyyaml.h"


/// Callback to set the count.

static void set_count (easyyaml_stack * stack, int val, int * countp)
{
  *countp = val;
}


/// Parse the file and return the resultant configuration structure.

int main (int argc, char ** argv)
{
  int count;

  // Define the schema, which is a YAML document with just one item called 'count'.
  static EASYYAML_SCHEMA(schema)
    EASYYAML_INT("count", &set_count, "Count"),
    EASYYAML_END();

  // For unit tests.
  char * filename = argv[1];
  if (filename == NULL) {
    filename = (char *) malloc(strlen(argv[0]) + strlen(".yml") + 1);
    sprintf(filename, "%s.yml", argv[0]);
    printf("%s\n", filename);
  }

  // Parse the YAML file, passing the filename, the schema, and
  // a pointer to p where the result will be parsed to.
  if (easyyaml_parse_file(filename, schema, &count) != EASYYAML_SUCCESS) {
    fprintf(stderr, "ey_hello_world: YAML read/parse failed, sorry\n");
    exit(1);
  }

  printf("ey_hello_world: count = %d\n", count);

  exit(0);
}
