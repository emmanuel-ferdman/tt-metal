// SPDX-FileCopyrightText: (c) 2024 Tenstorrent AI ULC
//
// SPDX-License-Identifier: Apache-2.0

#include "tokenizers/bpe_tokenizer.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <vector>

using namespace ttml::tokenizers;

namespace {
std::string getTestDataDir() {
    const char* envVar = std::getenv("TEST_DATA_DIR");
    return (envVar) ? std::string(envVar) : std::string(TEST_DATA_DIR);
}
}

class BPETokenizerTest : public ::testing::Test {
protected:
    BPETokenizer tokenizer = BPETokenizer(getTestDataDir() + "/tokenizer.json");
};

TEST_F(BPETokenizerTest, EncodeAndDecode) {
    const std::string prompt = "What is the  capital of Canada?";
    auto ids = tokenizer.encode(prompt);
    auto decoded_prompt = tokenizer.decode(ids);
    EXPECT_EQ(decoded_prompt, prompt);
}

TEST_F(BPETokenizerTest, IdToTokenAndTokenToId) {
    auto vocab_size = tokenizer.get_vocab_size();
    EXPECT_EQ(vocab_size, 50277);
}