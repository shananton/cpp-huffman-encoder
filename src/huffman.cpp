#include <utility>

#include "huffman.h"

#include <cassert>
#include <fstream>
#include <ios>
#include <string>
#include <iostream>

byte read_byte(huffman_decoding_tree::bit_const_iter_t &it) {
    byte res = 0;
    for (auto pos = 0; pos < BITS_IN_BYTE; ++pos, ++it) {
        res |= (*it << pos);
    }
    return res;
}

std::string huffman_exception::format(const char *fmt, const char *str) {
    constexpr auto BUF_SIZE = 1024;
    char buf[BUF_SIZE];
    std::snprintf(buf, BUF_SIZE, fmt, str);
    return buf;
}

std::vector<byte> bits_to_bytes(const std::vector<bool> &bits) {
    assert(bits.size() % BITS_IN_BYTE == 0);
    auto byte_count = bits.size() / BITS_IN_BYTE;
    std::vector<byte> res(byte_count);
    auto bit_it = std::begin(bits);
    for (auto &b : res) {
        b = read_byte(bit_it);
    }
    return res;
}

std::vector<bool> bytes_to_bits(const std::vector<byte> &bytes) {
    std::vector<bool> res;
    res.reserve(BITS_IN_BYTE * bytes.size());
    for (auto b : bytes) {
        for (auto pos = 0; pos < BITS_IN_BYTE; ++pos) {
            res.push_back((b >> pos) & 1u);
        }
    }
    return res;
}

std::vector<byte> bits_to_bytes_with_padding(std::vector<bool> bits) {
    byte padding = BITS_IN_BYTE - bits.size() % BITS_IN_BYTE;
    bits.insert(bits.begin(), padding, false);
    auto res = bits_to_bytes(bits);
    res.insert(res.begin(), padding);
    return res;
}

std::vector<bool> bytes_with_padding_to_bits(std::vector<byte> bytes) {
    byte padding = bytes.front();
    bytes.erase(bytes.begin());
    auto res = bytes_to_bits(bytes);
    res.erase(res.begin(), res.begin() + padding);
    return res;
}

huffman_encoding_tree::huffman_encoding_tree(
        byte_const_iter_t begin, byte_const_iter_t end) {
    std::array<int, TABLE_SIZE> freq{};
    for (auto it = begin; it != end; ++it) {
        freq[*it]++;
    }

    auto char_cnt = TABLE_SIZE - std::count(std::begin(freq), std::end(freq), 0);
    // Handle special cases: all of the characters are the same or the file is empty
    if (char_cnt == 0) {
        freq[0]++;
    }
    if (char_cnt <= 1) {
        byte ch = std::find_if(std::begin(freq), std::end(freq), [](auto x) {
            return x != 0;
        }) - std::begin(freq);
        freq[static_cast<byte>(ch + 1)]++;
    }

    std::priority_queue<subtree_ref> heap;
    for (std::size_t ch = 0; ch < TABLE_SIZE; ++ch) {
        if (freq[ch]) {
            heap.emplace(std::make_unique<node>(ch), freq[ch]);
        }
    }

    // Huffman algorithm
    auto get_from_heap = [&heap]() {
        auto ptr = std::move(heap.top().subtree);
        auto freq = heap.top().freq;
        heap.pop();
        return std::make_pair(std::move(ptr), freq);
    };

    while (heap.size() > 1) {
        auto[ptr_left, freq_left] = get_from_heap();
        auto[ptr_right, freq_right] = get_from_heap();
        heap.emplace(std::make_unique<node>(
                std::move(ptr_left), std::move(ptr_right)), freq_left + freq_right);
    }

    tree = get_from_heap().first;
    build_codes();
}

void huffman_encoding_tree::tree_info_dfs(const node &u, std::vector<bool> &info) const {
    info.push_back(u.is_leaf());
    if (u.is_leaf()) {
        byte ch = u.ch;
        for (int i = 0; i < BITS_IN_BYTE; ++i) {
            info.push_back(ch & 1u);
            ch >>= 1u;
        }
    } else {
        for (int next_bit = 0; next_bit <= 1; ++next_bit) {
            tree_info_dfs(*u.go[next_bit], info);
        }
    }
}

void huffman_encoding_tree::build_codes_dfs(const huffman_tree_base::node &u, std::vector<bool> &cur_code) {
    if (u.is_leaf()) {
        codes[u.ch] = cur_code;
    } else {
        for (int next_bit = 0; next_bit <= 1; ++next_bit) {
            cur_code.push_back(next_bit);
            build_codes_dfs(*u.go[next_bit], cur_code);
            cur_code.pop_back();
        }
    }
}

