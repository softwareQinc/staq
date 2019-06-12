/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include "utils/source.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <optional>

namespace synthewareQ {
namespace detail {

inline std::string extract_name_from_path(const std::string& file_path)
{
	auto slash = file_path.rfind('/');
	if (slash == std::string::npos) {
		return file_path;
	} else {
		return file_path.substr(slash + 1);
	}
}

inline std::optional<std::string> load_content(const std::string& file_path)
{
	std::string content;
	// Don't accept a 'string_view' as parameter :(
	std::ifstream input_file(file_path);
	if (input_file.is_open()) {
		input_file.seekg(0, input_file.end);
		auto length = input_file.tellg();
		input_file.seekg(0);
		content.resize(length, '\0');
		input_file.read(&content[0], length);
		input_file.close();
		return content;
	}
	return std::nullopt;
}
} // namespace detail

class file final : public source {
public:
	file(const file&) = delete;
	file& operator=(const file&) = delete;

	static std::unique_ptr<file> open(const std::string& file_path, uint32_t offset)
	{
		auto name = detail::extract_name_from_path(file_path);
		auto content = detail::load_content(file_path);
		if (content) {
			return std::unique_ptr<file>(new file(name, content.value(), offset));
		}
		return nullptr;
	}

	std::string_view name() const override
	{
		return name_;
	}

private:
	file(std::string_view name, std::string_view content, uint32_t offset)
	    : source(content, offset)
	    , name_(name)
	{}

private:
	std::string name_;
};

} // namespace synthewareQ
