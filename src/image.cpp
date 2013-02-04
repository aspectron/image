#include "image.hpp"
#include "library.hpp"

using namespace v8;
using namespace v8::juice;
using namespace v8::juice::convert;
using namespace aspect;
using namespace aspect::v8_core;

// DECLARE_LIBRARY_ENTRYPOINTS(image_install, image_uninstall);

V8_IMPLEMENT_CLASS_BINDER(aspect::image::device, device);

/*

TODO - invoke from dependent libraries
set global "initialized" state - init only once
destroy in dependent libraries
maintain reference count.

do this via shared pointer?  so that when 0 references the object is destroyed?

aspect::image::init(target);
aspect::image::cleanup();

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  INSTALL IMAGE LIBRARY INTO v8 GLOBAL OBJECT AUTOMATICALLY
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  THIS WAY WE DO NOT NEED KEEP/PASS TARGET!


class local_initializer
{
	public:

		local_initializer();
		~local_initializer();
};

static local_initializer g_local_initializer;
*/

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
	using namespace aspect::image;

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
	V8_DESTROY_CLASS_BINDER(aspect::image::device);
}

namespace v8 { namespace juice {

	V8_DECLARE_WEAK_CLASS_CTOR(aspect::image::device, args, exceptionText)
	{
		throw std::runtime_error("only derived classes may be created");
	}

	V8_DECLARE_WEAK_CLASS_DTOR(aspect::image::device, o)
	{
		o->release();
	}
} } // v8::juice

namespace aspect { namespace image {

uint32_t g_reference_count = 0;

void init(void)
{
	using namespace aspect::image;

	g_reference_count++;
	if(g_reference_count > 1)
		return;

	Handle<Object> target = v8::Context::GetCurrent()->Global();

	ClassBinder<device>* binder = new ClassBinder<device>(target);
	V8_SET_CLASS_BINDER(device, binder);
	(*binder)
		.BindMemFunc<uint32_t, &device::get_dropped_frames>("get_dropped_frames")
		.Seal();
}

void cleanup(void)
{
	g_reference_count--;
	if(g_reference_count)
		return;

//	Handle<Object> target = v8::Context::GetCurrent()->Global();
	V8_DESTROY_CLASS_BINDER(device);
}

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
//	aspect::v8_core::register_v8_binding image_device_bindings_init(&aspect::image::image_device_bindings);


void device::schedule_input_frame( boost::shared_ptr<shared_bitmap_container> & frame, bool drop_frames)
{

	capture_queue_.push(frame);

	// clear up the queue so that at most we have 2 pending frames
	if(drop_frames)
	{
		while(capture_queue_.size() > 2)
		{
			if(trace_dropped_frames_)
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



#if OS(WINDOWS)

// sanity check
#pragma warning ( disable : 4702 ) // unreachable code
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  fdwReason, LPVOID)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_DETACH:
			{
				_aspect_assert(aspect::image::g_reference_count == 0 && "image library was not terminated correctly");
			}
			break;
	}
	return TRUE;
}

#endif

