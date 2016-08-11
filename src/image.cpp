#include "image/image.hpp"

#include <node.h>

#include <v8pp/module.hpp>
#include <v8pp/class.hpp>

namespace aspect { namespace image {

// https://github.com/ofTheo/videoInput/blob/master/videoInputSrcAndDemos/libs/videoInput/videoInput.cpp
// http://www.codeproject.com/Articles/12869/Real-time-video-image-processing-frame-grabber-usi


static void init(v8::Handle<v8::Object> exports, v8::Handle<v8::Object> module)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8pp::module image_module(isolate);

	v8pp::class_<device> device_class(isolate);
	device_class
		.set("get_dropped_frames", &device::get_dropped_frames)
		;
	image_module.set("__image_device_interface", device_class);
	
	exports->SetPrototype(image_module.new_instance());
}
NODE_MODULE(image, init);

size_t bitmap::total_memory_ = 0;

enum grid_type { GRID_LINE, GRID_1, GRID_2 };

static inline grid_type get_grid(int x, int y)
{
	if ( (x % 8 == 0) || (y % 8 == 0) ) return GRID_LINE;
	else if ( ((x / 8) + (y / 8)) % 2 ) return GRID_1;
	else                                return GRID_2;
}

void bitmap::checker3(const uint32_t c1, const uint32_t c2, const uint32_t c3)
{
	uint32_t* tmp = reinterpret_cast<uint32_t*>(data());

	for (int y = 0; y < size_.height; ++y)
	{
		for (int x = 0; x < size_.width; ++x, ++tmp)
		{
			switch( get_grid(x,y) )
			{
			case GRID_LINE: *tmp=c3; break;
			case GRID_1:    *tmp=c1; break;
			case GRID_2:    *tmp=c2; break;
			}
		}
	}
}

void bitmap::checker2(uint32_t c1, uint32_t c2)
{
	uint32_t* tmp = reinterpret_cast<uint32_t*>(data());
	for (int y=0; y < size_.height; ++y)
	{
		for (int x=0; x < size_.width; ++x, ++tmp)
		{
			*tmp = ((x>>3)^(y>>3) & 1? c1 : c2);
		}
	}
}

void device::schedule_input_frame(shared_bitmap_container const& frame, bool drop_frames)
{
	capture_queue_.push(frame);

	// clear up the queue so that at most we have 2 pending frames
	if (drop_frames)
	{
		while (capture_queue_.size() > 2)
		{
			if (trace_dropped_frames_)
			{
				printf("image::device \"%s\" dropping input frame\n", name_.c_str());
			}
			shared_bitmap_container container;
			capture_queue_.try_pop(container);
			++dropped_frames_;
		}
	}
}

v8::Handle<v8::Value> device::get_info(v8::Isolate* isolate) const
{
	v8::EscapableHandleScope scope(isolate);

	v8::Local<v8::Object> o = v8::Object::New(isolate);
	v8pp::set_option(isolate, o, "dropped_frames", dropped_frames_);

	return scope.Escape(o);
}

}} // aspect::image
