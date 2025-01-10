/// Partial implementation of ZIP archive specs:
/// https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT relies on the
/// known subset of ZIP features used to append SFX-zip resources to binary
#include "sfx.hpp"
#include "zip.hpp"

#include <thinsys/io/input.hpp>
#include <thinsys/io/io.hpp>
#include <thinsys/io/span_io.hpp>

#include <libs/memtricks/object_bytes.hpp>

namespace sfx {

namespace {

constexpr uint32_t zip64_wide_marker = ~0;

struct end_of_cd_record {
  static constexpr uint32_t valid_signature = 0x06054b50;

  // end of central dir signature    4 bytes  (0x06054b50)
  uint32_t signature;
  // number of this disk             2 bytes
  uint16_t disk_num;
  // number of the disk with the
  // start of the central directory  2 bytes
  uint16_t cd_start_disk_num;
  // total number of entries in the
  // central directory on this disk  2 bytes
  uint16_t this_disk_entries_count;
  // total number of entries in
  // the central directory           2 bytes
  uint16_t total_entries_count;
  // size of the central directory   4 bytes
  uint32_t cd_size;
  // offset of start of central
  // directory with respect to
  // the starting disk number        4 bytes
  uint32_t cd_offset;

  // NOTE: Out of interest:
  static constexpr std::streamoff empty_zip_comment_offset = 2;
  // .ZIP file comment length        2 bytes
  // .ZIP file comment       (variable size)

  static end_of_cd_record read(thinsys::io::file_descriptor& in) {
    std::streamoff cd_off = sizeof(end_of_cd_record) + end_of_cd_record::empty_zip_comment_offset;
    thinsys::io::seek(in, -cd_off, thinsys::io::seek_whence::end);

    end_of_cd_record res;
    thinsys::io::read(in, object_bytes(res));

    if (res.signature != end_of_cd_record::valid_signature)
      throw std::runtime_error{"Can't locate end of central directory record. File is either not a "
                               "zip archive or has triling .ZIP comment."};
    if (res.disk_num != 0 || res.cd_start_disk_num != 0)
      throw std::runtime_error{"Multi disk ZIP archives are not supported."};

    return res;
  }
};
static_assert(sizeof(end_of_cd_record) == 20, "Paddings kills reading this struct");

struct cd_file_header {
  struct base {
    static constexpr uint32_t valid_signature = 0x02014b50;

    // central file header signature   4 bytes  (0x02014b50)
    uint32_t signature;
    // version made by                 2 bytes
    uint16_t version_made_by;
    // version needed to extract       2 bytes
    uint16_t version_needed_to_extract;
    // general purpose bit flag        2 bytes
    uint16_t gp_bits;
    // compression method              2 bytes
    zip::compression compression_method;
    // last mod file time              2 bytes
    uint16_t modtime;
    // last mod file date              2 bytes
    uint16_t moddate;
    // crc-32                          4 bytes
    uint32_t crc32;
    // compressed size                 4 bytes
    uint32_t compressed_size;
    // uncompressed size               4 bytes
    uint32_t raw_size;
  };
  static_assert(sizeof(base) == 28, "Paddings kills reading this struct");

  struct str_sizes {
    // file name length                2 bytes
    uint16_t fname_size;
    // extra field length              2 bytes
    uint16_t extrafield_size;
    // file comment length             2 bytes
    uint16_t file_comment_size;
  };
  static_assert(sizeof(str_sizes) == 6, "Paddings kills reading this struct");

  struct attrs {
    // disk number start               2 bytes
    uint16_t disk_num_start;
    // internal file attributes        2 bytes
    uint16_t internal_attrs;
    // external file attributes        4 bytes
    uint32_t external_attrs;
    // relative offset of local header 4 bytes
    uint32_t local_header_offset;

    // file name (variable size)
    // extra field (variable size)
    // file comment (variable size)
  };
  static_assert(sizeof(attrs) == 12, "Paddings kills reading this struct");

  static cd_file_header read(thinsys::io::file_descriptor& in) {
    cd_file_header res;

    thinsys::io::read(in, object_bytes(res.base_info));
    if (res.base_info.signature != cd_file_header::base::valid_signature)
      throw std::runtime_error{"Bad central directory file header signature"};

    str_sizes sizes;
    thinsys::io::read(in, object_bytes(sizes));
    thinsys::io::read(in, object_bytes(res.file_attrs));

    std::string fname;
    fname.resize(sizes.fname_size);
    thinsys::io::read(in, std::as_writable_bytes(std::span{fname}));

    res.filename = std::move(fname);
    thinsys::io::seek(in, sizes.extrafield_size + sizes.file_comment_size, thinsys::io::seek_whence::cur);

    return res;
  }

