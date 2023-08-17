AC_INIT([libreactor],[0.9.2],[fredrik.widlund@gmail.com])
AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define])

: ${CFLAGS="-Wall -Wextra -Wpedantic"}

AM_PROG_AR
LT_INIT
AM_PROG_CC_C_O

AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_FILES([Makefile libreactor.pc example/Makefile])
AC_OUTPUT
