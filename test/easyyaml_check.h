/// \file
/// \brief easyyaml check common utils.


#ifndef EASYYAML_CHECK_INCLUDED
#define EASYYAML_CHECK_INCLUDED


#include <check.h>
#include <stddef.h>


#define EASYYAML_CHECK_SRUNNER_FLAGS CK_VERBOSE
#define EASYYAML_MAX_REC_LEVELS      64
#define EASYYAML_MAX_ALLOC_TRACK     8192


extern char ** new_tags ();
extern char ** add_tag (char ** tags, char * tag);
extern void *  new_fixtures ();
extern void *  add_fixture (void (**fixtures)(), void (*setup)(), void (*teardown)());
extern void    build_suite (char ** tags, void (**fixtures)(), void (*add_tests_fn)(), Suite * s, void * extra);
extern void    free_suite_rec_allocs ();


#endif // EASYYAML_CHECK_INCLUDED
