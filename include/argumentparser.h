#include <format>
#include <vector>
#include <string>
#include <string_view>

/* `CommandLineOptions` stores a set of program options with values determined from command-line
arguments passed to this program at launch. It handles verifying and parsing option names and
values, type-checking, and error handling. The values of the program options are stored in
the corresponding public fields of this class. */
class CommandLineOptions {

    /* Returns a `std::vector<std::string>` containing the command-line arguments in order,
    excluding the first argument (which is always the executable itself). The returned arguments
    are guaranteed to be encoded in UTF-8. */
    static auto get_command_line_arguments(int argc, char **argv) -> std::vector<std::string>;

    /* Attempts to assign the value given by `argument` (a `std::string_view`) to the option
    `option` of type `T`. */
    template <typename T>
    void try_assign(
        T &option,
        std::string_view argument,
        std::string_view option_name,
        auto &it
    );

    /* Attempts to set the value of `option` from the `curr_option_name` and `curr_option_value`
    command-line arguments passed in by the user. */
    template <typename T>
    auto try_set_option(
        T &option,
        std::string_view actual_option_name,
        std::string_view curr_option_name,
        std::string_view curr_option_value,
        auto &it,
        bool require_bool
    ) -> bool;

    /* Given the option name `option_name` and value `option_value` from the command-line arguments,
    `try_processing `attempts to set the value of the option corresponding to `option_name` to the
    value given by `option_value`. It returns `true` if success occurs, `false` if no option's name
    matched the `option_name` passed in, and does not return at all if an error is raised instead.
    */
    auto try_processing(
        std::string_view option_name,
        std::string_view option_value,
        auto &it,
        bool require_bool = false
    ) -> bool;

public:

    /* Each field corresponds to one option, and vice versa. */
    int nthreads = 0;
    int spp = 0;
    int seed = 0;
    std::string image_file = "image.ppm";
    std::string input_file = "scene.txt";
    bool quiet = false;
    bool log_util = false;
    bool partial = false;

    /* Constructs a `CommandLineOptions` using the `argc` command-line arguments stored in
    `argv`. `argc` and `argv` correspond to the argument given to the `main` function. */
    CommandLineOptions(int argc, char **argv);
};

/* Specialize `std::formatter` for `CommandLineOptions` */
template <>
struct std::formatter<CommandLineOptions> : public std::formatter<std::string> {
    auto format(const CommandLineOptions &item, std::format_context &format_context) const {
        return std::format_to(
            format_context.out(),
            "{{\n"
            "    nthreads: {},\n"
            "    spp: {},\n"
            "    seed: {},\n"
            "    image_file: {},\n"
            "    input_file: {},\n"
            "    quiet: {},\n"
            "    log_util: {},\n"
            "    partial: {}\n"
            "}}\n",
            item.nthreads, item.spp, item.seed, item.image_file, item.input_file, item.quiet,
            item.log_util, item.partial
        );
    }
};