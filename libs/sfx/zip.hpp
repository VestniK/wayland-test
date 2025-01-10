#pragma once

#include <cstdint>

namespace sfx::zip {

enum class compression : uint16_t {
  stored = 0,                  // The file is stored (no compression)
  shrunk = 1,                  // The file is Shrunk
  reduced_factor1 = 2,         // The file is Reduced with compression factor 1
  reduced_factor2 = 3,         // The file is Reduced with compression factor 2
  reduced_factor3 = 4,         // The file is Reduced with compression factor 3
  reduced_factor4 = 5,         // The file is Reduced with compression factor 4
  imploded = 6,                // The file is Imploded
  reserved_4_torenization = 7, // Reserved for Tokenizing compression algorithm
  deflated = 8,                // The file is Deflated
  deflate64 = 9,               // Enhanced Deflating using Deflate64(tm)
  pkware_implode = 10,         // PKWARE Data Compression Library Imploding (old IBM TERSE)
  pkware_reseved1 = 11,        // Reserved by PKWARE
  bzip2 = 12,                  // File is compressed using BZIP2 algorithm
  pkware_reseved2 = 13,        // Reserved by PKWARE
  lzma = 14,                   // LZMA
  pkware_reseved3 = 15,        // Reserved by PKWARE
  cmpsc = 16,                  // IBM z/OS CMPSC Compression
  pkware_reseved4 = 17,        // Reserved by PKWARE
  ibm_terse = 18,              // File is compressed using IBM TERSE (new)
  ibm_lz77 = 19,               // IBM LZ77 z Architecture
  deprecated_zstd = 20,        // deprecated (use method 93 for zstd)
  zstd = 93,                   // Zstandard (zstd) Compression
  mp3 = 94,                    // MP3 Compression
  xz = 95,                     // XZ Compression
  jpeg = 96,                   // JPEG variant
  wav_pack = 97,               // WavPack compressed data
  ppmd = 98,                   // PPMd version I, Rev 1
  ae_x = 99,                   // AE-x encryption marker (see APPENDIX E)
};

}
