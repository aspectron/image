#ifndef WCAPPROC_BITMAP_HPP_INCLUDED
#define WCAPPROC_BITMAP_HPP_INCLUDED

#define HAVE_JPEG_SUPPORT 0

#include <openssl/sha.h>

#include <zlib.h>
#include <png.h>
#if HAVE(JPEG_SUPPORT)
#include <jpeglib.h>
#endif


#include "geometry.hpp"
#include "types.hpp"

namespace aspect {
namespace codecs {

class IMAGE_API bitmap : boost::noncopyable
{
public:
	/// Create bitmap generator for the image rect
	bitmap(uint8_t const* pixels, unsigned bytes_per_pixel,
		image_size const& size, image_rect const& rect, bool generate_hash);

	image_size const& size() const { return size_; }

	/// Compresses rect into png image
	void generate_png(int compression = Z_BEST_SPEED, int color_format = PNG_COLOR_TYPE_PALETTE);

#if HAVE(JPEG_SUPPORT)
	/// Compresses rect into jpeg image
	void generate_jpeg(int quality);
#endif

	/// Compresses rect into bitmap image
	void generate_bmp();

	/// Compressed result data
	shared_buffer const& data() const { return data_; }

	/// Compressed image MIME type
	string const& mime_type() const { return mime_type_; }

	/// Data hash
	uint8_t const* hash() const { return hash_; }
	size_t hash_size() const { return sizeof hash_; }

	/// Data hash in a hex string
	std::string hash_string() const;

	/// Create checkerboard with lines between checkers
	void checker3(const uint32_t c1 = 0x20202020, const uint32_t c2 = 0x40404040, const uint32_t c3 = 0x80808080);

	/// Create checkerboard without lines between checkers
	void checker2(const uint32_t c1 = 0x00000000, const uint32_t c2 = 0xffffffff);

private:
	uint8_t const* pixels_;
	unsigned bytes_per_pixel_;
	image_size size_;
	image_rect rect_;

	shared_buffer data_;
	bool generate_hash_;
	uint8_t hash_[SHA_DIGEST_LENGTH];

	string mime_type_;
};

class IMAGE_API rescaler : boost::noncopyable
{
private:

	template<uint32_t N>
	float get(int x,int y);

	template<uint32_t N>
	void set(int x,int y,float val);

	void set(int x, int y, float v1, float v2, float v3, float v4);

	template<uint32_t N>
	float integrate(float p,int q,float scale,int dir);

	template<uint32_t N>
	float lint(float p,int q,int dir);

	void resize_nearest(int xwidth,int ywidth,float xpos,float ypos,float xscale,float yscale);
	void resize_bilinear(int xwidth,int ywidth,float xpos,float ypos,float xscale,float yscale);
	void resize_bicubic(int xwidth,int ywidth,float xpos,float ypos,float xscale,float yscale);

	void alloc(int xsize,int ysize,bool reassign);
	void dealloc();

	uint8_t* data_orig_;
	uint8_t* data_result_;

	int src_stride_;
	int dst_stride_;

	int src_width_, src_height_;
	int dst_width_, dst_height_;

	bool owns_data_orig_;

public:

	enum
	{
		NEAREST,
		BILINEAR,
		BICUBIC
	};

	rescaler();
	~rescaler();

	/// Rescale
	//
	// note that the values xpos,ypos,xscale,yscale are in logical image coordinate
	// ie scale 1.0 is size of final image
	// pos 0.0 is centre of final image, 1.0 is a shift of half the image
	void rescale(uint8_t* pixels, image_size const& src_size, int mode, image_size const& dst_size,
		float xpos = 0.0f,float ypos = 0.0f,float xscale = 1.0f,float yscale = 1.0f);

	uint8_t const* pixels() const { return data_result_; }
	image_size size() const { return image_size(dst_width_, dst_height_); }
};

} } // namespace aspect::codecs


#endif // WCAPPROC_BITMAP_HPP_INCLUDED
