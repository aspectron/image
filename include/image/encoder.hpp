#ifndef IMAGE_ENCODER_HPP_INCLUDED
#define IMAGE_ENCODER_HPP_INCLUDED

#include "jsx/geometry.hpp"
#include "jsx/types.hpp"

namespace aspect { namespace image {

class bitmap;

class IMAGE_API png_color_type
{
public:
	enum value_type { palette, rgb, rgba };

	png_color_type(value_type value) : value_(value) {}
	operator value_type() const { return value_; }
private:
	value_type value_;
};

/// Compresses bitmap image rect into PNG and place in result buffer, return MIME type
/// compression is in [0..9], where 0 - no compression, 1 - best speed, 9 - best compression, -1 is default, see comression levels in libpng
///
IMAGE_API std::string generate_png(bitmap const& image, buffer& result, image_rect rect,
	bool flip = false, int compression = -1, png_color_type color_type = png_color_type::rgb);

inline std::string generate_png(bitmap const& image, buffer& result,
	bool flip = false, int compression = -1, png_color_type color_type = png_color_type::rgb)
{
	return generate_png(image, result, image_rect(0, 0, image.size().width, image.size().height),
		flip, compression, color_type);
}

/// Compresses bitmap image rect into JPEG and place in result buffer, return MIME type
IMAGE_API std::string generate_jpeg(bitmap const& image, buffer& result, image_rect rect, bool flip = false, int quality = 90);

inline std::string generate_jpeg(bitmap const& image, buffer& result, bool flip = false, int quality = 90)
{
	return generate_jpeg(image, result, image_rect(0, 0, image.size().width, image.size().height), flip, quality);
}

/// Compresses bitmap image rect into BMP and place in result buffer, return MIME type
IMAGE_API std::string generate_bmp(bitmap const& image, buffer& result, image_rect rect, bool flip = false, bool with_alpha = false);

inline std::string generate_bmp(bitmap const& image, buffer& result, bool flip = false, bool with_alpha = false)
{
	return generate_bmp(image, result, image_rect(0, 0, image.size().width, image.size().height), flip, with_alpha);
}

}} // namespace aspect::image


#endif // IMAGE_ENCODER_HPP_INCLUDED