huffman_decoding_tree::huffman_decoding_tree(
        bit_const_iter_t begin, bit_const_iter_t end) : it{begin}, end{end} {
    tree = tree_build_dfs();
}

std::unique_ptr<huffman_tree_base::node> huffman_decoding_tree::tree_build_dfs() {
    if (*it++) {
        return std::make_unique<node>(read_byte(it));
    }
    auto left = tree_build_dfs();
    auto right = tree_build_dfs();
    return std::make_unique<node>(std::move(left), std::move(right));

}

bool huffman_decoding_tree::eof() const {
    return it == end;
}

huffman_decoding_tree::bit_const_iter_t huffman_decoding_tree::get_it() const {
    return it;
}

byte huffman_decoding_tree::decode_char_dfs(const huffman_tree_base::node &subtree) {
    if (subtree.is_leaf()) {
        return subtree.ch;
    }
    return decode_char_dfs(*subtree.go[*it++]);
}

std::vector<byte> huffman_runner::read_data() {
    std::vector<byte> res;
    std::ifstream is(input, std::ios_base::in | std::ios_base::binary);
    if (!is.good()) {
        throw huffman_exception(huffman_exception::INPUT_ERROR_FORMAT, input.c_str());
    }
    while (!is.eof()) {
        res.emplace_back();
        is >> std::noskipws >> res.back();
    }
    res.pop_back(); // gets rid of an extra '\0' character
    return res;
}

void huffman_runner::write_data(const std::vector<byte> &data) {
    std::ofstream os(output, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    if (!os.good()) {
        throw huffman_exception(huffman_exception::OUTPUT_ERROR_FORMAT, output.c_str());
    }
    for (const auto &b : data) {
        os << b;
    }
}

void huffman_runner::set_action(huffman_runner::action action) {
    if (op != action::NOT_SET) {
        throw huffman_exception(huffman_exception::MULTIPLE_ACTIONS);
    }
    op = action;
}

void huffman_runner::set_input_file(std::string filename) {
    if (!input.empty()) {
        throw huffman_exception(huffman_exception::MULTIPLE_INPUTS);
    }
    input = std::move(filename);
}

void huffman_runner::set_output_file(std::string filename) {
    if (!output.empty()) {
        throw huffman_exception(huffman_exception::MULTIPLE_OUTPUTS);
    }
    output = std::move(filename);
}

void huffman_runner::execute() {
    if (input.empty()) {
        throw huffman_exception(huffman_exception::NO_INPUT);
    }
    if (output.empty()) {
        throw huffman_exception(huffman_exception::NO_OUTPUT);
    }
    if (op == action::NOT_SET) {
        throw huffman_exception(huffman_exception::NO_ACTION);
    }
    auto data = read_data();
    result res;
    switch (op) {
        case action::ENCODE:
            res = encode(data);
            break;
        case action::DECODE:
            res = decode(data);
            break;
        default:
            assert(false);
    }
    write_data(res.data);
    log_uint(res.initial_size);
    log_uint(res.processed_size);
    log_uint(res.aux_size);
}

huffman_runner::result huffman_runner::encode(const std::vector<byte> &data) {
    huffman_encoding_tree tree(std::begin(data), std::end(data));
    std::vector<bool> encoded_chars;
    for (const auto &b : data) {
        const auto &code = tree.get_code(b);
        encoded_chars.insert(encoded_chars.end(), std::begin(code), std::end(code));
    }
    std::vector<bool> res = tree.get_tree_info();
    res.insert(res.end(), std::begin(encoded_chars), std::end(encoded_chars));
    auto data_to_write = bits_to_bytes_with_padding(res);

    auto encoded_text_size = (encoded_chars.size() + BITS_IN_BYTE - 1) / BITS_IN_BYTE;
    auto aux_size = data_to_write.size() - encoded_text_size;
    return {data_to_write, data.size(), encoded_text_size, aux_size};
}

huffman_runner::result huffman_runner::decode(const std::vector<byte> &data) {
    auto bits = bytes_with_padding_to_bits(data);
    auto tree = huffman_decoding_tree(std::begin(bits), std::end(bits));

    std::size_t encoded_text_size = (std::end(bits) - tree.get_it() + BITS_IN_BYTE - 1) / BITS_IN_BYTE;

    std::vector<byte> res;
    while (!tree.eof()) {
        res.push_back(tree.decode_char());
    }

    auto aux_size = data.size() - encoded_text_size;
    return {res, encoded_text_size, res.size(), aux_size};
}

void huffman_runner::log_uint(std::size_t val) {
    log << val << std::endl;
}
