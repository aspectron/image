#include "image/image.hpp"
#include "image/encoder.hpp"
#include "image/quantizer.hpp"

#include <png.h>
#include <jpeglib.h>

#include <cassert>

namespace aspect { namespace image {

template<typename T>
T clamp(T value, T low, T high)
{
	return value < low ? low : (value > high? high : value);
}

inline image_rect clamped_rect(bitmap const& image, image_rect rect)
{
	rect = image_rect(
		clamp(rect.left, 0, image.size().width),
		clamp(rect.top, 0, image.size().height),
		clamp(rect.width, 0, image.size().width - rect.left),
		clamp(rect.height, 0, image.size().height - rect.top)
	);

	assert(image.data() && !image.size().is_empty() && !rect.is_empty());
	assert(image.pixel_format() == RGBA8
		|| image.pixel_format() == ARGB8
		|| image.pixel_format() == BGRA8
		||  image.pixel_format() == RGB8);

	return rect;
}

inline int libpng_color_type(png_color_type color_type)
{
	switch (color_type)
	{
	case png_color_type::palette:
		return PNG_COLOR_TYPE_PALETTE;
	case png_color_type::rgb:
		return PNG_COLOR_TYPE_RGB;
	case png_color_type::rgba:
		return PNG_COLOR_TYPE_RGBA;
	default:
		assert(false && "unknown png_color_type");
		return -1;
	}
}

// -- callbacks
inline void png_write_file(png_struct* png, png_byte* data, png_size_t size)
{
	buffer& result = *reinterpret_cast<buffer*>(png_get_io_ptr(png)); // user data
	
	size_t const pos = result.size();
	result.resize(pos + size);
	memcpy(&result[pos], data, size);
}

inline void png_flush_file(png_struct*)
{
	// do nothing - used for flushing file i/o
}

template<typename F>
struct scope_guard_t
{
	scope_guard_t(F&& fun) : on_exit(std::move(fun)) {}
	~scope_guard_t() { on_exit(); }
	F on_exit;
};

template<typename F>
auto scope_guard(F&& f) { return scope_guard_t<F>(std::move(f)); }

std::string generate_png(bitmap const& image, buffer& result, image_rect rect,
	bool flip, int compression, png_color_type color_type)
{
	rect = clamped_rect(image, rect);

	uint8_t const* pixels = image.data();
	size_t stride = rect.width * image.bytes_per_pixel();
	size_t const bytes_per_pixel = image.bytes_per_pixel();

	png_struct* png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info* info = png_create_info_struct(png);
	auto const destroy_on_exit = scope_guard([&png, &info]()
	{
		png_destroy_info_struct(png, &info);
		png_destroy_write_struct(&png, &info);
	});

	assert(png && info);
	png_set_write_fn(png, &result, &png_write_file, &png_flush_file);

	png_set_compression_level(png, compression);

	png_set_IHDR(png, info, rect.width, rect.height, 8, libpng_color_type(color_type),
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info_before_PLTE(png, info);

	png_set_packing(png);

	if (image.pixel_format() == BGRA8)
	{
		png_set_bgr(png);
	}
	if (bytes_per_pixel == 4)
	{
		if (color_type == png_color_type::rgba)
		{
			if (image.pixel_format() == ARGB8)
			{
				png_set_swap_alpha(png);
			}
			png_set_invert_alpha(png);
		}
		else if (color_type == png_color_type::rgb)
		{
			png_set_filler(png, 0, image.pixel_format() == ARGB8? PNG_FILLER_BEFORE : PNG_FILLER_AFTER);
		}
	}

	int x = static_cast<int>(rect.left * bytes_per_pixel);
	int y = rect.top;
	int y_end = rect.bottom();
	int dy = 1;

	aspect::image::quantizer quantizer;
	if (color_type == png_color_type::palette)
	{
		quantizer.quantize(pixels, stride, rect, 0xff);
		png_set_PLTE(png, info, (png_color*)quantizer.lut24(), 0xff);

		// quantizer generates index data already in the desired resolution, just store it
		x = 0;
		y = 0;
		y_end = rect.height;
		pixels = quantizer.result_data();
		stride = rect.width;
	}

	if (flip)
	{
		std::swap(y, y_end);
		--y; --y_end;
		dy = -1;
	}

	png_write_info(png, info);
	for (; y != y_end; y += dy)
	{
		png_write_row(png, &pixels[(y * stride) + x]);
	}
	png_write_end(png, NULL);

	return "image/png";
}

std::string generate_jpeg(bitmap const& image, buffer& result, image_rect rect, bool flip, int quality)
{
	rect = clamped_rect(image, rect);

	uint8_t const* const pixels = image.data();
	size_t const stride = rect.width * image.bytes_per_pixel();
	size_t const bytes_per_pixel = image.bytes_per_pixel();

	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
	unsigned char* buf_data = NULL;
	unsigned long  buf_size = 0;

	auto const destroy_on_exit = scope_guard([&cinfo, buf_data]()
	{
		jpeg_destroy_compress(&cinfo);
		free(buf_data);
	});

	// Step 1: allocate and initialize JPEG compression objects
	cinfo.err = jpeg_std_error(&jerr);
	//jerr.error_exit = GAPI_JpegErrorHandler;
	//jerr.output_message = GAPI_JpegWarningMessage;
	jpeg_create_compress(&cinfo);

	// Step 2: specify data destination (eg, a file)
	// asy - compressing in memory
	jpeg_mem_dest(&cinfo, &buf_data, &buf_size);

	// Step 3: set parameters for compression
	cinfo.image_width			= rect.width;      // image width and height, in pixels
	cinfo.image_height			= rect.height;
	cinfo.input_components		= static_cast<int>(bytes_per_pixel); // # of color components per pixel
	// colorspace of input image
	switch (image.pixel_format())
	{
	case RGBA8:
		cinfo.in_color_space = JCS_EXT_RGBA;
		break;
	case ARGB8:
		cinfo.in_color_space = JCS_EXT_ARGB;
		break;
	case BGRA8:
		cinfo.in_color_space = JCS_EXT_BGRA;
		break;
	case RGB8:
		cinfo.in_color_space = JCS_EXT_RGB;
		break;
	default:
		assert(false && "unsupported pixel format");
		return "";
	}

	// Now use the library's routine to set default compression parameters.
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	// Step 4: Start compressor
	// TRUE ensures that we will write a complete interchange-JPEG file
	jpeg_start_compress(&cinfo, TRUE);

	// Step 5: while (scan lines remain to be written)
	//           jpeg_write_scanlines(...); 
	//
	// Here we use the library's state variable cinfo.next_scanline as the
	// loop counter, so that we don't have to keep track ourselves.
	// To keep things simple, we pass one scanline per call; you can pass
	// more if you wish, though.
	//
	int const x = static_cast<int>(rect.left * bytes_per_pixel);
	int y = 0;
	int y_end = cinfo.image_height;
	int dy = 1;
	if (flip)
	{
		std::swap(y, y_end);
		--y; --y_end;
		dy = -1;
	}

	for (; y != y_end; y += dy)
	{
		uint8_t const* line = &pixels[(y * stride) + x];
		jpeg_write_scanlines(&cinfo, const_cast<uint8_t**>(&line), 1);
	}

	// Step 6: Finish compression
	jpeg_finish_compress(&cinfo);

	// copy back the memory to the requester
	// TODO - make libjpeg populate existing/persistent buffer
	result.resize(buf_size);
	if (buf_size > 0)
	{
		memcpy(&result[0], buf_data, buf_size);
	}

	return "image/jpeg";
}

// resize result buffer and fill BMP file headers for a 32 bit BMP with specified
// dimensions and pixel format masks
static size_t fill_bmp_headers(buffer& result, int32_t width, int32_t height,
	uint32_t red_mask, uint32_t green_mask, uint32_t blue_mask, uint32_t alpha_mask)
{
	// See Windows SDK for BITMAPFILEHEADER and BITMAPV4HEADER structures
	static uint32_t const sizeof_BITMAPFILEHEADER = 14;
	static uint32_t const sizeof_BITMAPV4HEADER = 108;

	// offsets in BITMAPFILEHEADER
	static uint32_t const BITMAPFILEHEADER_bfType = 0;
	static uint32_t const BITMAPFILEHEADER_bfSize = 2;
	static uint32_t const BITMAPFILEHEADER_bfOffBits = 10;

	// offsets in BITMAPV4HEADER
	static uint32_t const BITMAPV4HEADER_bV4Size = 0;
	static uint32_t const BITMAPV4HEADER_bV4Width = 4;
	static uint32_t const BITMAPV4HEADER_bV4Height = 8;
	static uint32_t const BITMAPV4HEADER_bV4Planes = 12;
	static uint32_t const BITMAPV4HEADER_bV4BitCount = 14;
	static uint32_t const BITMAPV4HEADER_bV4V4Compression = 16;
	static uint32_t const BITMAPV4HEADER_bV4RedMask = 40;
	static uint32_t const BITMAPV4HEADER_bV4GreenMask = 44;
	static uint32_t const BITMAPV4HEADER_bV4BlueMask = 48;
	static uint32_t const BITMAPV4HEADER_bV4AlphaMask = 52;

	static uint16_t const planes = 1;
	static uint16_t const bits = 32;
	static uint32_t const compression= 3; // BI_BITFIELDS

	static uint32_t const info_offset = sizeof_BITMAPFILEHEADER;
	static uint32_t const pixels_offset = sizeof_BITMAPFILEHEADER + sizeof_BITMAPV4HEADER;

	uint32_t const image_size = width * height * 4;
	uint32_t const file_size = pixels_offset + image_size;

	result.resize(file_size);

	// fill BITMAPFILEHEADER
	memcpy(&result[BITMAPFILEHEADER_bfType], "BM", 2);
	memcpy(&result[BITMAPFILEHEADER_bfSize], &file_size, 4);
	memcpy(&result[BITMAPFILEHEADER_bfOffBits], &pixels_offset, 4);

	// fill BITMAPV4HEADER
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4Size], &sizeof_BITMAPV4HEADER, 4);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4Width], &width, 4);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4Height], &height, 4);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4Planes], &planes, 2);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4BitCount], &bits, 2);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4V4Compression], &compression, 4);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4RedMask], &red_mask, 4);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4GreenMask], &green_mask, 4);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4BlueMask], &blue_mask, 4);
	memcpy(&result[info_offset + BITMAPV4HEADER_bV4AlphaMask], &alpha_mask, 4);

	return pixels_offset;
}

