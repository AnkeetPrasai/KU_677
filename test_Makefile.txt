# Makefile to run dataflow.py on test1.ll through test8.ll

# Variables
PYTHON = python3
SCRIPT = dataflow.py
TESTS = test1.ll test2.ll test3.ll test4.ll test5.ll test6.ll test7.ll test8.ll
OUTS = $(addprefix outputs/, $(TESTS:.ll=.out))

# Phony targets
.PHONY: all clean test1 test2 test3 test4 test5 test6 test7 test8

# Ensure the outputs directory exists
outputs/:
	@echo "Creating outputs directory..."
	mkdir -p outputs/

# Pattern rule to generate .out files from .ll files
outputs/%.out: %.ll | outputs/
	@echo "Running $(SCRIPT) on $<..."
	$(PYTHON) $(SCRIPT) $< > $@ && echo "Output saved to $@"

# Default target to run all tests
all: outputs/ $(OUTS)

# Individual test targets (optional)
test1: outputs/test1.out
test2: outputs/test2.out
test3: outputs/test3.out
test4: outputs/test4.out
test5: outputs/test5.out
test6: outputs/test6.out
test7: outputs/test7.out
test8: outputs/test8.out

# Clean target to remove generated .out files
clean:
	@echo "Cleaning up generated output files..."
	rm -f outputs/*.out && echo "Cleaned successfully."