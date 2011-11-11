AC_INIT([cpm], [0.1], [grindars@gmail.com])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AM_PROG_CC_C_O

AC_CHECK_HEADERS([z80ex/z80ex.h], [], [AC_MSG_ERROR([z80ex is required])])
AC_CHECK_LIB([z80ex], [z80ex_get_version], [], [AC_MSG_ERROR([z80ex is required])])

#PKG_CHECK_MODULES([SDL], [sdl])

AC_CHECK_TARGET_TOOL([Z80ASM], [z80asm], [AC_MSG_ERROR([z80asm is required])])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile src/Makefile rom/Makefile])

AC_OUTPUT
