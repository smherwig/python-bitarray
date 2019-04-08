build:
	python setup.py build
	
# installs under $HOME/.local/
# use `pip uinstall bitarray' to uninstall
install:
	pip install --user .

uninstall:
	pip uninstall bitarray

clean:
	rm -rf build
	
.PHONY: build install clean
