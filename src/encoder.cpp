#include "image.hpp"
#include "encoder.hpp"

#include <zlib.h>
#include <png.h>
#if HAVE(JPEG_SUPPORT)
#include <jpeglib.h>
#endif

#include <boost/algorithm/clamp.hpp>
#include <boost/scope_exit.hpp>

#include "quantizer.hpp"
#include "utils.hpp"

namespace aspect { namespace image {

inline image_rect clamped_rect(image_rect const& rect, image_size const& size)
{
	using boost::algorithm::clamp;

	return image_rect(
		clamp(rect.left, 0, size.width),
		clamp(rect.top, 0, size.height),
		clamp(rect.width, 0, size.width - rect.left),
		clamp(rect.height, 0, size.height - rect.top)
	);
}

static int libpng_color_type(png_color_type color_type)
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
		_aspect_assert(false && "unknown png_color_type");
		return -1;
	}
}

// -- callbacks
static void png_write_file(png_struct* png, png_byte* data, png_size_t size)
{
	buffer& result = *reinterpret_cast<buffer*>(png_get_io_ptr(png)); // user data
	
	size_t const pos = result.size();
	result.resize(pos + size);
	memcpy(&result[pos], data, size);
}

static void png_flush_file(png_struct*)
{
	// do nothing - used for flushing file i/o
}

std::string generate_png(bitmap const& image, buffer& result, image_rect rect,
	bool flip, int compression, png_color_type color_type)
{
	// TODO - do sanity checks! make sure rects are within the image etc..
	_aspect_assert(image.data() && !image.size().is_empty() && !rect.is_empty());
	_aspect_assert(image.bytes_per_pixel() == 4);
	// -- 

	rect = clamped_rect(rect, image.size());

	uint8_t const* const pixels = image.data();
	size_t const stride = rect.width * image.bytes_per_pixel();
	size_t const bytes_per_pixel = image.bytes_per_pixel();

	png_struct* png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info* info = png_create_info_struct(png);

	BOOST_SCOPE_EXIT(&png, &info)
	{
		png_destroy_info_struct(png, &info);
		png_destroy_write_struct(&png, &info);
	} 
	BOOST_SCOPE_EXIT_END

	_aspect_assert(png && info);
	png_set_write_fn(png, &result, &png_write_file, &png_flush_file);

	png_set_compression_level(png, compression);

	aspect::image::quantizer quantizer;

	int row_begin = rect.top;
	int row_end = rect.bottom();
	int row_step = 1;

	if (color_type == png_color_type::palette)
	{
		quantizer.quantize(pixels, stride, rect, 0xff);
		png_set_PLTE(png, info, (png_color*)quantizer.lut24(), 0xff);
		row_begin = 0;
		row_end = rect.height;
	}

	png_set_IHDR(png, info, rect.width, rect.height, 8, libpng_color_type(color_type),
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);
	png_set_packing(png);

	// WARNING - always check RGB vs BGR
	// png_set_bgr(ppng);
	// png_set_strip_alpha(ppng);

	if (flip)
	{
		std::swap(row_begin, row_end);
		--row_begin; --row_end;
		row_step = -1;
	}

	if (color_type == png_color_type::rgba)
	{
		for (int row = row_begin; row != row_end; row += row_step)
		{
			png_write_row(png, &pixels[(row*stride)+(rect.left * bytes_per_pixel)]);
		}
	}
	else if (color_type == png_color_type::rgb)
	{
		buffer buf(rect.width * 3);
		for (int row = row_begin; row != row_end; row += row_step)
		{
			uint8_t const* row_ptr = &pixels[(row * stride)+(rect.left * bytes_per_pixel)];
			uint8_t const* src = row_ptr;
			uint8_t* dst = &buf[0];
			for (int i = 0; i < rect.width; ++i)
			{
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0]; 
				// dst[3] = src[0];
				dst += 3; src += 4;
			}
			png_write_row(png, &buf[0]);
		}
	}
	else if (color_type == png_color_type::palette)
	{
		// quantizer generates index data already in the desired resolution, just store it
		for (int row = row_begin; row != row_end; row += row_step)
		{
			png_write_row(png, quantizer.result_data() + row * rect.width);
		}
	}

	png_write_end(png, NULL);

	return "image/png";
}

