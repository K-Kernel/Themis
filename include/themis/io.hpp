#pragma once
#include <cstddef>
#include <themis/buffer.hpp>
#include <themis/table.hpp>
#include <vector>


inline Table slice_csv(Buffer csv){
	std::string_view whole{csv.buffer_view.data(), csv.buffer_view.size()};

	std::vector<std::string_view> header{};
	
	size_t ncols{1};
	for(auto c: whole){if (c == ',') {
		++ncols;
	}else if (c == '\n'){
		break;
	}}
	

	
	std::vector<std::string_view>cells;

	size_t field_start{0};
	for(size_t i{0}; i < whole.size(); ++i){
		if(whole[i] == ',' || whole[i] == '\n'){
			cells.push_back(whole.substr(field_start, i - field_start));
			field_start = i+1;
		}
	}

	if (field_start < whole.size()) {
		cells.push_back(whole.substr(field_start, whole.size() - field_start));
	}



	return Table(csv,header, cells, ncols);
}





