# Define the directories to check
SRC_DIRS := .

.PHONY: all mypy flake8

all: mypy flake8

mypy:
	-python -m mypy $(SRC_DIRS)

flake8:
	-python -m flake8 $(SRC_DIRS)
