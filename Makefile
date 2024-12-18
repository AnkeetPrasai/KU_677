# Makefile to run dataflow.py on test1.ll, test2.ll, test3.ll, and test4.ll

# Variables
PYTHON = python3
SCRIPT = dataflow.py
TESTS = test1.ll test2.ll test3.ll test4.ll
OUTS = test1.out test2.out test3.out test4.out

# Phony targets
.PHONY: all clean test1 test2 test3 test4

# Default target to run all tests
all: $(OUTS)

# Individual test targets (optional)
test1: test1.out
test2: test2.out
test3: test3.out
test4: test4.out

# Clean target to remove generated .out files
clean:
    @echo "Cleaning up generated output files..."
    rm -f $(OUTS) && echo "Cleaned successfully."