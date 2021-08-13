#include "gtest/gtest.h"

// A hack that allows testing of private fields/methods without coupling
// the test code with the production code.
#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wkeyword-macro"
#define private public
#define protected public
#pragma GCC diagnostic pop

#include "huffman_test.h"
#include "huffman.h"
#include "huffman.cpp"

#define ALL(x) std::begin(x), std::end(x)

TEST(utils_test, read_byte_works) {
    std::vector<bool> bits{0, 1, 1, 0, 1, 1, 1, 0};
    auto it = std::cbegin(bits);
    EXPECT_EQ(read_byte(it), 0b01110110u);
}

TEST(utils_test, bits_to_bytes_works) {
    EXPECT_EQ(bits_to_bytes({0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1}),
              std::vector<byte>({0b11101100u, 0b11100000u}));
}

TEST(utils_test, bytes_to_bits_works) {
    EXPECT_EQ(bytes_to_bits({0b11101100u, 0b11100000u}),
              std::vector<bool>({0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1}));
}


TEST(utils_test, bits_to_bytes_with_padding_works) {
    EXPECT_EQ(bits_to_bytes_with_padding({0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0}),
              std::vector<byte>({5, 0b11000000u, 0b01001001u}));
}

TEST(utils_test, bytes_with_padding_to_bits_works) {
    EXPECT_EQ(bytes_with_padding_to_bits({5, 0b11000000u, 0b01001001u}),
              std::vector<bool>({0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0}));
}

static bool test_tree_equiv(const huffman_tree_base::node &t1,
                            const huffman_tree_base::node &t2) {
    if (t1.is_leaf() != t2.is_leaf()) {
        return false;
    }
    if (t1.is_leaf()) {
        return t1.ch == t2.ch;
    }
    for (int i = 0; i <= 1; ++i) {
        if (test_tree_equiv(*t1.go[0], *t2.go[i]) &&
            test_tree_equiv(*t1.go[1], *t2.go[i ^ 1])) {
            return true;
        }
    }
    return false;
}

using std::make_unique;

class huffman_tree_test : public testing::Test {
protected:
    using node = huffman_tree_base::node;

    std::vector<byte> test_input{'a', 'a', 'b', 'a', 'c'};
    std::unique_ptr<huffman_encoding_tree> test_tree_ptr;

    void SetUp() override {
        test_tree_ptr = make_unique<huffman_encoding_tree>(
                ALL(test_input));
    }

    template<typename It>
    static byte get_char(const node &t, It begin, It end) {
        if (begin == end) {
            return t.ch;
        } else {
            return get_char(*t.go[*begin], next(begin), end);
        }
    }
};

TEST_F(huffman_tree_test, tree_construction_works_simple) {
    std::vector<byte> input{'a', 'a', 'b', 'a', 'c'};
    EXPECT_TRUE(test_tree_equiv(
            *huffman_encoding_tree(ALL(input)).tree,
            node{make_unique<node>('a'),
                 make_unique<node>(make_unique<node>('b'), make_unique<node>('c'))}
    ));
}

TEST_F(huffman_tree_test, tree_construction_works_same_char) {
    std::vector<byte> input{'a', 'a', 'a', 'a'};
    EXPECT_TRUE(test_tree_equiv(
            *huffman_encoding_tree(ALL(input)).tree,
            node{make_unique<node>('a'), make_unique<node>('b')}
    ));
}

TEST_F(huffman_tree_test, tree_construction_works_same_char_wrap) {
    auto ones = static_cast<byte>(-1);
    std::vector<byte> input{ones, ones, ones, ones};
    EXPECT_TRUE(test_tree_equiv(
            *huffman_encoding_tree(ALL(input)).tree,
            node{make_unique<node>(ones), make_unique<node>(0)}
    ));
}

TEST_F(huffman_tree_test, tree_construction_works_empty) {
    std::vector<byte> input{};
    EXPECT_TRUE(test_tree_equiv(
            *huffman_encoding_tree(ALL(input)).tree,
            node{make_unique<node>(0), make_unique<node>(1)}
    ));
}

TEST_F(huffman_tree_test, tree_codes_assigned_correctly) {
    std::sort(ALL(test_input));
    for (std::size_t ch = 0; ch < test_tree_ptr->codes.size(); ++ch) {
        const auto &code = test_tree_ptr->codes[ch];
        if (!code.empty()) {
            EXPECT_EQ(get_char(*test_tree_ptr->tree, ALL(code)), ch);
        } else {
            EXPECT_EQ(std::find(ALL(test_input), ch), std::end(test_input));
        }
    }
}

TEST_F(huffman_tree_test, tree_info_able_to_rebuild) {
    auto info = test_tree_ptr->get_tree_info();
    auto new_tree = huffman_decoding_tree(ALL(info));
    EXPECT_TRUE(test_tree_equiv(*test_tree_ptr->tree, *new_tree.tree));
}

TEST_F(huffman_tree_test, encode_and_decode) {
    std::vector<byte> data(BEE_MOVIE.length());
    std::copy(ALL(BEE_MOVIE), std::begin(data));
    std::stringstream log;
    huffman_runner r(log);
    auto enc_res = r.encode(data);
    auto dec_res = r.decode(enc_res.data);
    EXPECT_EQ(data, dec_res.data);
    EXPECT_EQ(data.size(), enc_res.initial_size);
    EXPECT_EQ(enc_res.initial_size, dec_res.processed_size);
    EXPECT_EQ(enc_res.processed_size, dec_res.initial_size);
    EXPECT_EQ(enc_res.aux_size, dec_res.aux_size);
}

