#include "argumentparser.h"
#include <iostream>

using namespace std::literals;

/* Formats the arguments `args...` to `stdout` using `std::format` and `std::cout`, then
calls `std::exit(-1)`. */
template <typename... Args>
[[noreturn]] auto print_then_exit(std::format_string<Args...> format_str, Args&&... args) {
    std::cout << std::format(format_str, std::forward<Args>(args)...) << std::endl;
    std::exit(-1);
}

/* Attempts to assign the value given by `argument` (a `std::string_view`) to the option `option`
of type `T`. That is, this function effectively tries to perform a `std::string`-to-`T` conversion
on `argument`; if that succeeds, then the resulting value is assigned to `option`. The name of the
option `option_name` is passed in for use in error messages, and the current argument iterator `it`
is passed in so that we can handle cases where two arguments are consumed to initialize an option,
rather than one (cases such as `--nthreads 5` vs `--nthreads=5`; the first uses up two command-line
arguments to initialize the `nthreads` option, while the second uses up just one argument). */
template <typename T>
void CommandLineOptions::try_assign(
    T &option,
    std::string_view argument,
    std::string_view option_name,
    auto &it
) {
    /* Case on the type of `T` */
    if constexpr (std::is_same_v<T, std::string>) {
        /* If the option type is `std::string`, we just need to assign it to a `std::string`
        constructed from the `std::string_view` `argument`. */
        option = std::string(argument);
    } else if constexpr (std::is_same_v<T, char>) {
        /* If the option type is `char`, then all we need to do is verify that `argument`
        has length equal to 1. If it does, we assign the sole character of `argument` to
        `option`. */
        if (argument.size() != 1) {
            print_then_exit(
                "Error: Unexpected argument {} for char option {}",
                argument, option_name
            );
        }
        option = argument.front();
    } else if constexpr (std::is_same_v<T, bool>) {
        /* If the option type is `bool`, then we need to do some special handling with `it`,
        because boolean options do not need to specify a value (if they do not, they are
        implicitly set to true, as in `cmd --quiet`, for example). The handling is explained
        below. */

        if (argument.empty() || argument == "1" || argument == "true") {
            /* If the argument provided to this option was "1" or "true", or if we passed in an
            empty string (meaning there either was no argument given to this option, or this
            option was part of a clustered boolean option string), then no special handling */
            option = true;
        } else if (argument == "0" || argument == "false") {
            /* We set `option` to `false` if the provided `argument` was "0" or "false". */
            option = false;
        } else if (argument.front() != '-') {
            /* If the current boolean option was followed by another command-line argument and
            that argument was none of "1", "true", "0", or "false", then that next command-line
            argument must be the start of another option (because besides "1", "true", "0", or
            "false", we disallow any other values from being provided to a boolean option).
            An argument is an option iff it begins with a dash; if it doesn't, then we have
            an unexpected argument to the current boolean option. */
            print_then_exit(
                "Error: Unexpected argument {} for boolean option {}",
                argument, option_name
            );
        } else {
            /* If the next command-line argument is an option (e.g. `cmd --quiet --nthreads=...),
            that means the current boolean option had no argument given to it. In this case, it is
            implicitly set to true. */
            option = true;

            /* Additionally, we will decrement the argument pointer `it`. This is because in this
            case, we have still only consumed one argument to initialize `option`, because no
            value was provided to initialize this boolean option. However, the condition used
            later to determine if two arguments were used to initialize the option is simply
            if there was no `=` in the first argument, and if there was a command-line argument
            coming after that first argument. Thus, that condition will mistakenly advance `it`
            one more than it should; thus, we decrement it once to negate that effect. */
            --it;
        }
    } else if constexpr (std::is_same_v<T, int>) {
        /* If the option type is `int`, then we try converting `argument` to an `int`; if that
        succeeds (if `argument` represents a number and that number does not overflow the `int`
        type), then the resulting value is assigned to `option`. */
        option = 0;
        for (char c : argument) {

            /* If there is a non-digit character in `argument`, then the argument is invalid
            for an option of type `int` */
            if (!(c >= '0' && c <= '9')) {
                print_then_exit(
                    "Error: Expected integer argument for int option {}, got {}",
                    option_name, argument
                );
            }

            /* Check for integer overflow */
            if ((std::numeric_limits<T>::max() - (c - '0')) / 10 < option) {
                print_then_exit(
                    "Error: Argument {} overflows for int option {}",
                    argument, option_name
                );
            }

            option = 10 * option + (c - '0');
        }
    } else {
        static_assert(
            false,
            "Option type not supported; you need to add code to CommandLineOptions::try_assign()."
        );
    }
}

