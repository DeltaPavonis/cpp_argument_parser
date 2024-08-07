#!/usr/bin/env bash

# Global variables
CURRENT_TEST_NUMBER=0
TESTS_PASSED=0

run_test() {
    # Local variables
    local test_description=$1  # This means "set `test_description` equal to the first argument of `run_test()`"
    local command_line_arguments=$2  # The second argument ($2) gives the command-line arguments to call `cpp_argument_parser` with
    local expected_output_file="../tests/expected_output_${CURRENT_TEST_NUMBER}.txt"
    # The `mktemp` command creates an unique temporary file, which we'll write the output of calling
    # `cpp_argument_parser` with the current given command-line arguments to`
    local actual_output_file=$(mktemp)

    # Start the test
    echo "Running Test ${CURRENT_TEST_NUMBER} (${test_description})..."
    echo "Using command ./cpp_argument_parser ${command_line_arguments}"
    
    # Pass the command-line argments in `command_line_arguments` to `cpp_argument_parser`, and
    # write the resulting output to `actual_output_file`
    ./cpp_argument_parser ${command_line_arguments} > ${actual_output_file}

    # Use `diff` to compare the actual output to the expected output.
    if diff ${actual_output_file} ${expected_output_file}; then
        echo "Passed."
        ((TESTS_PASSED++))
    else
        echo "Failed."
    fi
    echo ""

    ((CURRENT_TEST_NUMBER++))
}

# Test general functionality
run_test "Test with no command-line arguments given (all options should have their default values)" ""
run_test "Test all --option[=value]" "--nthreads=4 --spp=100 --seed=1 --imagefile="imagefile.txt" --input="inputfile.txt" --quiet --logutil --partial"
run_test "Test all --option[ value]" "--nthreads 4 --spp 100 --seed 1 --imagefile "imagefile.txt" --input "inputfile.txt""
run_test "Test all -shortened_option[=value]" "-n=4 -s=1 -q -l -p"
run_test "Test boolean option chaining" "-qlp"
run_test "Mix" "--nthreads 4 -s=1 -qp -l=false --input "other_scene.txt""

# Test errors
run_test "Emits error on unknown option in --[option]=value" "--something=5"
run_test "Emits error on unknown option in --[option]" "--something"
run_test "Emits error on unknown option in -[option]" "-x"
run_test "Emits error on argument given without option" ""Hello!""
run_test "Emits error on single dash followed by multi-character argument (-[option]=[...])" "-pqs=5"
run_test "Emits error on single dash folloed by string containing non-boolean single-character argument" "-pqs"
run_test "Emits error on missing argument for non-boolean option" "--nthreads"
run_test "Does NOT emit error on missing argument for boolean option (instead, defaults to true)" "--quiet"
run_test "Emits error on invalid argument to int option" "-n="Hello""
run_test "Emits error on integer overflow to int option" "-n=2147483648"
run_test "Does NOT emit error on non-overflowing integer argument to int option" "-n=2147483647"
run_test "Emits error on missing argument for non-boolean option with equals sign present" "--spp="

# Report statistics (numbr of tests pasased, whether all tests passed or not)
# The $((CURRENT_TEST_NUMBER - 0)) in place of ${CURRENT_TEST_NUMBER} is for educational
# purposes; in bash, double parentheses are used to perform arithmetic (see above, where
# we did `((CURRENT_TEST_NUMBER++))`), and the $, in this case, is used to access the resulting
# value of the mathematical computation
echo "${TESTS_PASSED} / $((CURRENT_TEST_NUMBER - 0)) tests passed"

if [ ${TESTS_PASSED} -eq $((CURRENT_TEST_NUMBER - 0)) ]; then
    echo "ALL TESTS PASSED"
else
    echo "SOME TESTS FAILED"
fi