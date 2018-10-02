/*
 *  Created on: 2018/09/30
 *      Author: take
 */

#pragma once


#include <memory>

namespace MinDxr {

class Image{
private:
	std::unique_ptr<uint32_t[]> m_data;
	size_t m_size;
public:
	Image(uint32_t w, uint32_t h);
	uint32_t* GetData() { return m_data.get(); }
	size_t GetSize() { return m_size; }
};

}  // namespace MinDxr