/* Attempts to set the value of `option` from the `curr_option_name` and `curr_option_value`
command-line arguments passed in by the user. The actual name of the option to test is given
in `actual_option_name` (which is used to match with `curr_option_name`). Additionally, the
iterator to the current argument `it` is passed in for use in error messages and for some
special handling logic within the boolean option case in `try_assign`, and `bool_cluster`
(whether or not the current option is being set as part of a cluster of single-character
boolean options in a command-line argument) is used to provide more specific error messages
to the user. */
template <typename T>
auto CommandLineOptions::try_set_option(
    T &option,
    std::string_view actual_option_name,
    std::string_view curr_option_name,
    std::string_view curr_option_value,
    auto &it,
    bool bool_cluster
) -> bool {

    /* If the option name passed in as a command-line argument does not match the actual
    option name of this `option`, then we obviously cannot set the value of `option` with
    the given command-line arguments. Thus, we instantly return `false` in this case. */
    if (actual_option_name != curr_option_name) {
        return false;
    }

    /* If `bool_cluster` is true, then require that the type `T` of the actual `option`
    be `bool`. */
    if (bool_cluster && !std::is_same_v<T, bool>) {
        print_then_exit(
            "Error: Non-boolean argument {} in {}\nHelp: Single dashes are used "
            "for either one single-character option (e.g. cmd -n 5),\nor for multiple "
            "single-character boolean options. Try separating non-boolean options out.",
            curr_option_name,
            *it
        );
    }

    /* If `option` is not a boolean option, then it must have been given a value. If
    no value was given (i.e. if `curr_option_value` was passed in as the empty string),
    then raise an error. */
    if (!std::is_same_v<T, bool> && curr_option_value.empty()) {
        print_then_exit("Error: Missing value for option {}", curr_option_name);
    }

    /* Otherwise, try to set the value of `option` from the sequence of characters given in
    `curr_option_name`. */
    try_assign(option, curr_option_value, curr_option_name, it);

    /* If the above function returns without terminating the program, then assignment succeeded,
    and so we return `true`. Success! */
    return true;
}

/* Given the option name `option_name` and value `option_value` from the command-line arguments,
`try_processing `attempts to set the value of the option corresponding to `option_name` to the
value given by `option_value`. It returns `true` if success occurs, `false` if no option's name
matched the `option_name` passed in, and does not return at all if an error is raised instead.
The iterator to the current argument `it` is passed in for use in error messages, and in some
special handling logic within the boolean option case in  `try_assign`. `bool_cluster` (defaulted
to `false`; see declaration) should be set to `true` if the current option is part of a cluster
of single-character boolean options; it is used to provide more specific error messages in
`try_set_option`. */
auto CommandLineOptions::try_processing(
    std::string_view option_name,
    std::string_view option_value,
    auto &it,
    bool bool_cluster
) -> bool {
    /* For every possible option name, try to set the corresponding option to the value given by
    `option_value`. If a new option or option name is added, one single line needs to be added
    to this function. */
    return (try_set_option(nthreads, "nthreads", option_name, option_value, it, bool_cluster) ||
            try_set_option(nthreads, "n", option_name, option_value, it, bool_cluster) ||
            try_set_option(spp, "spp", option_name, option_value, it, bool_cluster) ||
            try_set_option(seed, "seed", option_name, option_value, it, bool_cluster) ||
            try_set_option(seed, "s", option_name, option_value, it, bool_cluster) ||
            try_set_option(image_file, "imagefile", option_name, option_value, it, bool_cluster) ||
            try_set_option(input_file, "input", option_name, option_value, it, bool_cluster) ||
            try_set_option(quiet, "quiet", option_name, option_value, it, bool_cluster) ||
            try_set_option(quiet, "q", option_name, option_value, it, bool_cluster) ||
            try_set_option(log_util, "logutil", option_name, option_value, it, bool_cluster) ||
            try_set_option(log_util, "l", option_name, option_value, it, bool_cluster) ||
            try_set_option(partial, "partial", option_name, option_value, it, bool_cluster) ||
            try_set_option(partial, "p", option_name, option_value, it, bool_cluster));
}

