/*******************************************************************************
 * test/src/classic_index_construction.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include "test_util.hpp"
#include <cobs/util/file.hpp>
#include <cobs/util/fs.hpp>
#include <cobs/util/parameters.hpp>
#include <gtest/gtest.h>

namespace {
namespace fs = cobs::fs;
fs::path in_dir("test/out/classic_index_construction/input");
fs::path documents_dir(in_dir.string() + "/documents");
fs::path isi_2_dir(in_dir.string() + "/isi_2");
fs::path classic_index_path(in_dir.string() + "/index.cla_idx.isi");
fs::path tmp_dir("test/out/classic_index_construction/tmp");

std::string query = cobs::random_sequence(10000, 1);

class classic_index_construction : public ::testing::Test
{
protected:
    virtual void SetUp() {
        cobs::error_code ec;
        fs::remove_all(in_dir, ec);
        fs::remove_all(tmp_dir, ec);
        fs::create_directories(in_dir);
        fs::create_directories(tmp_dir);
    }
};

TEST_F(classic_index_construction, deserialization) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());

    cobs::classic_index::construct(tmp_dir, in_dir, 8, 3, 0.1);
    std::vector<uint8_t> data;
    cobs::file::classic_index_header h;
    h.read_file(classic_index_path, data);
    ASSERT_EQ(h.file_names().size(), 33U);
    ASSERT_EQ(h.num_hashes(), 3U);
}

TEST_F(classic_index_construction, file_names) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());

    std::vector<fs::path> paths;
    fs::recursive_directory_iterator it(tmp_dir), end;
    std::copy_if(it, end, std::back_inserter(paths), [](const auto& p) {
                     return cobs::file::file_is<cobs::file::document_header>(p);
                 });
    std::sort(paths.begin(), paths.end());

    cobs::classic_index::construct(tmp_dir, in_dir, 8, 3, 0.1);
    std::vector<uint8_t> data;
    auto h = cobs::file::deserialize_header<cobs::file::classic_index_header>(classic_index_path);
    h.read_file(classic_index_path, data);
    for (size_t i = 0; i < h.file_names().size(); i++) {
        ASSERT_EQ(h.file_names()[i], cobs::file::file_name(paths[i]));
    }
}

TEST_F(classic_index_construction, num_ones) {
    auto documents = generate_documents_all(query);
    generate_test_case(documents, tmp_dir.string());
    cobs::classic_index::construct(tmp_dir, in_dir, 8, 3, 0.1);
    std::vector<uint8_t> data;
    cobs::file::classic_index_header h;
    h.read_file(classic_index_path, data);

    std::map<std::string, size_t> num_ones;
    for (size_t j = 0; j < h.signature_size(); j++) {
        for (size_t k = 0; k < h.block_size(); k++) {
            uint8_t d = data[j * h.block_size() + k];
            for (size_t o = 0; o < 8; o++) {
                size_t file_names_index = k * 8 + o;
                if (file_names_index < h.file_names().size()) {
                    std::string file_name = h.file_names()[file_names_index];
                    num_ones[file_name] += (d & (1 << o)) >> o;
                }
            }
        }
    }

    double set_bit_ratio = cobs::calc_average_set_bit_ratio(h.signature_size(), 3, 0.1);
    double num_ones_average = set_bit_ratio * h.signature_size();
    for (auto& no : num_ones) {
        ASSERT_LE(no.second, num_ones_average * 1.01);
    }
}
}

/******************************************************************************/