  base base_info;
  attrs file_attrs;
  fs::path filename;
};

struct local_file_header {
  static constexpr uint32_t valid_signature = 0x04034b50;
  static constexpr size_t serialized_size = 30;

  // local file header signature     4 bytes  (0x04034b50)
  uint32_t signature;
  // version needed to extract       2 bytes
  uint16_t version_needed_to_extract;
  // general purpose bit flag        2 bytes
  uint16_t gp_bits;
  // compression method              2 bytes
  zip::compression compression_method;
  // last mod file time              2 bytes
  uint16_t modtime;
  // last mod file date              2 bytes
  uint16_t moddate;
  // crc-32                          4 bytes
  uint32_t crc32;
  // compressed size                 4 bytes
  uint32_t compressed_size;
  // uncompressed size               4 bytes
  uint32_t raw_size;
  // file name length                2 bytes
  uint16_t fname_size;
  // extra field length              2 bytes
  uint16_t extra_field_size;

  // file name (variable size)
  // extra field (variable size)

  static local_file_header read(thinsys::io::file_descriptor& in) {
    std::array<std::byte, serialized_size> buf;
    thinsys::io::read(in, buf);

    std::span<const std::byte> buf_tail{buf};
    local_file_header res;
    thinsys::io::read(buf_tail, object_bytes(res.signature));
    if (res.signature != valid_signature)
      throw std::runtime_error{"bad local file header signature"};
    thinsys::io::read(buf_tail, object_bytes(res.version_needed_to_extract));
    thinsys::io::read(buf_tail, object_bytes(res.gp_bits));
    thinsys::io::read(buf_tail, object_bytes(res.compression_method));
    if (res.compression_method != zip::compression::stored)
      throw std::runtime_error{"compressed resources are not supported"};
    thinsys::io::read(buf_tail, object_bytes(res.modtime));
    thinsys::io::read(buf_tail, object_bytes(res.moddate));
    thinsys::io::read(buf_tail, object_bytes(res.crc32));
    thinsys::io::read(buf_tail, object_bytes(res.compressed_size));
    thinsys::io::read(buf_tail, object_bytes(res.raw_size));
    thinsys::io::read(buf_tail, object_bytes(res.fname_size));
    thinsys::io::read(buf_tail, object_bytes(res.extra_field_size));

    thinsys::io::seek(in, res.fname_size + res.extra_field_size, thinsys::io::seek_whence::cur);

    return res;
  }
};

size_t read_data_offsed_from_local_file_header(thinsys::io::file_descriptor& in) {
  const auto hdr = local_file_header::read(in);
  return local_file_header::serialized_size + hdr.fname_size + hdr.extra_field_size;
}

thinsys::io::file_descriptor open_self_stream() {
  const fs::path self = "/proc/self/exe";
  return thinsys::io::open(self, thinsys::io::mode::read_only);
}

} // namespace

thinsys::io::file_descriptor& archive::open(entry& e) {
  thinsys::io::seek(fd_, e.offset, thinsys::io::seek_whence::set);
  if (!e.offset_is_adjusted) {
    e.offset += read_data_offsed_from_local_file_header(fd_);
    e.offset_is_adjusted = true;
  }
  return fd_;
}
thinsys::io::file_descriptor& archive::open(const fs::path& path) {
  const auto it = entries_.find(path);
  if (it == entries_.end())
    throw std::system_error{std::make_error_code(std::errc::no_such_file_or_directory), "sfx-archive open"};
  return open(it->second);
}

archive archive::open_self() {
  auto self = open_self_stream();
  const auto cd_end = end_of_cd_record::read(self);

  if (cd_end.cd_offset == zip64_wide_marker)
    throw std::runtime_error{"zip64 archives are not supported"};

  std::unordered_map<fs::path, entry> entries;
  thinsys::io::seek(self, cd_end.cd_offset, thinsys::io::seek_whence::set);
  for (int i = 0; i < cd_end.total_entries_count; ++i) {
    const auto file_rec = cd_file_header::read(self);
    entries.emplace(
        file_rec.filename,
        entry{
            .size = file_rec.base_info.raw_size,
            .offset = file_rec.file_attrs.local_header_offset,
            .offset_is_adjusted = false
        }
    );
  }

  return {std::move(entries), std::move(self)};
}

} // namespace sfx
