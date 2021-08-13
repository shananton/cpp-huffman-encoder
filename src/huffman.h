#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <string>
#include <vector>
#include <queue>
#include <array>
#include <algorithm>
#include <tuple>
#include <stdexcept>
#include <memory>

using byte = unsigned char;
static constexpr int BITS_IN_BYTE = 8;

class huffman_exception : public std::logic_error {
public:
    explicit huffman_exception(const std::string &message) : std::logic_error{message} {}

    huffman_exception(const char *fmt, const char *str) : std::logic_error{format(fmt, str)} {}

    static std::string format(const char *fmt, const char *str);

    static constexpr auto NO_INPUT =
            "No input file specified. Use -f <path> or --file <path> to set.";
    static constexpr auto NO_OUTPUT =
            "No output file specified. Use -o <path> or --output <path> to set.";
    static constexpr auto NO_ACTION =
            "No action specified. Use -c to compress or -u to uncompress.";
    static constexpr auto MULTIPLE_ACTIONS =
            "Multiple actions specified. Only one of -c or -u should be used.";
    static constexpr auto MULTIPLE_INPUTS =
            "Multiple input files specified. Only one of -f <path> or --file <path> should be used.";
    static constexpr auto MULTIPLE_OUTPUTS =
            "Multiple output files specified. Only one of -o <path> or --output <path> should be used.";
    static constexpr auto INPUT_ERROR_FORMAT =
            "Error opening input file '%s'. Check that the path is valid and the file exists.";
    static constexpr auto OUTPUT_ERROR_FORMAT =
            "Error creating output file '%s'. Check that the path is valid.";
    static constexpr auto PATH_EXPECTED_FORMAT =
            "<path> expected after '%s', got nothing.";
    static constexpr auto UNKNOWN_OPTION_FORMAT =
            "Unknown option '%s'. Valid options are:\n-f --file\n-o --output\n-c\n-u";
};

class huffman_tree_base {
protected:
    struct node {
        node(byte ch) : go{nullptr, nullptr}, ch{ch} {}

        node(std::unique_ptr<node> left, std::unique_ptr<node> right)
                : go{std::move(left), std::move(right)}, ch{0} {}

        std::array<std::unique_ptr<node>, 2> go;
        byte ch;

        bool is_leaf() const { return go[0] == nullptr; }
    };

    std::unique_ptr<node> tree;
};

class huffman_encoding_tree : private huffman_tree_base {
public:
    using byte_const_iter_t = std::vector<byte>::const_iterator;

    struct subtree_ref {
        explicit subtree_ref(std::unique_ptr<node> subtree, int freq)
                : subtree{std::move(subtree)}, freq{freq} {}

        // Workaround to move subtree from priority_queue
        mutable std::unique_ptr<node> subtree;
        int freq;

        bool operator<(const subtree_ref &other) const {
            return freq > other.freq;
        }
    };

    explicit huffman_encoding_tree(byte_const_iter_t begin, byte_const_iter_t end);

    std::vector<bool> get_tree_info() const {
        std::vector<bool> res;
        tree_info_dfs(*tree, res);
        return res;
    }

    static constexpr std::size_t TABLE_SIZE = 1 << BITS_IN_BYTE;

    const std::vector<bool> &get_code(byte ch) const { return codes[ch]; }

private:
    void build_codes() {
        std::vector<bool> code;
        build_codes_dfs(*tree, code);
    }

    void build_codes_dfs(const node &u, std::vector<bool> &cur_code);
    void tree_info_dfs(const node &u, std::vector<bool> &info) const;

    std::array<std::vector<bool>, TABLE_SIZE> codes{};
};

class huffman_decoding_tree : private huffman_tree_base {
public:
    using bit_const_iter_t = std::vector<bool>::const_iterator;

    explicit huffman_decoding_tree(bit_const_iter_t begin, bit_const_iter_t end);

    bool eof() const;
    bit_const_iter_t get_it() const;

    byte decode_char() { return decode_char_dfs(*tree); }

private:
    byte decode_char_dfs(const node &subtree);
    std::unique_ptr<node> tree_build_dfs();

    bit_const_iter_t it, end;
};

class huffman_runner {
public:
    explicit huffman_runner(std::ostream &log) : log{log} {}

    enum class action {
        NOT_SET, ENCODE, DECODE
    };

    void set_action(action action);
    void set_input_file(std::string filename);
    void set_output_file(std::string filename);

    void execute();

private:
    struct result {
        std::vector<byte> data;
        std::size_t initial_size;
        std::size_t processed_size;
        std::size_t aux_size;
    };

    result encode(const std::vector<byte> &data);
    result decode(const std::vector<byte> &data);
    void log_uint(std::size_t val);
    std::vector<byte> read_data();
    void write_data(const std::vector<byte> &data);

    std::ostream &log;
    action op = action::NOT_SET;
    std::string input, output;
};

#endif //HUFFMAN_H
