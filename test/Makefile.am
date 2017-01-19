AM_CFLAGS = @AM_CFLAGS@ -I$(srcdir)/../src -L$(srcdir)/../src
LDADD = ../src/libutf8validator.a

check_PROGRAMS = \
	test_validator

test_validator_SOURCES = test-validator.c
test_validator_LDADD = $(LDADD)

TESTS_ENVIRONMENT = \
	top_builddir=$(top_builddir); \
	# Enable malloc debugging where available
	export MallocScribble=1; \
	export MallocPreScribble=1; \
	export MallocGuardEdges=1;

TESTS = \
	test_validator
