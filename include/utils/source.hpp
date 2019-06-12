/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Bruno Schmitt
| Forked from boschmitt/synthewareQ
*------------------------------------------------------------------------------------------------*/
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace synthewareQ {

// This object owns the file content string.
class source {
	std::string content_;
	uint32_t offset_;
	mutable std::map<unsigned, unsigned> line_map_;

private:
	void construct_line_map() const
	{
		auto line_number = 0u;
		for (auto i = 0u; i < content_.length(); ++i) {
			if (content_[i] == '\n') {
				line_map_.emplace(i, ++line_number);
			}
		}
		line_map_.emplace(content_.length(), ++line_number);
	}

public:
	source(const source&) = delete;
	source& operator=(const source&) = delete;

	static std::unique_ptr<source> build(std::string_view content, uint32_t offset)
	{
		return std::unique_ptr<source>(new source(content, offset));
	}

	std::string_view content() const
	{
		return content_;
	}

	virtual std::string_view name() const
	{
		return "";
	}

	std::string_view content(uint32_t pos, uint32_t len) const
	{
		return std::string_view(&content_[pos], len);
	}

	uint32_t length() const
	{
		return content_.length();
	}

	uint32_t offset() const
	{
		return offset_;
	}

	uint32_t line(uint32_t location) const
	{
		if (line_map_.size() == 0) {
			construct_line_map();
		}
		return line_map_.lower_bound(location - offset_)->second;
	}

	uint32_t column(uint32_t location) const
	{
		uint32_t line_start = location - offset_;
		while (line_start && content_[line_start - 1] != '\n'
		       && content_[line_start - 1] != '\r') {
			--line_start;
		}
		return location - offset_ - line_start + 1;
	}

protected:
	source(const std::string_view content, const uint32_t offset)
	    : content_(content)
	    , offset_(offset)
	{}
};

} // namespace synthewareQ
