AC_INIT([libeasyyaml], 1.0.1)
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT

AC_CONFIG_MACRO_DIRS([m4])

PKG_CHECK_MODULES([CHECK], [check], have_check=yes)

AC_PROG_CC
AC_PROG_CC_STDC

AC_CHECK_LIB([yaml], [yaml_parser_initialize], [], [exit 1])

AC_DEFINE([MAX_LOGMSG_LEN], [1024], [Maximum log message length])
AC_DEFINE([MAX_STACKPATH_LEN], [1024], [Maximum stack path length (returned by easyyaml_stack_path)])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES(Makefile src/Makefile test/Makefile)

AC_OUTPUT
