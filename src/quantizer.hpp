#ifndef _ASPECT_QUANTIZER_HPP_
#define _ASPECT_QUANTIZER_HPP_

#include "geometry.hpp"
#include "types.hpp"

namespace aspect
{

class IMAGE_API quantizer
{
public:
	quantizer()
		: lut_size_(0)
	{
	}
	
	void quantize(uint8_t const* pixels, unsigned stride, image_rect const& rect, size_t num_colors = 0xff);

	void clear() { result_data_.clear(); }

	void const* lut24() const { return &lut_.rgb; }
	void const* lut32() { return &lut_.rgba; }

	size_t lut_size() const { return lut_size_; }
	uint8_t* result_data() { return &result_data_[0]; }

private:

	struct lut
	{
		struct
		{
			uint8_t r,g,b;
		} rgb[256];

		struct
		{
			uint8_t r,g,b,a;
		} rgba[256];
	};

	buffer result_data_;

	lut lut_;
	size_t lut_size_;
};

} // aspect

#endif // _ASPECT_QUANTIZER_HPP_
