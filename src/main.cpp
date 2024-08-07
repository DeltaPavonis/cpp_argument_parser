#include "argumentparser.h"
#include <iostream>

int main(int argc, char** argv)
{
    /* Read and print out command-line options */
    CommandLineOptions options(argc, argv);
    std::cout << std::format("Parsed options: {}", options);

    return 0;
}