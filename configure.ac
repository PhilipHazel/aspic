dnl Process this file with autoconf to produce a configure script.

dnl This is required at the start; the name is the name of a file
dnl it should be seeing, to verify it is in the same directory.

AC_INIT
AC_CONFIG_SRCDIR([NOTICE])

dnl Checks for programs.

AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_TYPE_SIZE_T

dnl Variables that are substituted

AC_SUBST(LFLAGS)

dnl This must be last; it determines what files are written

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

cat <<EOF

Aspic configuration summary:

    Install prefix ..................... : ${prefix}              
    C preprocessor ..................... : ${CPP}
    C compiler ......................... : ${CC}                           
    Linker ............................. : ${LD}                             
    C preprocessor flags ............... : ${CPPFLAGS}                         
    C compiler flags ................... : ${CFLAGS}
    Linker flags ....................... : ${LDFLAGS}
    Extra libraries .................... : ${LIBS}  

EOF
