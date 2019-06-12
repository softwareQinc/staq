/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "utils/file.hpp"
#include "utils/source.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace synthewareQ {

// This class handles loading source files into memory.
// TODO: use <filesystem>
class source_manager {
	using source_pointer = std::unique_ptr<source>;

	std::map<uint32_t, source_pointer> location_map_;
	uint32_t next_offset_ = 0;

	// TODO: Eventually I will have to properly implement this:
	std::string path_;

public:
	source_manager() = default;
	source_manager(const source_manager&) = delete;

	source* add_target_file(const std::string_view file_path)
	{
		auto temp = path_;
		if (!path_.empty()) {
			temp.append(file_path);
		} else {
			auto const pos = file_path.find_last_of('/');
			path_ = file_path.substr(0, pos + 1);
			temp = file_path;
		}
		auto file = file::open(temp, next_offset_);
		if (file != nullptr) {
			auto file_ptr = &(*file);
			next_offset_ += file->length() + 1;
			location_map_.emplace(next_offset_, std::move(file));
			return file_ptr;
		}
		return nullptr;
	}

	source* add_target_buffer(const std::string_view buffer)
	{
		auto buf = source::build(buffer, next_offset_);
		auto buf_ptr = &(*buf);
		next_offset_ += buf->length() + 1;
		location_map_.emplace(next_offset_, std::move(buf));
		return buf_ptr;
	}

	std::string location_str(const uint32_t location) const
	{
		std::stringstream ss;
		ss << "<" << location_map_.lower_bound(location)->second->name() << ":"
		   << location_map_.lower_bound(location)->second->line(location) << ":"
		   << location_map_.lower_bound(location)->second->column(location) << ">";
		return ss.str();
	}
};

} // namespace synthewareQ
