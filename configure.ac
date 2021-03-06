AC_INIT(config, version-0.1)

# Check for cc
AC_PROG_CC
AC_LANG(C)
AC_DISABLE_OPTION_CHECKING
# Check for scheduling
AC_CHECK_HEADER(linux/sched.h,,,) 
AC_CHECK_HEADER(pcap/pcap.h,,,) 
AC_CHECK_HEADER(pthread.h,,,) 
AC_CHECK_LIB(m, cos)

# Check if libreadline exists
a=1
AC_CHECK_LIB(readline, readline,,[a=0],)
if test $a == 1
then
	AC_SUBST(USEREADLINE, 1)
fi

# Check if libbroker exists
#a=1
#AC_CHECK_LIB(broker,broker_init,,[a=0],)
#if test $a == 1
#then
#	AC_SUBST(USEBROKER, 1)
#fi

# Check lua-5.1 path (lib & inc)
AC_ARG_WITH(stuff, [  --with-lua-lib 	  path to the liblua5.1 install root])
if test "$with_lua_lib" != ""
then
	AC_SUBST(LUALIBPATH, $with_lua_lib)
else
	if echo `uname` | grep -q "FreeBSD"
	then
		AC_SUBST(LUALIBPATH, /usr/local/lib)
	fi
	if echo `uname` | grep -q "Linux"
	then
		AC_SUBST(LUALIBPATH, /usr/lib)
	fi
fi

a=1
AC_CHECK_LIB(lua-5.1,lua_close,,[a=0],)
if test $a == 1
then
	AC_SUBST(LIBLUANAME, -llua-5.1)
else
	b=1
	AC_CHECK_LIB(lua5.1,lua_close,,[b=0],)
	if test $b == 1
	then
		AC_SUBST(LIBLUANAME, -llua5.1)
	fi
fi

#AC_CHECK_LIB(lua5.1,lua_close,,[a=0],)
#if test $a == 1
#then
#	AC_SUBST(LIBLUANAME, -llua5.1)
#fi 

AC_ARG_WITH(stuff, [  --with-lua-include	  path to the lua5.1 header install root])
if test "$with_lua_include" != ""
then
	AC_SUBST(LUAINCPATH, $with_lua_include)
else
	if echo `uname` | grep -q "FreeBSD"
	then
		AC_SUBST(LUAINCPATH, /usr/local/include/lua51)
	fi
	if echo `uname` | grep -q "Linux"
	then
		AC_SUBST(LUAINCPATH, /usr/include/lua5.1)
	fi
fi

AC_ARG_WITH(stuff, [  --with-broker-include	  path to the broker header install root])
if test "$with_broker_include" != ""
then
	AC_SUBST(BROKERINCPATH, $with_broker_include)
	AC_SUBST(USEBROKER, 1)
fi

AC_ARG_WITH(stuff, [  --with-broker-lib 	  path to the broker library install root])
if test "$with_broker_lib" != ""
then
	AC_SUBST(BROKERLIBPATH, $with_broker_lib)
	AC_SUBST(USEBROKER, 1)
fi

a=1
# Check whether lua-5.1 exists
AC_CHECK_HEADER($LUAINCPATH/lua.h,[AC_DEFINE(HAVE_LUA_H, 1)],[a=0],
[[
#if HAVE_LUA_H
#include <lua5.1/lua.h>
#endif
]])
if test $a == 0
then
	AC_MSG_ERROR([lua.h is missing. Please install lua-5.1 development library.])
fi

# Check if the user wants to use libjemalloc
AC_ARG_WITH(stuff, [  --with-jemalloc-include	  path to the jemalloc header install root])
if test "$with_jemalloc_include" != ""
then
	AC_SUBST(JEINCPATH, $with_jemalloc_include)
fi

