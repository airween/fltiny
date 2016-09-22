AC_INIT([fltiny], [0.1], [airween@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_PROG_INSTALL
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h fcntl.h stdarg.h stdio.h stdlib.h string.h termios.h \
                  unistd.h xmlrpc-c/base.h xmlrpc-c/client_global.h xmlrpc-c/client.h])

AC_CHECK_LIB([xmlrpc], [xmlrpc_init], [
        AC_DEFINE([HAVE_LIBXMLRPC], [1],
                [Define to 1 if you have the `xmlrpc' library (-lxmlrpc).])
        AC_SUBST([LIBXMLRPC_LIB], [-lxmlrpc])], [
        AC_MSG_ERROR([xmlrpc library not found...])])

AC_CHECK_LIB([xmlrpc_util], [xmlrpc_env_init], [
        AC_DEFINE([HAVE_LIBXMLRPC_UTIL], [1],
                [Define to 1 if you have the `xmlrpc_util' library (-lxmlrpc_util).])
        AC_SUBST([LIBXMLRPC_UTIL_LIB], [-lxmlrpc_util])], [
        AC_MSG_ERROR([xmlrpc_util library not found...])])

AC_CHECK_LIB([xmlrpc_client], [xmlrpc_client_create], [
        AC_DEFINE([HAVE_LIBXMLRPC_CLIENT], [1],
                [Define to 1 if you have the `xmlrpc_client' library (-lxmlrpc_client).])
        AC_SUBST([LIBXMLRPC_CLIENT_LIB], [-lxmlrpc_client])], [
        AC_MSG_ERROR([xmlrpc_client library not found...])])


AC_OUTPUT
