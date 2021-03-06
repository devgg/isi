/*******************************************************************************
 * cobs/file/compact_index_header.cpp
 *
 * Copyright (c) 2018 Florian Gauger
 *
 * All rights reserved. Published under the MIT License in the LICENSE file.
 ******************************************************************************/

#include <cobs/file/compact_index_header.hpp>

namespace cobs::file {

const std::string compact_index_header::magic_word = "COMPACT_INDEX";
const std::string compact_index_header::file_extension = ".com_idx.isi";

size_t compact_index_header::padding_size(uint64_t curr_stream_pos) const {
    return (m_page_size - ((curr_stream_pos + compact_index_header::magic_word.size()) % m_page_size)) % m_page_size;
}

void compact_index_header::serialize(std::ofstream& ofs) const {
    cobs::serialize(ofs, (uint32_t)m_parameters.size(), (uint32_t)m_file_names.size(), m_page_size);
    ofs.flush();
    for (const auto& p : m_parameters) {
        cobs::serialize(ofs, p.signature_size, p.num_hashes);
    }
    for (const auto& file_name : m_file_names) {
        ofs << file_name << std::endl;
    }

    std::vector<char> padding(padding_size(ofs.tellp()));
    ofs.write(padding.data(), padding.size());
}

void compact_index_header::deserialize(std::ifstream& ifs) {
    uint32_t parameters_size;
    uint32_t file_names_size;
    cobs::deserialize(ifs, parameters_size, file_names_size, m_page_size);
    m_parameters.resize(parameters_size);
    for (auto& p : m_parameters) {
        cobs::deserialize(ifs, p.signature_size, p.num_hashes);
    }

    m_file_names.resize(file_names_size);
    for (auto& file_name : m_file_names) {
        std::getline(ifs, file_name);
    }

    stream_metadata smd = get_stream_metadata(ifs);
    ifs.seekg(smd.curr_pos + padding_size(smd.curr_pos), std::ios::beg);
}

compact_index_header::compact_index_header(uint64_t page_size) : m_page_size(page_size) { }

compact_index_header::compact_index_header(const std::vector<compact_index_header::parameter>& parameters, const std::vector<std::string>& file_names, uint64_t page_size)
    : m_parameters(parameters), m_file_names(file_names), m_page_size(page_size) { }

const std::vector<compact_index_header::parameter>& compact_index_header::parameters() const {
    return m_parameters;
}

const std::vector<std::string>& compact_index_header::file_names() const {
    return m_file_names;
}

uint64_t compact_index_header::page_size() const {
    return m_page_size;
}

void compact_index_header::read_file(std::ifstream& ifs,
                                     std::vector<std::vector<uint8_t> >& data) {
    ifs.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
    header<compact_index_header>::deserialize(ifs, *this);
    data.clear();
    data.resize(parameters().size());
    for (size_t i = 0; i < parameters().size(); i++) {
        size_t data_size = page_size() * parameters()[i].signature_size;
        std::vector<uint8_t> d(data_size);
        ifs.read(reinterpret_cast<char*>(d.data()), data_size);
        data[i] = std::move(d);
    }
}

void compact_index_header::read_file(const fs::path& p,
                                     std::vector<std::vector<uint8_t> >& data) {
    std::ifstream ifs(p.string(), std::ios::in | std::ios::binary);
    read_file(ifs, data);
}

} // namespace cobs::file

/******************************************************************************/