std::string generate_bmp(bitmap const& image, buffer& result, image_rect rect, bool flip, bool with_alpha)
{
	rect = clamped_rect(image, rect);

	uint32_t red_mask, green_mask, blue_mask, alpha_mask;

	switch (image.pixel_format())
	{
	case RGBA8:
		red_mask   = 0x000000FF;
		green_mask = 0x0000FF00;
		blue_mask  = 0x00FF0000;
		alpha_mask = 0xFF000000;
		break;
	case ARGB8:
		alpha_mask = 0x000000FF;
		red_mask   = 0x0000FF00;
		green_mask = 0x00FF0000;
		blue_mask  = 0xFF000000;
		break;
	case BGRA8:
		blue_mask  = 0x000000FF;
		green_mask = 0x0000FF00;
		red_mask   = 0x00FF0000;
		alpha_mask = 0xFF000000;
		break;
	default:
		assert(false && "unsupported pixel format");
		return "";
	}
	assert(image.bytes_per_pixel() == 4);

	size_t const pixels_offset = fill_bmp_headers(result, rect.width, rect.height,
		red_mask, green_mask, blue_mask, with_alpha? alpha_mask : 0);

	uint8_t const* pixels = image.data();
	size_t const stride = rect.width * 4;

	int const x = rect.left * 4;
	int y = rect.top;
	int y_end = rect.bottom();
	int dy = 1;
	if (flip)
	{
		std::swap(y, y_end);
		--y; --y_end;
		dy = -1;
	}
	for (int iY = rect.height - 1; y != y_end; y += dy, --iY)
	{
		uint8_t const* src = &pixels[(y * stride) + x];
		uint8_t* dst = &result[pixels_offset] + (iY * stride);
		memcpy(dst, src, stride);
	}

	return "image/bmp";
}

}} // namespace aspect::image