CommandLineOptions::CommandLineOptions(int argc, char **argv) {

    /* First, get the command-line arguments (excluding the first one, which is always the
    executable itself) as a `std::vector<std::string>`, and store it in `arguments`. */
    auto arguments = get_command_line_arguments(argc, argv);

    /* Iterate over every non-executable command-line argument. */
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {

        /* We define `curr_argument` as a `std::string_view` over the current argument `*it`. */
        auto curr_argument = std::string_view(*it);

        /* Find the number of dashes at the beginning of the current argument, and remove all
        such prefix dashes from `curr_argument`. */
        auto num_prefix_dashes = curr_argument.find_first_not_of('-');
        curr_argument.remove_prefix(num_prefix_dashes);

        /* We have several cases for `curr_argument`:
        Case 1: `curr_argument` might not be an option at all; this occurs if it is prefixed by
        zero dashes. In this case, we immediately raise an error, because we always will expect
        `it` to point to an option at the start of every iteration in this `for`-loop.

        Case 2: `curr_argument` was prefixed with exactly one dash, there were multiple characters
        following that dash, and there either was no equal sign present, or the first '=' occurred
        more than one character after the dashes.
        An example of the first case is `-abcd`, and an example of the second case is `-abcd=[...]`.
        Clearly, the second possibility is invalid, because single-dashes are used exclusively for
        single-character options (e.g. `-a`), or a cluster of single-character boolean options
        (e.g. `-abc`, where `a`, `b`, and `c` are all boolean options). The first case here
        corresponds exactly to the case of a boolean option cluster; thus, we will need to set the
        values of all the single-character options in the argument to `true` in that case.

        Case 3: `curr_argument` was either prefixed with two dashes, or it was prefixed with one
        dash and then followed by a single character and then possibly an equals sign.
        This case is designed to capture all arguments that could represent a valid option-value
        pair. Specifically, this case handles arguments of the form `--option=[value]`,
        `-o=[value]`, `--option`, and `-o`. In the cases of `--option` and `-o`, we will expect to
        find a value as the next argument, unless `option` is a boolean option, in which case a
        value is optional (if no value is given, the boolean option will be set to true). */
        if (auto equals_sign_index = curr_argument.find('='); num_prefix_dashes == 0) {
            /* If the current argument was prefixed by zero dashes, then it is not a valid option
            at all, and so we raise an error. */
            print_then_exit("Error: Expected -[option] or --[option], got {}", *it);
        } else if (num_prefix_dashes == 1 && curr_argument.size() > 1 && equals_sign_index > 1) {
            /* Handle Case 2 (clusters of single-character boolean options). Note that the condition
            `equals_sign_index > 1` implicitly includes `equals_sign_index == std::string::npos`,
            because `std::string::npos` is defined as being the largest possible `size_t` value.
            That is, `equals_sign_index > 1` will capture both the case when the first `=` occurs
            more than one character after the prefix dashes, and the case where there is no `=`
            in the current argument at all. */

            /* If there is an equals sign in the string (e.g. `-abcd=[...]`), we know there is
            an error, because single dashes are used exclusively for single-character options
            (and `abcd` contains multiple characters), or for clusters of single-character
            boolean options (in which case no value should be given; the argument should just
            be `-abcd`). Thus, we raise an error in this case. */
            if (equals_sign_index != std::string::npos) {
                print_then_exit(
                    "Error: Unrecognized option {} in -{}\nHelp: Single dashes are used "
                    "for either one single-character option (e.g. cmd -n 5),\nor for multiple "
                    "single-character boolean options. Did you mean to use two dashes\ninstead "
                    "of one?",
                    curr_argument.substr(0, equals_sign_index),
                    curr_argument
                );
            }

            /* Otherwise, we have a cluster of single-character boolean options, such as `-abcd`.
            These are equivalent to setting every individual single-character boolean option to
            true. So, we loop through all characters of `curr_argument`, and call
            `try_processing` on each one. */
            for (char option_name : curr_argument) {
                /* `std::string_view(&option_name, 1)` looks odd, but it is a way to create
                a `std::string_view` over a single character. Additionally, note that (a)
                we pass in an empty string to the `option_value` parameter of `try_processing`,
                denoting that the user did not explicitly provide a value for the current
                option, and that (b) we set the `bool_cluster` parameter to true. This turns
                on a requirement that the type of the option be boolean; if not, a detailed
                error message will be raised. */
                if (!try_processing(std::string_view(&option_name, 1), "", it, true)) {
                    print_then_exit(
                        "Error: Unrecognized option {} in -{}",
                        option_name, curr_argument
                    );
                }
            }
        } else {

            /* Extract the name of the current argument's option, and the value we should set that
            option to. */
            std::string_view option_name, option_value;
            if (equals_sign_index != std::string::npos) {
                /* If the current argument contains a `=`, then we are looking for the cases
                of an option followed by an equal sign and then followed by its value, all
                in the same string (e.g. `--nthreads=4` or `-n=4`). In this case, after
                the prefix dashes have been stripped out, the option name and value from the
                current argument are simply the substrings before and after the `=` sign. */
                option_name = curr_argument.substr(0, equals_sign_index);
                option_value = curr_argument.substr(equals_sign_index + 1);
            } else {
                /* If the current argument contains no equals sign, then we either have a
                boolean option with no value given (in which case the option is automatically
                set to `true`), or an option whose value is given as the next argument (e.g.
                `--nthreads 4`, `-n 4`, `--quiet 1`, etc). In this case, after removing prefix
                dashes from the current argument, the option name is simply given by the current
                argument, while the option value is given by the next argument. If there is no
                next argument, we set `option_value` to the empty string; this is checked in
                the functions called from `try_processing`. */
                option_name = curr_argument;
                option_value = (std::next(it) != arguments.end() ? *std::next(it) : ""sv);
            }


            if (!try_processing(option_name, option_value, it)) {
                print_then_exit("Error: Unrecognized option {}", option_name);
            }

            /* Increment it again if we used two arguments just now */
            /* If the option name and value were given as two separate arguments, then we
            actually consumed two command-line arguments to initialize the current option, and
            so we need to increment `it` an extra time. This occurs when the current argument
            contained no equals sign, and when there was a next command-line argument (unless
            the option was a boolean option and the next command-line argument was the next
            option, which is a case we handle in the `try_assign` function; see above). */
            if (equals_sign_index == std::string::npos && !option_value.empty()) {
                ++it;
            }
        }
    }
}