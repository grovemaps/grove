#pragma once

#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include <atomic>
#include <concepts>
#include <string>
#include <string_view>

namespace grove
{
// Writes a cache file through a temporary file and a rename, so readers on other threads never see half of it and a
// crash never leaves a truncated one behind. write(FileWriter &) writes the contents. False on errors.
template <class WriteFn>
  requires std::invocable<WriteFn, FileWriter &>
bool WriteAtomically(std::string const & path, WriteFn && write)
{
  static std::atomic<uint64_t> counter{0};
  std::string const tmp = path + ".tmp" + std::to_string(counter.fetch_add(1));
  try
  {
    {
      FileWriter writer(tmp);
      write(writer);
    }
    if (base::RenameFileX(tmp, path))
      return true;
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Can't write", path, e.Msg()));
  }
  base::DeleteFileX(tmp);
  return false;
}

inline bool WriteAtomically(std::string const & path, std::string_view bytes)
{
  return WriteAtomically(path, [bytes](FileWriter & w) { w.Write(bytes.data(), bytes.size()); });
}
}  // namespace grove
