/*
 *  Created on: 2018/09/30
 *      Author: take
 */
#include "Image.h"

namespace MinDxr {

Image::Image(uint32_t w, uint32_t h) :m_data(std::make_unique<uint32_t[]>(w*h)),m_size(w*h*sizeof(uint32_t))
{
}

}  // namespace MinDxr