# Check if the path to to libjemalloc is valid
a=1
# Check whether lua-5.1 exists
AC_CHECK_HEADER($JEINCPATH/jemalloc.h,[AC_DEFINE(HAVE_JEMALLOC_H, 1)],[a=0],
[[
#if HAVE_JEMALLOC_H
#include <jemalloc/jemalloc.h>
#endif
]])
if test $a == 0
then
	AC_MSG_WARN([jemalloc.h not found.])
fi

# Check if `install' exists
AC_CHECK_PROG(INSTALL_CHECK,install,yes)
if test x"$INSTALL_CHECK" != x"yes" ; then
	AC_MSG_ERROR([Please install `install'.])
else
	AC_SUBST(INSTALL, "install -c")
	AC_SUBST(INSTALL_PROGRAM, "install -c")
	AC_SUBST(INSTALL_DATA, "install -m 644")
fi

# Check if gzip exists 
AC_CHECK_PROG(GZIP_CHECK,gzip,yes)
if test x"$GZIP_CHECK" != x"yes" ; then
   AC_MSG_ERROR([Please install `gzip'.])
fi

# Check if gunzip exists 
AC_CHECK_PROG(GUNZIP_CHECK,gunzip,yes)
if test x"$GUNZIP_CHECK" != x"yes" ; then
   AC_MSG_ERROR([Please install `gunzip'.])
fi

# Check if bash exists
AC_CHECK_PROG(BASH_CHECK,bash,yes)
if test x"$BASH_CHECK" != x"yes" ; then
   AC_MSG_ERROR([Please install `bash'.])
fi

# Check if env exists
AC_CHECK_PROG(ENV_CHECK,env,yes)
if test x"$ENV_CHECK" != x"yes" ; then
   AC_MSG_ERROR([Please install `env'.])
fi

AC_OUTPUT(Makefile)
AC_OUTPUT(src/Makefile)
if echo `uname` | grep -q "FreeBSD"
then
	# Check if `gmake' exists
	AC_CHECK_PROG(GMAKE_CHECK,gmake,yes)
	if test x"$GMAKE_CHECK" != x"yes" ; then
	   AC_MSG_ERROR([Please install `gmake'.])
	fi
	
	# Check if `makewhatis' exists
	AC_CHECK_PROG(MAKEWHATIS_CHECK,makewhatis,yes)
	if test x"$MAKEWHATIS_CHECK" != x"yes" ; then
	   AC_MSG_ERROR([Please install `makewhatis'.])
	fi

	# Check if `gmake' exists
	AC_CHECK_PROG(GMAKE_CHECK,gmake,yes)
	if test x"$GMAKE_CHECK" != x"yes" ; then
	   AC_MSG_ERROR([Please install `gmake'.])
	fi
	AC_OUTPUT(src/FreeBSD/Makefile)
fi
if echo `uname` | grep -q "Linux"
then
	# Check if `make' exists
	AC_CHECK_PROG(MAKE_CHECK,make,yes)
	if test x"$MAKE_CHECK" != x"yes" ; then
	   AC_MSG_ERROR([Please install `make'.])
	fi
	
	# Check if `mandb' exists
	AC_CHECK_PROG(MANDB_CHECK,mandb,yes)
	if test x"$MANDB_CHECK" != x"yes" ; then
	   AC_MSG_ERROR([Please install `mandb'.])
	fi

	# Check if `make' exists
	AC_CHECK_PROG(MAKE_CHECK,make,yes)
	if test x"$MAKE_CHECK" != x"yes" ; then
	   AC_MSG_ERROR([Please install `make'.])
	fi
	AC_OUTPUT(src/Linux/Makefile)
fi
AC_OUTPUT(src/bricks/Makefile)
AC_OUTPUT(scripts/load-balance.lua)
AC_OUTPUT(scripts/duplicate.lua)
AC_OUTPUT(scripts/startup-one-thread.lua)
AC_OUTPUT(scripts/startup-multi-threads.lua)

AC_MSG_NOTICE([Type 'gmake' to compile and 'gmake install' to install the tool. Send feedback to ajamshed@icsi.berkeley.edu.])
