CXX = g++
CXX_FLAGS = -pedantic -Wall -Wextra -Werror -std=c++17 -O2
LD_FLAGS =

EXE = huffman
TEST_EXE = test
SRC_DIR = src
TEST_SRC_DIR = test
BIN_DIR = bin
TEST_BIN_DIR = bin

GTEST_LIB = libgtest.a
GTEST_DIR = test/gtest
GTEST_CXX_FLAGS = -std=c++17 -isystem $(GTEST_DIR)/include -I$(GTEST_DIR) -pthread

TEST_CXX_FLAGS = -pedantic -Wall -Wextra -Werror -std=c++17 -isystem $(GTEST_DIR)/include -I$(SRC_DIR) -pthread

OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.o, $(wildcard $(SRC_DIR)/*.cpp))
TEST_OBJECTS = $(patsubst $(TEST_SRC_DIR)/%.cpp,$(TEST_BIN_DIR)/%.o, $(wildcard $(TEST_SRC_DIR)/*.cpp))

# Main application

huffman: $(BIN_DIR)/$(EXE)

$(BIN_DIR)/$(EXE): $(BIN_DIR) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LD_FLAGS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXX_FLAGS) -c -MMD -o $@ $<

# Tests

test: $(TEST_BIN_DIR)/$(TEST_EXE)

$(TEST_BIN_DIR)/$(TEST_EXE): $(TEST_BIN_DIR) $(TEST_OBJECTS) $(TEST_BIN_DIR)/$(GTEST_LIB)
	$(CXX) $(TEST_OBJECTS) $(TEST_BIN_DIR)/$(GTEST_LIB) -o $@ $(LD_FLAGS)

$(TEST_BIN_DIR)/$(GTEST_LIB): $(TEST_BIN_DIR)/gtest-all.o
	ar -rv $@ $<

$(TEST_BIN_DIR)/gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	$(CXX) $(GTEST_CXX_FLAGS) -c -o $@ $<

$(TEST_BIN_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp
	$(CXX) $(TEST_CXX_FLAGS) -c -MMD -o $@ $<


include $(wildcard $(BIN_DIR)/*.d) $(wildcard $(TEST_BIN_DIR)/*.d)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Uncomment if BIN_DIR and TEST_BIN_DIR are different
#$(TEST_BIN_DIR):
# 	mkdir -p $(TEST_BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

.PHONY: clean all test gtest
