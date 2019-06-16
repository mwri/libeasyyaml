lib_LTLIBRARIES = libeasyyaml.la

libeasyyaml_la_SOURCES = easyyaml.c
libeasyyaml_la_LDFLAGS = -export-symbols exports.sym -version-info 0:0:0
libeasyyaml_la_LIBADD = -lyaml
libeasyyaml_la_CFLAGS = -Wall

include_HEADERS = easyyaml.h

CLEANFILES = *.gcda *.gcno
