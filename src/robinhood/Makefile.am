AM_CFLAGS= $(CC_OPT) $(DB_CFLAGS) $(PURPOSE_CFLAGS)
AM_LDFLAGS= -lpthread -rpath $(pkglibdir)

DATE=`date --utc --date @$${SOURCE_DATE_EPOCH:-$$(date +%s)} '+%F %T'`
MISC_FLAGS="-DCOMPIL_DATE=\"$(DATE)\""

noinst_LTLIBRARIES=librobinhood.la

rbh_libs=   ../cfg_parsing/librbhcfg.la         \
            ../fs_scan/libfsscan.la             \
            ../entry_processor/libentryproc.la  \
            ../policies/libpolicies.la

if CHANGELOGS
rbh_libs += ../chglog_reader/libchglog_rd.la
endif

rbh_libs += ../list_mgr/liblistmgr.la \
            ../common/libcommontools.la ../cfg_parsing/libconfigparsing.la

librobinhood_la_SOURCES=cmd_helpers.h cmd_helpers.c
librobinhood_la_LIBADD=$(rbh_libs)

sbin_PROGRAMS=robinhood rbh-report rbh-diff rbh-undelete rbh-rebind rbh-jreader
bin_PROGRAMS=rbh-find rbh-du

# dependencies:
librobinhood_la_DEPENDENCIES=$(rbh_libs)
robinhood_DEPENDENCIES=./librobinhood.la
rbh_report_DEPENDENCIES=./librobinhood.la
rbh_du_DEPENDENCIES=./librobinhood.la
rbh_find_DEPENDENCIES=./librobinhood.la
rbh_diff_DEPENDENCIES=./librobinhood.la
rbh_undelete_DEPENDENCIES=./librobinhood.la
rbh_rebind_DEPENDENCIES=./librobinhood.la

robinhood_SOURCES=rbh_daemon.c
robinhood_CFLAGS=$(AM_CFLAGS) $(FS_CFLAGS) $(MISC_FLAGS)
robinhood_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAGS) $(AM_LDFLAGS)

rbh_jreader_SOURCES=rbh_jreader.c pfid_check.c
rbh_jreader_CFLAGS=$(AM_CFLAGS) $(FS_FLAGS) $(MISC_FLAGS)
rbh_jreader_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAGS) $(AM_LDFLAGS) -ljansson

rbh_report_SOURCES=rbh_report.c
rbh_report_CFLAGS=$(AM_CFLAGS) $(FS_CFLAGS) $(MISC_FLAGS)
rbh_report_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAGS) $(AM_LDFLAGS)

rbh_find_SOURCES=rbh_find.c rbh_find_printf.c rbh_find.h
rbh_find_CFLAGS=$(AM_CFLAGS) $(FS_CFLAGS) $(MISC_FLAGS)
rbh_find_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAGS) $(AM_LDFLAGS)

rbh_du_SOURCES=rbh_du.c
rbh_du_CFLAGS=$(AM_CFLAGS) $(FS_CFLAGS) $(MISC_FLAGS)
rbh_du_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAGS) $(AM_LDFLAGS)

rbh_diff_SOURCES=rbh_diff.c
rbh_diff_CFLAGS=$(AM_CFLAGS) $(FS_CFLAGS) $(MISC_FLAGS)
rbh_diff_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAG) $(AM_LDFLAGS)

rbh_undelete_SOURCES=rbh_undelete.c
rbh_undelete_CFLAGS=$(AM_CFLAGS) $(FS_CFLAGS) $(MISC_FLAGS)
rbh_undelete_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAGS) $(AM_LDFLAGS)

rbh_rebind_SOURCES=rbh_rebind.c
rbh_rebind_CFLAGS=$(AM_CFLAGS) $(FS_CFLAGS) $(MISC_FLAGS)
rbh_rebind_LDFLAGS=-rdynamic ./librobinhood.la $(DB_LDFLAGS) $(FS_LDFLAGS) $(PURPOSE_LDFLAGS) $(AM_LDFLAGS)

new: clean all

indent:
	$(top_srcdir)/scripts/indent.sh