#if HAVE(JPEG_SUPPORT)
void bitmap::generate_jpeg(bitmap const& image, buffer& result, image_rect rect, bool flip, int quality)
{
	rect = clamped_rect(rect, image.size());

	uint8_t const* const pixels = image.data();
	size_t const stride = rect.width * image.bytes_per_pixel();
	size_t const bytes_per_pixel = image.bytes_per_pixel();

	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;
	unsigned char* buf_data = NULL;
	unsigned long  buf_size = 0;

	BOOST_SCOPE_EXIT(&cinfo, &buf_data)
	{
		jpeg_destroy_compress(&cinfo);
		free(buf_data);
	}
	BOOST_SCOPE_EXIT_END

	// Step 1: allocate and initialize JPEG compression objects
	cinfo.err = jpeg_std_error(&jerr);
	//jerr.error_exit = GAPI_JpegErrorHandler;
	//jerr.output_message = GAPI_JpegWarningMessage;
	jpeg_create_compress(&cinfo);

	// Step 2: specify data destination (eg, a file)
	// asy - compressing in memory
	jpeg_mem_dest(&cinfo, &buf_data, &buf_size);

	// Step 3: set parameters for compression
	cinfo.image_width			= rect.width;		// image width and height, in pixels
	cinfo.image_height			= rect.height;
	cinfo.input_components		= 3;				// # of color components per pixel
	cinfo.in_color_space		= JCS_RGB; 			// colorspace of input image

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
	//GAPI_Color32 **ppData = ppGetData();
	buffer line_buf(3 *cinfo.image_width);
	while (cinfo.next_scanline < cinfo.image_height)
	{
		// !!!TODO: Check for cancel
		int y = cinfo.next_scanline;
		unsigned char* buf = &line_buf[0];
		for (int x = 0; x!=(int)cinfo.image_width; ++x)
		{
//			gl::color24 *ptr = (gl::color24 *)(data+((y*get_width()+x)*bpp));
			uint8_t const* ptr = pixels + ( (y + rect.top) * stride + (rect.left + x) * bytes_per_pixel);
			buf[0] = ptr[2];
			buf[1] = ptr[1];
			buf[2] = ptr[0];
			buf +=3;
		}
		uint8_t* line = &line_buf[0];
		jpeg_write_scanlines(&cinfo, &line, 1);
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
#endif

std::string generate_bmp(bitmap const& image, buffer& result, image_rect rect, bool flip)
{
#if OS(WINDOWS)
	rect = clamped_rect(rect, image.size());

	size_t const offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	size_t const file_size = offset + (rect.width * rect.height * 4);

	BITMAPFILEHEADER hdr = {};
	hdr.bfType = 'B'+('M'<<8);
	hdr.bfOffBits = static_cast<DWORD>(offset);
	hdr.bfSize = static_cast<DWORD>(file_size);

	BITMAPINFOHEADER info = {};
	info.biSize = sizeof(BITMAPINFOHEADER);
	info.biBitCount = 32;
	info.biPlanes = 1;
	info.biCompression = BI_RGB;
	info.biWidth = rect.width;
	info.biHeight = rect.height;

	result.resize(file_size);
	uint8_t* ptr = &result[0];

	memcpy(ptr, &hdr, sizeof(hdr));
	ptr += sizeof(hdr);
	
	memcpy(ptr, &info, sizeof(info));
	ptr += sizeof(info);

	uint8_t const* pixels = image.data();
	size_t const stride = rect.width * image.bytes_per_pixel();

	int row_begin = rect.top;
	int row_end = rect.bottom();
	int row_step = 1;
	if (flip)
	{
		std::swap(row_begin, row_end);
		--row_begin; --row_end;
		row_step = -1;
	}
	for (int row = row_begin, iY = rect.height - 1; row != row_end; row += row_step, --iY)
	{
		uint8_t const* row_ptr = &pixels[(row * stride)+(rect.left * 4)];
		uint8_t* dst = ptr + (iY * rect.width * 4);
		memcpy(dst, row_ptr, rect.width * 4);
	}

	return "image/bmp";
#else
#warning No BMP generation yet
	return "none/none";
#endif
}

}} // namespace aspect::image
