#include "ply.h"
#include "logging.h"

#include <regex>

namespace viewer::ply {

PlyType ply_type_from_string(const std::string& t) {
    if (t == "double") return PlyType::Double;
    if (t == "float") return PlyType::Float;
    if (t == "int") return PlyType::Int;
    if (t == "uint") return PlyType::UInt;
    if (t == "short") return PlyType::Short;
    if (t == "ushort") return PlyType::UShort;
    if (t == "char") return PlyType::Char;
    if (t == "uchar") return PlyType::UChar;
    LOG_FATAL("invalid type: %s", t.c_str());
}

size_t ply_type_size(PlyType t) {
    switch (t) {
    case PlyType::Double: return sizeof(double);
    case PlyType::Float: return sizeof(float);
    case PlyType::Int:
    case PlyType::UInt: return sizeof(int);
    case PlyType::Short:
    case PlyType::UShort: return sizeof(short);
    case PlyType::Char:
    case PlyType::UChar: return sizeof(char);
    default: LOG_FATAL("should never happen");
    }
}

PlyHeader::PlyHeader(const llfio::mapped_file_handle& file) {
    // Find header
    char* const file_begin = reinterpret_cast<char*>(file.address());
    header_end_idx = [&]() {
        constexpr std::string_view END_HEADER_STR = "end_header\n";
        const auto length = file.maximum_extent().value();
        for (char* p = file_begin;
             (p = (char*)std::memchr(p, END_HEADER_STR.at(0), file_begin + length - p));
             p++) {
            if (END_HEADER_STR.compare(0, END_HEADER_STR.size(), p, END_HEADER_STR.size()) == 0)
                return p - file_begin + END_HEADER_STR.size();
        }
        LOG_FATAL("could not parse PLY file");
    }();

    // Extract header
    const std::string ply_header(file_begin, file_begin+header_end_idx);

    if (ply_header.rfind("ply\nformat binary_little_endian 1.0", 0) != 0)
        LOG_FATAL("unsupported PLY format");

    // Find num vertices
    const std::regex VERTEX_REGEX("element vertex (\\d+)\n");
    auto vtx_matches = std::sregex_iterator(ply_header.begin(), ply_header.end(),
                                            VERTEX_REGEX);
    if (vtx_matches == std::sregex_iterator())
        LOG_FATAL("could not parse number of vertices");
    num_vertices = std::stoi((*vtx_matches)[1].str());

    // Find properties
    const std::regex PROPERTY_REGEX("property (\\w+) (\\w+)\n");
    const auto prop_match_begin =
        std::sregex_iterator(ply_header.begin(), ply_header.end(), PROPERTY_REGEX);
    props.clear();
    for (auto prop_match = prop_match_begin;
         prop_match != std::sregex_iterator();
         ++prop_match) {
        // const size_t idx = std::distance(prop_match_begin, prop_match);
        const std::string type = (*prop_match)[1].str();
        const std::string name = (*prop_match)[2].str();
        // LOG_INFO("prop %lu: <%s> %s", idx, type.c_str(), name.c_str());
        props.emplace_back(type, name);
    }

    // Find offsets
    row_length = 0;
    offsets.clear();
    for (const PlyProperty& prop : props) {
        offsets.push_back(row_length);
        row_length += ply_type_size(prop.type);
    }
}

}
