#include "huffman.h"
#include <iostream>
#include <cstring>

class argument_parser {
public:
    using iter_t = const char **;

    argument_parser(iter_t begin, iter_t end, huffman_runner &runner) : it{begin}, end{end}, runner{runner} {}

    void parse() {
        while (it != end) {
            parse_next();
        }
    }

private:
    void parse_next() {
        if (!strcmp(*it, "-c")) {
            runner.set_action(huffman_runner::action::ENCODE);
        } else if (!strcmp(*it, "-u")) {
            runner.set_action(huffman_runner::action::DECODE);
        } else if (!strcmp(*it, "-f") || !strcmp(*it, "--file")) {
            ++it;
            runner.set_input_file(get_filename());
        } else if (!strcmp(*it, "-o") || !strcmp(*it, "--output")) {
            ++it;
            runner.set_output_file(get_filename());
        } else {
            throw huffman_exception(huffman_exception::UNKNOWN_OPTION_FORMAT, *it);
        }
        ++it;
    }

    std::string get_filename() {
        if (it == end) {
            throw huffman_exception(huffman_exception::PATH_EXPECTED_FORMAT, *std::prev(it));
        }
        return *it;
    }

    iter_t it, end;
    huffman_runner &runner;

};

int main(int argc, const char **argv) {
    try {
        huffman_runner runner(std::cout);
        argument_parser parser{argv + 1, argv + argc, runner};
        parser.parse();
        runner.execute();
    } catch (const std::exception &ex) {
        std::cout << ex.what() << std::endl;
    }
}