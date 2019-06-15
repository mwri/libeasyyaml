SUBDIRS = src test examples

ACLOCAL_AMFLAGS = -I m4

test: check

clean-cov-data:
	find . -name '*.gcda' -exec rm {} \;

cov-report:
	mkdir -p coverage
	lcov --base-directory src --directory . -c -o coverage/lcov.info
	gcovr --html --html-details -o coverage/index.html -e test/

.PHONY: test clean-cov-data cov-report
