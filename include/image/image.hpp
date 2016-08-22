#ifndef IMAGE_HPP_INCLUDED
#define IMAGE_HPP_INCLUDED

#include "nitrogen/aligned_allocator.hpp"
#include "nitrogen/geometry.hpp"
#include "nitrogen/threads.hpp"

#include <shared_mutex>

#if _MSC_VER
//	#pragma warning ( disable : 4251 )
#if defined(IMAGE_EXPORTS)
#define IMAGE_API __declspec(dllexport)
#else
#define IMAGE_API __declspec(dllimport)
#endif
#elif __GNUC__ >= 4
# define IMAGE_API __attribute__((visibility("default")))
#else
#define IMAGE_API // nothing, symbols in a shared library are exported by default
#endif

namespace aspect { namespace image {

typedef std::vector<uint8_t> buffer;
typedef std::shared_ptr<buffer> shared_buffer;

/// Image pixel format
enum encoding
{
	UNKNOWN,
	YUV8,
	YUV10,
	A8,
	RGBA8,
	ARGB8,
	BGRA8,
	RGB8,
	RGB10,
	RGB32F
};

/// Bitmap image with specified size and pixel format
class IMAGE_API bitmap
{
public:
	bitmap() : pixel_format_(UNKNOWN) {}
	bitmap(bitmap const&) = delete;
	bitmap& operator=(bitmap const&) = delete;

	/// Create a bitmap with specified size and pixel format
	bitmap(image_size const& size, encoding pixel_format = BGRA8)
	{
		resize(size, pixel_format);
	}

	~bitmap()
	{
		total_memory_ -= data_.size();
	}

	/// Resize bitmap and change pixel format
	void resize(image_size const& size, encoding pixel_format)
	{
		std::unique_lock<std::shared_mutex> lock(shared_mutex_);

		if (size != size_ || pixel_format_ == pixel_format)
		{
			total_memory_ -= data_.size();
			data_.resize(size.width * size.height * bytes_per_pixel(pixel_format), 0);
			total_memory_ += data_.size();

			size_ = size;
			pixel_format_ = pixel_format;
		}
	}

	/// Resize bitmap
	void resize(image_size const& size)
	{
		resize(size, pixel_format_);
	}

	image_size const& size() const { return size_; }
	encoding pixel_format() const { return pixel_format_; }
	size_t bytes_per_pixel() const { return bytes_per_pixel(pixel_format_); }

	static size_t bytes_per_pixel(encoding pixel_format)
	{
		switch (pixel_format)
		{
			case YUV8: return 2;
			case A8:   return 1;
			case RGB8: return 3;
			default:   return 4; // all other formats use 4 bytes per pixel
		}
	}

	size_t row_bytes() const { return size_.width * bytes_per_pixel(); }

	uint8_t const* data() const { return data_.empty()? nullptr : &data_[0]; }
	uint8_t* data() { return data_.empty()? nullptr : &data_[0]; }

	size_t data_size() const { return data_.size(); }

	std::shared_mutex& shared_mutex() { return shared_mutex_; }

	/// Create checkerboard with lines between checkers
	void checker3(const uint32_t c1 = 0x20202020, const uint32_t c2 = 0x40404040, const uint32_t c3 = 0x80808080);

	/// Create checkerboard without lines between checkers
	void checker2(const uint32_t c1 = 0x00000000, const uint32_t c2 = 0xffffffff);

private:
	std::shared_mutex shared_mutex_;

	image_size size_;
	std::vector<uint8_t, aligned_allocator<uint8_t, 32>> data_;
	encoding pixel_format_;

	static size_t total_memory_;
};

typedef std::shared_ptr<bitmap> shared_bitmap;

class IMAGE_API shared_bitmap_container
{
public:

	enum flags {
		DEFAULT	= 0x00000000,		// uninitialized
		INPUT	= 0x00000001,		// belongs to input devices
		OUTPUT	= 0x00000002,		// belongs to output devices
		LOCAL	= 0x00000004		// for local use
	};

	shared_bitmap_container()
		: flags_(DEFAULT)
	{
	}

	shared_bitmap_container(shared_bitmap color, uint32_t flags)
		: color_(color)
		, flags_(flags)
	{
	}

	shared_bitmap_container(shared_bitmap color, shared_bitmap alpha, uint32_t flags)
		: color_(color)
		, alpha_(alpha)
		, flags_(flags)
	{
	}

	void set_color(shared_bitmap color) { color_ = color; }
	shared_bitmap color() const { return color_; }

	void set_alpha(shared_bitmap alpha) { alpha_ = alpha; }
	shared_bitmap alpha() const { return alpha_; }

	uint32_t flags() const { return flags_; }
	void set_flags(uint32_t flags) { flags_ = flags; }

private:
	shared_bitmap color_;	// source data
	shared_bitmap alpha_;	// separate alpha (if available)
	uint32_t      flags_;
};

class IMAGE_API device
{
public:
	explicit device(char const* name = "N/A")
		: name_(name? name : "")
		, encoding_(UNKNOWN)
		, dropped_frames_(0)
		, trace_dropped_frames_(false)
	{
	}

	virtual ~device()
	{
	}

	encoding get_encoding() const { return encoding_; }
	void set_encoding(encoding encoding) { encoding_ = encoding; }

	uint32_t get_dropped_frames() const { return dropped_frames_; }

	virtual v8::Handle<v8::Value> get_info(v8::Isolate* isolate) const;

	virtual bool acquire_input_frame(shared_bitmap_container& frame) { return capture_queue_.try_pop(frame); }
	virtual void acquire_input_frame_blocking(shared_bitmap_container& frame) { return capture_queue_.wait_and_pop(frame); }
	virtual void release_input_frame(shared_bitmap_container const& frame) { available_queue_.push(frame); }
	virtual void schedule_output_frame(shared_bitmap_container const&) = 0;
	virtual void schedule_input_frame(shared_bitmap_container const& frame, bool drop_frames);

protected:
	std::string name_;
	encoding encoding_;
	uint32_t dropped_frames_;
	bool trace_dropped_frames_;

	threads::concurrent_queue<shared_bitmap_container> capture_queue_;
	threads::concurrent_queue<shared_bitmap_container> available_queue_;
};

}} // aspect::image

#endif // IMAGE_HPP_INCLUDED
