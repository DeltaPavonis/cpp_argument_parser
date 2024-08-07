# cpp_argument_parser

A basic command-line argument parser written in C++20.
- **Cross-platform**; tested on Windows, Mac, and Linux.
- Supports single and double-dashed arguments with arbitrary names and types
    - Supports optional `=` signs (so `cmd --max-depth=5` and `cmd --max-depth 5` both work)
- **Supports option bundling**: combining multiple single-character boolean options with a single dash (e.g. `cmd -abcd` rather than `cmd -a -b -c -d`)


## General Usage
Say you wanted a command to support the following options:
- `max_threads` (aka `m`): The maximum number of threads to use for the hypothetical command
- `quiet` (aka `q`): Whether or not to suppress compiler output
- `outfile` (aka `o`): The file path of the output file
- `log-errors` (aka `l`): Whether to write error messages to the output file
- `fail-on-warning`: Whether or not to abort if the system generates a warning

To have your command support these options, you would simply 

1. Add the fields to the `CommandLineOptions` class (you would not need to change anything else), so it would look like
```cpp
class CommandLineOptions {
    ...
public:
    /* List options here as fields */
    int max_threads = 0;
    bool quiet = false;
    std::string outfile = "output.txt";
    bool log_errors = false;
    bool fail_on_warning = false;

    ...
};
```
2. Then, for each option name, add one line to `CommandLineOptions::try_processing()`.
3. Finally add the fields corresponding to your options to `std::formatter<CommandLineOptions>::format()`.

Afterwards, you would be able to execute your program, passing your options to the executable. For example, `cpp_argument_parser` would correctly handle all of the following:
- `./cmd --max_threads 5 -ql -outfile=log.txt`
- `./cmd -m=5 -l=false --fail-on-warning`
- `./cmd -q=1 --max_threads=20 -fl -outfile log.txt`

The command-line arguments are directly accessible from the fields of `CommandLineOptions`, which are set in its constructor.


## How to Build
1. `git clone` this repo.
2. After `git clone`ing, `cd` to the top directory of `cpp_argument_parser`, and create a build directory (e.g. `mkdir build`).
3. `cd` into the build directory and run `cmake ..`.
4. Build the project (run `make` if the previous step generated Makefiles).

## How to Run Tests
**From the build directory**, give executable permissions to the test script first using `chmod +x ./../scripts/run_tests.sh` (if on Linux). Then, use the  command `./../scripts/run_tests.sh`.


