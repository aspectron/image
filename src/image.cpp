#include "image.hpp"
#include "library.hpp"

using namespace v8;
using namespace v8::juice;
using namespace v8::juice::convert;
using namespace aspect;
using namespace aspect::v8_core;

DECLARE_LIBRARY_ENTRYPOINTS(image_install, image_uninstall);

V8_IMPLEMENT_CLASS_BINDER(aspect::image2::device, device);

void image_install(Handle<Object> target)
{

	// https://github.com/ofTheo/videoInput/blob/master/videoInputSrcAndDemos/libs/videoInput/videoInput.cpp

	// http://www.codeproject.com/Articles/12869/Real-time-video-image-processing-frame-grabber-usi

	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	using namespace aspect::image2;

	ClassBinder<device>* binder = new ClassBinder<device>(target);
	V8_SET_CLASS_BINDER(device, binder);
	(*binder)
		.BindMemFunc<uint32_t, &device::get_dropped_frames>("get_dropped_frames")
		.Seal();


	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!
	// MOVE THIS TO DLLMAIN!

}

void image_uninstall(Handle<Object> target) 
{
	V8_DESTROY_CLASS_BINDER(aspect::image2::device);
}

namespace v8 { namespace juice {

	V8_DECLARE_WEAK_CLASS_CTOR(aspect::image2::device, args, exceptionText)
	{
		throw std::runtime_error("only derived classes may be created");
	}

	V8_DECLARE_WEAK_CLASS_DTOR(aspect::image2::device, o)
	{
		o->release();
	}
} } // v8::juice

namespace aspect { namespace image2 {

	std::vector<boost::shared_ptr<device>>	g_devices;
	uint64_t bitmap::total_memory_ = 0;

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

	// ----------------
/*
	void image_device_bindings(binding_list &list, Handle<Object> global)
	{
		ClassBinder<device>* binder = new ClassBinder<device>(global);
		list.push_back(binder);
		V8_SET_CLASS_BINDER(device, binder);
		(*binder)
			.BindMemFunc<uint32_t, &device::get_dropped_frames>("get_dropped_frames")
			.Seal();

		// 	binder_device = new ClassBinder<device>(target);
		// 	V8_SET_CLASS_BINDER(device, binder_device);
		// 	(*binder_device)
		// 		.Seal();

	}
*/
//	aspect::v8_core::register_v8_binding image_device_bindings_init(&aspect::image2::image_device_bindings);


	void device::schedule_input_frame( boost::shared_ptr<shared_bitmap_container> & frame, bool drop_frames)
	{

		capture_queue_.push(frame);

		// clear up the queue so that at most we have 2 pending frames
		if(drop_frames)
		{
			while(capture_queue_.size() > 2)
			{
				printf("image::device \"%s\" dropping input frame\n",name_.c_str());
				boost::shared_ptr<shared_bitmap_container> container;
				capture_queue_.try_pop(container);
				dropped_frames_++;
			}
		}
	}

	Handle<Value> device::get_info(void)
	{
		HandleScope scope;

		Handle<Object> o = Object::New();
		o->Set(String::NewSymbol("dropped_frames"), convert::UInt32ToJS(dropped_frames_));

		return scope.Close(o);
	}

} } // aspect::image