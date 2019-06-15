#include <stdlib.h>
#include <stdio.h>

#include "easyyaml_check.h"


int    free_list_counter = 0;
void * free_list[EASYYAML_MAX_ALLOC_TRACK];


/// Allocate memory and track it to be freed after test suite run.

void * tracked_malloc (int size)
{
  if (free_list_counter >= EASYYAML_MAX_ALLOC_TRACK) {
    fprintf(stderr, "cannot build test suite, exceeds suite memory tracking, increase EASYYAML_MAX_ALLOC_TRACK or fix issue");
    exit(1);
  }

  void * ptr = malloc(size);
  free_list[free_list_counter++] = ptr;

  return ptr;
}


/// Create new tag sequence.

char ** new_tags ()
{
  char ** tags = (char **) tracked_malloc(sizeof(char *) * EASYYAML_MAX_REC_LEVELS);

  for (int i = 0; i < EASYYAML_MAX_REC_LEVELS; i++)
    tags[i] = NULL;

  return tags;
}


/// Add a tag to the sequence.

char ** add_tag (char ** tags, char * tag)
{
  char ** new_tags = (char **) tracked_malloc(sizeof(char *) * EASYYAML_MAX_REC_LEVELS);
  memcpy(new_tags, tags, sizeof(char *) * EASYYAML_MAX_REC_LEVELS);

  for (int i = 0; i < EASYYAML_MAX_REC_LEVELS; i++)
    if (new_tags[i] == NULL) {
      new_tags[i] = tag;
      return new_tags;
    }

  return NULL;
}


/// Create a new fixture sequence.

void * new_fixtures ()
{
  void (**fixtures)() = (void (**)()) tracked_malloc(sizeof(void (*)()) * EASYYAML_MAX_REC_LEVELS * 2);

  for (int i = 0; i < EASYYAML_MAX_REC_LEVELS * 2; i++)
    fixtures[i] = NULL;

  return fixtures;
}


/// Add a new fixture (setup and teardown function) to the sequence.

void * add_fixture (void (**fixtures)(), void (*setup)(), void (*teardown)())
{
  void (**new_fixtures)() = (void (**)()) tracked_malloc(sizeof(void (*)()) * EASYYAML_MAX_REC_LEVELS * 2);
  memcpy(new_fixtures, fixtures, sizeof(char *) * EASYYAML_MAX_REC_LEVELS);

  for (int i = 0; i < EASYYAML_MAX_REC_LEVELS * 2; i += 2)
    if (new_fixtures[i] == NULL) {
      new_fixtures[i] = setup;
      new_fixtures[i+1] = teardown;
      return new_fixtures;
    }

  return NULL;
}


/// Recurse the test setup.

void build_suite (char ** tags, void (**fixtures)(), void (*add_tests_fn)(), Suite * s, void * extra)
{
  char tag_str[EASYYAML_MAX_REC_LEVELS * 2];

  tag_str[0] = '\0';
  for (int i = 0; i < EASYYAML_MAX_REC_LEVELS; i++)
    if (tags[i] == NULL) {
      break;
    } else {
      if (i > 0)
        strcat(tag_str, " ");
      strcat(tag_str, tags[i]);
    }

  char * tag_str_cpy = (char *) tracked_malloc(strlen(tag_str) + 1);
  strcpy(tag_str_cpy, tag_str);

  TCase * tc = tcase_create(tag_str_cpy);

  for (int i = 0; i < EASYYAML_MAX_REC_LEVELS * 2; i += 2)
    if (fixtures[i] == NULL)
      break;
    else
      tcase_add_checked_fixture(tc, fixtures[i], fixtures[i+1]);

  (add_tests_fn)(tc, s, tags, fixtures, extra);
  suite_add_tcase(s, tc);

  return;
}


/// Free memory used.

void free_suite_rec_allocs ()
{
  while (free_list_counter--)
    free(free_list[free_list_counter]);
}
