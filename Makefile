.PHONY: build test clean pre-commit
build:
	python$(PY) setup.py build
test:
	python$(PY) setup.py test
clean:
	rm -rf build
	find . '(' -name '*.pyc' -o -name '*.so' ')' -delete
pre-commit:
	$(MAKE) clean && $(MAKE) PY=2 build && $(MAKE) PY=2 test
	$(MAKE) clean && $(MAKE) PY=3 build && $(MAKE) PY=3 test
