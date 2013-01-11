#ifndef IMAGE_HPP_INCLUDED
#define IMAGE_HPP_INCLUDED

#include "core.hpp"
#include "types.hpp"
#include "geometry.hpp"
#include "v8_core.hpp"
#include "shared_ptr_object.hpp"
#include "math.hpp"

#if OS(WINDOWS)
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

namespace aspect { namespace image2 {

enum encoding
{
	UNKNOWN,
	YUV8,
	YUV10,
	A8,
	RGBA8,
	ARGB8,
	BGRA8,
	RGB10,
	RGB32F
};

//template<class _Ax = std::allocator<uint8_t>>
class IMAGE_API bitmap : boost::noncopyable
{
	public:
		explicit bitmap()
			: pixel_format_(UNKNOWN)
		{
		}

		explicit bitmap(unsigned width, unsigned height, encoding pixel_format = BGRA8)
		{
			resize(width, height,pixel_format);
		}

		~bitmap()
		{
			total_memory_ -= data_.size();
		}

		void resize(unsigned width, unsigned height, encoding pixel_format)
		{
			boost::unique_lock<boost::shared_mutex> lock(shared_mutex_);
			if(width == size_.width && height == size_.height && pixel_format_ == pixel_format)
				return;
			//		boost::lock_guard<boost::shared_mutex> lock(shared_mutex_);
			total_memory_ -= data_.size();

			pixel_format_ = pixel_format;
			data_.resize(width * height * get_bytes_per_pixel());
			total_memory_ += data_.size();
			size_.width = width;
			size_.height = height;
			memset(data(),0,data_size());
		} 

		image_size size() const { return size_; } 
		uint32_t width() const { return size_.width; }
		uint32_t height() const { return size_.height; }
		encoding get_pixel_format() const { return pixel_format_; }
		uint32_t get_bytes_per_pixel() const 
		{  
			switch(pixel_format_)
			{
				case YUV8: return 2;
				case A8:		return 1;
				default:			return 4; // all other formats use 4 bytes per pixel
			}
		}
		uint32_t get_row_bytes() const { return width() * get_bytes_per_pixel(); }

		uint8_t const* data() const { return data_.empty()? NULL : &data_[0]; }
		uint8_t* data() { return data_.empty()? NULL : &data_[0]; }

		size_t data_size() const { return data_.size(); }

		boost::shared_mutex& shared_mutex() { return shared_mutex_; }

		/// Create checkerboard with lines between checkers
		void checker3(const uint32_t c1 = 0x20202020, const uint32_t c2 = 0x40404040, const uint32_t c3 = 0x80808080);

		/// Create checkerboard without lines between checkers
		void checker2(const uint32_t c1 = 0x00000000, const uint32_t c2 = 0xffffffff);


	private:
		boost::shared_mutex shared_mutex_;	

		image_size size_;
		//	buffer data_;
		std::vector<uint8_t, aligned_allocator<uint8_t, 32>> data_;
		//	std::vector<uint8_t, _Ax> data_;
		encoding	pixel_format_;

		static uint64_t total_memory_;
};

typedef boost::shared_ptr<bitmap> shared_bitmap;


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
		{ }

		// 		shared_bitmap_container(bitmap* color)
		// 			: color_(shared_bitmap(color))
		// 		{ }
		// 
		shared_bitmap_container(shared_bitmap& color, uint32_t flags)
			: color_(color), flags_(flags)
		{ }

		// 		shared_bitmap_container(bitmap* color, bitmap* alpha)
		// 			: color_(sha), alpha_(alpha)
		// 		{ }
		// 
		shared_bitmap_container(shared_bitmap& color, shared_bitmap& alpha, uint32_t flags)
			: color_(color), alpha_(alpha), flags_(flags)
		{ }

		void set_color(shared_bitmap& color) { color_ = color; }
		shared_bitmap& get_color(void) { return color_; }
		void set_alpha(shared_bitmap& alpha) { alpha_ = alpha; }
		shared_bitmap& get_alpha(void) { return alpha_; }
		bool has_alpha(void) const { return alpha_.get() ? true : false; }
		uint32_t get_flags(void) const { return flags_; }
		void set_flags(uint32_t flags) { flags_ = flags; }

	private:

		shared_bitmap	color_;	// source data
		shared_bitmap	alpha_;	// separate alpha (if available)
		//		shared_bitmap	aux_;	// auxiliary (converted 
		uint32_t		flags_;
};

class IMAGE_API device : public shared_ptr_object<device>
{
	public:

		V8_DECLARE_CLASS_BINDER(device);

		device(const char *name = "N/A")
			: name_(name), dropped_frames_(0)
		{
		}

		virtual ~device()
		{
		}

		virtual bool acquire_input_frame(boost::shared_ptr<shared_bitmap_container> & frame) { return capture_queue_.try_pop(frame); }
		virtual void acquire_input_frame_blocking(boost::shared_ptr<shared_bitmap_container> & frame) { return capture_queue_.wait_and_pop(frame); }
		virtual void release_input_frame(boost::shared_ptr<shared_bitmap_container> & frame) { available_queue_.push(frame); }
		virtual void schedule_output_frame(boost::shared_ptr<shared_bitmap_container> & ) { _aspect_assert(false && "aspecet::image::device::schedule_output_frame() is not overloaded"); }
		virtual void schedule_input_frame(boost::shared_ptr<shared_bitmap_container> & frame, bool drop_frames);

		encoding get_encoding(void) const { return encoding_; }
		void set_encoding(encoding encoding) { encoding_ = encoding; }

		//		threads::concurrent_queue<boost::shared_ptr<bitmap>>& get_capture_queue(void) { return capture_queue_; }
		threads::concurrent_queue<boost::shared_ptr<shared_bitmap_container>>& get_capture_queue(void) { return capture_queue_; }

		uint32_t get_dropped_frames(void) { return dropped_frames_; }

		virtual v8::Handle<v8::Value> get_info(void);

	protected:

		std::string name_;

		threads::concurrent_queue<boost::shared_ptr<shared_bitmap_container>>	capture_queue_;
		threads::concurrent_queue<boost::shared_ptr<shared_bitmap_container>>	available_queue_;

		encoding encoding_;

		uint32_t dropped_frames_;
};

extern IMAGE_API std::vector<boost::shared_ptr<device>>	g_devices;

} } // aspect::image


#define WEAK_CLASS_TYPE aspect::image2::device
#define WEAK_CLASS_NAME __image_device_interface
#include <v8/juice/WeakJSClassCreator-Decl.h>

#endif // IMAGE_HPP_INCLUDED
