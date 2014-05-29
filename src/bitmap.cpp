#include "image.hpp"
#include "bitmap.hpp"

#include <boost/algorithm/clamp.hpp>

#include "quantizer.hpp"
#include "utils.hpp"

namespace aspect {
namespace codecs {

/////////////////////////////////////////////////////////////////////////////
//
// PNG image writer
//
class png_writer : boost::noncopyable
{
public:
	/// Create writer to write image rect to output buffer data
	png_writer(shared_buffer data, uint8_t* hash);

	~png_writer();

	void encode(image_rect const& rect, uint8_t const* pixels, unsigned bytes_per_pixel, int compression, int color_type);

private:
	// -- callbacks
	static void png_write_file(png_structp ppng, png_bytep data_to_write, png_size_t data_size);
	static void png_flush_file(png_structp ppng);

	png_structp png_;
	png_infop info_;
	
	shared_buffer data_;
	uint8_t* hash_;

	SHA_CTX sha_ctx_;
};

png_writer::png_writer(shared_buffer data, uint8_t* hash)
	: png_()
	, info_()
	, data_(data)
	, hash_(hash)
{
	png_ = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ = png_create_info_struct(png_);
	_aspect_assert(png_ && info_);

	png_set_write_fn(png_, this, &png_write_file, &png_flush_file);

	if ( hash_ )
	{
		SHA_Init(&sha_ctx_);
	}
}

png_writer::~png_writer()
{
	png_destroy_info_struct(png_, &info_);
	png_destroy_write_struct(&png_, &info_);

	if ( hash_ )
	{
		SHA_Final(hash_, &sha_ctx_);
	}
}

void png_writer::encode(image_rect const& rect, uint8_t const* pixels, unsigned bytes_per_pixel,
		int compression, int color_type)
{
	unsigned const stride = rect.width * bytes_per_pixel;

	png_set_compression_level(png_, compression);

	aspect::quantizer quantizer;

	// if pa
	if ( color_type == PNG_COLOR_TYPE_PALETTE )
	{
		quantizer.quantize(pixels, stride, rect, 0xff);
		png_set_PLTE(png_, info_, (png_colorp)quantizer.lut24(), 0xff);
	}

	png_set_IHDR(png_, info_, rect.width, rect.height,
		8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	png_write_info(png_, info_);
	png_set_packing(png_);

	// WARNING - always check RGB vs BGR
	// png_set_bgr(ppng);  
	// png_set_strip_alpha(ppng);

	if ( color_type == PNG_COLOR_TYPE_RGBA )
	{
		for (int row = rect.top, bottom = rect.bottom(); row < bottom; ++row)
		{
			png_write_row(png_, const_cast<png_bytep>(&pixels[(row*stride)+(rect.left * bytes_per_pixel)]));
		}
	}
	else if ( color_type == PNG_COLOR_TYPE_RGB )
	{
		::buffer buf(rect.width * 3);
		for (int row = rect.top, bottom = rect.bottom(); row < bottom; ++row)
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
			png_write_row(png_, &buf[0]);
		}
	}
	else if ( color_type == PNG_COLOR_TYPE_PALETTE )
	{
		// quantizer generates index data already in the desired resolution, just store it
		for (int row = 0; row < rect.height; ++row)
		{
			png_write_row(png_, quantizer.result_data() + row * rect.width);
		}
	}

	png_write_end(png_, NULL);
}

void png_writer::png_write_file(png_structp ppng, png_bytep data_to_write, png_size_t data_size)
{
	png_writer* this_ = reinterpret_cast<png_writer*>(png_get_io_ptr(ppng)); // user data
	
	shared_buffer& data = this_->data_;
	
	size_t const pos = data->size();
	data->resize(pos + data_size);
	memcpy(&(*data)[pos], data_to_write, data_size);
	if ( this_->hash_ )
	{
		SHA_Update(&this_->sha_ctx_, data_to_write, data_size);
	}
}

void png_writer::png_flush_file(png_structp ppng)
{
	// do nothing - used for flushing file i/o
	ppng;
}


#if HAVE(JPEG_SUPPORT)
/////////////////////////////////////////////////////////////////////////////
//
// JPEG image writer
//
class jpeg_writer : boost::noncopyable
{
public:
	/// Create writer to write image rect to output buffer data
	jpeg_writer(image_rect const& rect, shared_buffer data, uint8_t* hash);

	~jpeg_writer();

	/// Begin image write
	void begin(int quality);

	/// Add data
	void add(uint8_t const* pixels, unsigned bytes_per_pixel);
	
	/// End image write
	void end();
private:
	jpeg_compress_struct cinfo_;
	jpeg_error_mgr jerr_;

	unsigned char* buf_data_;
	unsigned long  buf_size_;

	image_rect rect_;
	shared_buffer data_;
	uint8_t* hash_;
};

jpeg_writer::jpeg_writer(image_rect const& rect, shared_buffer data, uint8_t* hash)
	: rect_(rect)
	, data_(data)
	, hash_(hash)
{
	// Step 1: allocate and initialize JPEG compression objects
	cinfo_.err = jpeg_std_error(&jerr_);
	//jerr.error_exit = GAPI_JpegErrorHandler;
	//jerr.output_message = GAPI_JpegWarningMessage;
	jpeg_create_compress(&cinfo_);

	// Step 2: specify data destination (eg, a file)
	// asy - compressing in memory
	buf_data_ = NULL;
	buf_size_ = 0;
	jpeg_mem_dest(&cinfo_, &buf_data_, &buf_size_);
}

jpeg_writer::~jpeg_writer()
{
	jpeg_destroy_compress(&cinfo_);
}

void jpeg_writer::begin(int quality)
{
	// Step 3: set parameters for compression
	cinfo_.image_width			= rect_.width;		// image width and height, in pixels
	cinfo_.image_height			= rect_.height;
	cinfo_.input_components		= 3;				// # of color components per pixel
	cinfo_.in_color_space		= JCS_RGB; 			// colorspace of input image

	// Now use the library's routine to set default compression parameters.
	jpeg_set_defaults(&cinfo_);
	jpeg_set_quality(&cinfo_, quality, TRUE /* limit to baseline-JPEG values */);

	// Step 4: Start compressor
	// TRUE ensures that we will write a complete interchange-JPEG file
	jpeg_start_compress(&cinfo_, TRUE);
}

void jpeg_writer::add(uint8_t const* pixels, unsigned bytes_per_pixel)
{
	unsigned const stride = rect_.width * bytes_per_pixel;

	// Step 5: while (scan lines remain to be written)
	//           jpeg_write_scanlines(...); 
	//
	// Here we use the library's state variable cinfo.next_scanline as the
	// loop counter, so that we don't have to keep track ourselves.
	// To keep things simple, we pass one scanline per call; you can pass
	// more if you wish, though.
	//
	//GAPI_Color32 **ppData = ppGetData();
	::buffer line_buf(3 *cinfo_.image_width);
	while (cinfo_.next_scanline < cinfo_.image_height)
	{
		// !!!TODO: Check for cancel
		int y = cinfo_.next_scanline;
		unsigned char* buf = &line_buf[0];
		for (int x = 0; x!=(int)cinfo_.image_width; ++x) 
		{
//			gl::color24 *ptr = (gl::color24 *)(data+((y*get_width()+x)*bpp));
			uint8_t const* ptr = pixels + ( (y + rect_.top) * stride + (rect_.left + x) * bytes_per_pixel);
			buf[0] = ptr[2];
			buf[1] = ptr[1];
			buf[2] = ptr[0];
			buf +=3;
		}
		uint8_t* line = &line_buf[0];
		jpeg_write_scanlines(&cinfo_, &line, 1);
	}
}

void jpeg_writer::end()
{
	// Step 6: Finish compression
	jpeg_finish_compress(&cinfo_);

	// copy back the memory to the requester
	// TODO - make libjpeg populate existing/persistent buffer
	data_->resize(buf_size_);
	memcpy(&(*data_)[0], buf_data_, buf_size_);
	free(buf_data_);
	if ( hash_ )
	{
		SHA(data_->empty()? NULL : (uint8_t const*)&(*data_)[0], data_->size(), hash_);
	}
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// bitmap
//
bitmap::bitmap(uint8_t const* pixels, unsigned bytes_per_pixel,
		image_size const& size, image_rect const& rect, bool generate_hash)
	: pixels_(pixels)
	, bytes_per_pixel_(bytes_per_pixel)
	, size_(size)
	, rect_(rect)
	, data_(boost::make_shared< ::buffer >())
	, generate_hash_(generate_hash)
{
	rect_.left = boost::algorithm::clamp(rect_.left, 0, size_.width);
	rect_.top = boost::algorithm::clamp(rect_.top, 0, size_.height);
	rect_.width = boost::algorithm::clamp(rect_.width, 0, size_.width - rect_.left);
	rect_.height = boost::algorithm::clamp(rect_.height, 0, size_.height - rect_.top);
}

void bitmap::generate_png(int compression, int color_type)
{
	// TODO - do sanity checks! make sure rects are within the image etc..
	_aspect_assert(pixels_ && !size_.is_empty() && !rect_.is_empty());
	_aspect_assert(bytes_per_pixel_ == 4); 
	// -- 

	png_writer writer(data_, generate_hash_ ? hash_ : NULL);

//	int const compression = Z_BEST_SPEED; // 5;  Z_BEST_COMPRESSION
//	int const compression = Z_BEST_SPEED+4; // 5;  Z_BEST_COMPRESSION
//	int const color_type = PNG_COLOR_TYPE_PALETTE;
	//int const color_type = PNG_COLOR_TYPE_RGBA;
	//	int const color_type = PNG_COLOR_TYPE_RGB; // PNG_COLOR_TYPE_RGBA 
//	int color_type = PNG_COLOR_TYPE_RGB_ALPHA; // PNG_COLOR_TYPE_RGB
//	int const interlace_type = PNG_INTERLACE_NONE; // PNG_INTERLACE_ADAM7

	writer.encode(rect_, pixels_, bytes_per_pixel_, compression, color_type);

	mime_type_ = "image/png";
}

#if HAVE(JPEG_SUPPORT)
void bitmap::generate_jpeg(int quality)
{
	jpeg_writer writer(rect_, data_, generate_hash_ ? hash_ : NULL);
	
	writer.begin(quality);
	writer.add(pixels_, bytes_per_pixel_);
	writer.end();

	mime_type_ = "image/jpeg";
}
#endif

void bitmap::generate_bmp()
{
#if OS(WINDOWS)
	size_t const offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	size_t const file_size = offset + (rect_.width * rect_.height * 4);

	BITMAPFILEHEADER hdr = {};
	hdr.bfType = 'B'+('M'<<8);
	hdr.bfOffBits = static_cast<DWORD>(offset);
	hdr.bfSize = static_cast<DWORD>(file_size);

	BITMAPINFOHEADER info = {};
	info.biSize = sizeof(BITMAPINFOHEADER);
	info.biBitCount = 32;
	info.biPlanes = 1;
	info.biCompression = BI_RGB;
	info.biWidth = rect_.width;
	info.biHeight = rect_.height;

	data_->resize(file_size);
	uint8_t* ptr = &(*data_)[0];

	memcpy(ptr, &hdr, sizeof(hdr));
	ptr += sizeof(hdr);
	
	memcpy(ptr, &info, sizeof(info));
	ptr += sizeof(info);

	unsigned const stride = rect_.width * bytes_per_pixel_;
	for (int row = rect_.top, bottom = rect_.bottom(), iY = rect_.height - 1; row < bottom; ++row, --iY)
	{
		uint8_t const* row_ptr = &pixels_[(row * stride)+(rect_.left * 4)];
		uint8_t* dst = ptr + (iY * rect_.width * 4);
		memcpy(dst, row_ptr, rect_.width * 4);
	}

	mime_type_ = "image/bmp";
#else
#warning No BMP generation yet
#endif
}

std::string bitmap::hash_string() const
{
	_aspect_assert(generate_hash_);
	return generate_hash_? aspect::utils::hex_str(hash(), hash_size()) : "";
}

//////////////////////////////////////////////////////////////////////////

enum grid_type { GRID_LINE, GRID_1, GRID_2 };

static inline grid_type get_grid(int x, int y)
{
	if ( (x % 8 == 0) || (y % 8 == 0) ) return GRID_LINE;
	else if ( ((x / 8) + (y / 8)) % 2 ) return GRID_1;
	else                                return GRID_2;
}

void bitmap::checker3(const uint32_t c1, const uint32_t c2, const uint32_t c3)
{
	uint32_t* tmp = reinterpret_cast<uint32_t*>(&(*data_)[0]);

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
	uint32_t* tmp = reinterpret_cast<uint32_t*>(&(*data_)[0]);
	for (int y=0; y < size_.height; ++y)
	{
		for (int x=0; x < size_.width; ++x, ++tmp)
		{
			*tmp = ((x>>3)^(y>>3) & 1? c1 : c2);
		}
	}
}

///////////////////////////////////////////////////////////

#define _clip(a, min, max) (a < min ? min : (a > max ? max : a))

template<uint32_t N>
float rescaler::get(int x,int y)
{
	return data_orig_[y * src_stride_ + x * 4 + N];
}

template<uint32_t N>
void rescaler::set(int x,int y,float val)
{
	data_result_[y * dst_stride_ + x * 4 + N] = (uint8_t)val;
}

void rescaler::set(int x, int y, float v1, float v2, float v3, float v4)
{
	data_result_[y * dst_stride_ + x * 4 + 0] = (uint8_t)v1;
	data_result_[y * dst_stride_ + x * 4 + 1] = (uint8_t)v2;
	data_result_[y * dst_stride_ + x * 4 + 2] = (uint8_t)v3;
	data_result_[y * dst_stride_ + x * 4 + 3] = (uint8_t)v4;
}


#define XDIR 0
#define YDIR 1

template<uint32_t N>
float rescaler::integrate(float p,int q,float scale,int dir)
{
	int pos;
	float val,num=0.0f;
	float s=1.0f/scale;

	float minus=p-0.5f*s;
	float plus=p+0.5f*s;

	num=fabs(plus-minus);

	int start=(int)minus;
	int end=(int)plus;

	int res=src_width_;
	if( dir==YDIR ) res=src_height_;

	float f0,f1;
	if( minus < plus ) 
	{
		f0=1.0f-(minus-start);
		f1=plus-end;
	} 
	else 
	{
		f0=minus-start;
		f1=1.0f-(plus-end);
	}
	if( minus<0 ) { start=0;f0=0.0f; };
	if( plus<0 )  { end=0;f1=0.0f; };
	if( minus>=res ) { start=res-1;f0=0.0f; };
	if( plus>=res )  { end=res-1;f1=0.0f; };

	if( dir==XDIR )	val=get<N>(start,q)*f0+get<N>(end,q)*f1;
	else			val=get<N>(q,start)*f0+get<N>(q,end)*f1;

	if( abs(end-start)>1 ) {
		if( start>end) {
			int tmp=end;
			end=start;
			start=tmp;
		}
		for(pos=start+1;pos<end;pos++)
			if( dir==XDIR ) val+=get<N>(pos,q);
			else			val+=get<N>(q,pos);
	}
	return val/num;
}

template<uint32_t N>
float rescaler::lint(float p,int q,int dir)
{
	int res=src_width_;
	if(dir==YDIR) res=src_height_;
	if( p<0.0f || p>=res )
		return 0.0f;

	p-=0.5f;
	int p1=(int)p;
	if( p1<0 ) p1=0;
	int p2=(int)(p+1.0f);
	if( p2>=res ) p2=res-1;

	float f2=p-p1;
	float f1=1.0f-f2;

	if( dir==XDIR ) return get<N>(p1,q)*f1+get<N>(p2,q)*f2;
	else			return get<N>(q,p1)*f1+get<N>(q,p2)*f2;
}

static inline float _cubic(float x1,float a1,float a2,float a3,float a4)
{	
	float x2 = x1*x1;
	float x3 = x2*x1;

	return (0.5f*(x3-x2))*a4+(1.0f-2.5f*x2+1.5f*x3)*a2+(x2-0.5f*(x3+x1))*a1+(2.0f*x2+0.5f*(x1-3.0f*x3))*a3;
}


void rescaler::resize_nearest(int xwidth,int ywidth,float xpos,float ypos,float xscale,float yscale)
{
	short x,y;
	short oldx,oldy;

	int xp=(int)(src_width_*(1.0f+xpos-xscale)*65536.0f/(2.0f*xscale));
	int yp=(int)(src_height_*(1.0f+ypos-yscale)*65536.0f/(2.0f*yscale));
	xscale=xscale*(float)xwidth/src_width_;
	yscale=yscale*(float)ywidth/src_height_;

	int xf=(int)(65536.0f/xscale);
	int yf=(int)(65536.0f/yscale);

	alloc(xwidth,ywidth,false);

	for(y=0;y<ywidth;y++)
		for(x=0;x<xwidth;x++) 
		{
			oldx=(x*xf+(xf>>1)-xp)>>16;
			oldy=(y*yf+(xf>>1)-yp)>>16;
			if( oldx<0 || oldx>=src_width_ || oldy<0 || oldy>=src_height_ )
			{
				set(x,y,0.0f,0.0f,0.0f,0.0f);
			}
			else	
			{
				set(x,y,get<0>(oldx,oldy), get<1>(oldx,oldy), get<2>(oldx,oldy), get<3>(oldx,oldy));
			}
		}
}

void rescaler::resize_bilinear(int xwidth,int ywidth,float xpos,float ypos,float xscale,float yscale)
{
	short x,y;
	float fx,fy;

	xpos=xwidth*(1.0f+xpos-xscale)/2.0f-0.5f;
	ypos=ywidth*(1.0f+ypos-yscale)/2.0f-0.5f;
	xscale=xscale*(float)xwidth/src_width_;
	yscale=yscale*(float)ywidth/src_height_;

	alloc(xwidth,src_height_,false);

	if( fabs(xscale)<1.0f ) 
	{
		for(y=0;y<src_height_;y++)
			for(x=0;x<xwidth;x++) 
			{
				fx=(x-xpos)/xscale;
				set(x,y,integrate<0>(fx,y,xscale,XDIR), integrate<1>(fx,y,xscale,XDIR), integrate<2>(fx,y,xscale,XDIR), integrate<3>(fx,y,xscale,XDIR));
			}
	}
	else
	{
		for(y=0;y<src_height_;y++)
			for(x=0;x<xwidth;x++) 
			{
				fx=(x-xpos)/xscale;
				set(x,y,lint<0>(fx,y,XDIR), lint<1>(fx,y,XDIR), lint<2>(fx,y,XDIR), lint<3>(fx,y,XDIR));
			}
	}

	alloc(xwidth,ywidth,true);

	if( fabs(yscale)<1.0f )
	{
		for(y=0;y<ywidth;y++)
			for(x=0;x<xwidth;x++) 
			{
				fy=(y-ypos)/yscale;
				set(x,y,integrate<0>(fy,x,yscale,YDIR), integrate<1>(fy,x,yscale,YDIR), integrate<2>(fy,x,yscale,YDIR), integrate<3>(fy,x,yscale,YDIR));
			}
	}
	else
	{
		for(y=0;y<ywidth;y++)
			for(x=0;x<xwidth;x++) 
			{
				fy=(y-ypos)/yscale;
				set(x,y,lint<0>(fy,x,YDIR), lint<1>(fy,x,YDIR), lint<2>(fy,x,YDIR), lint<3>(fy,x,YDIR));
			}
	}
}

void rescaler::resize_bicubic(int xwidth,int ywidth,float xpos,float ypos,float xscale,float yscale)
{
	short x,y;
	float fx,fy;
	int p0,p1,p2,p3;

	xpos=xwidth*(1.0f+xpos-xscale)/2.0f-0.5f;
	ypos=ywidth*(1.0f+ypos-yscale)/2.0f-0.5f;
	xscale=xscale*(float)xwidth/src_width_;
	yscale=yscale*(float)ywidth/src_height_;

	alloc(xwidth,src_height_,false);

	if( fabs(xscale)<1.0f ) 
	{
		for(y=0;y<src_height_;y++)
			for(x=0;x<xwidth;x++) 
			{
				fx=(x-xpos)/xscale;
				set(x,y,integrate<0>(fx,y,xscale,XDIR), integrate<1>(fx,y,xscale,XDIR), integrate<2>(fx,y,xscale,XDIR), integrate<3>(fx,y,xscale,XDIR));
			}
	}
	else
	{
		for(y=0;y<src_height_;y++)
			for(x=0;x<xwidth;x++) 
			{
				fx=(x-xpos)/xscale;
				if( fx>=0 && fx<src_width_) 
				{
					p1=(int)_clip(floor(fx),0,src_width_-1);
					fx=_clip(fx-p1,0,1);
					p2=(int)_clip(p1+1,0,src_width_-1);
					p3=(int)_clip(p2+1,0,src_width_-1);
					p0=(int)_clip(p1-1,0,src_width_-1);

					set(x,y,_cubic(fx,get<0>(p0,y),get<0>(p1,y),get<0>(p2,y),get<0>(p3,y)),
						_cubic(fx,get<1>(p0,y),get<1>(p1,y),get<1>(p2,y),get<1>(p3,y)),
						_cubic(fx,get<2>(p0,y),get<2>(p1,y),get<2>(p2,y),get<2>(p3,y)),
						_cubic(fx,get<3>(p0,y),get<3>(p1,y),get<3>(p2,y),get<3>(p3,y)) );
				}
				else 
					set(x,y,0.0f,0.0f,0.0f,0.0f);
			}
	}

	alloc(xwidth,ywidth,true);

	if( fabs(yscale)<1.0f )
	{
		for(y=0;y<ywidth;y++)
			for(x=0;x<xwidth;x++) 
			{
				fy=(y-ypos)/yscale;
				set(x,y,integrate<0>(fy,x,yscale,YDIR),integrate<1>(fy,x,yscale,YDIR),integrate<2>(fy,x,yscale,YDIR),integrate<3>(fy,x,yscale,YDIR));
			}
	}
	else
	{
		for(y=0;y<ywidth;y++)
			for(x=0;x<xwidth;x++) 
			{
				fy=(y-ypos)/yscale;
				if( fy>=0 && fy<src_height_) 
				{
					p1=(int)_clip(floor(fy),0,src_height_-1);
					fy=_clip(fy-p1,0,1);
					p2=(int)_clip(p1+1,0,src_height_-1);
					p3=(int)_clip(p2+1,0,src_height_-1);
					p0=(int)_clip(p1-1,0,src_height_-1);

					set(x,y,_cubic(fy,get<0>(x,p0),get<0>(x,p1),get<0>(x,p2),get<0>(x,p3)),
						_cubic(fy,get<1>(x,p0),get<1>(x,p1),get<1>(x,p2),get<1>(x,p3)),
						_cubic(fy,get<2>(x,p0),get<2>(x,p1),get<2>(x,p2),get<2>(x,p3)),
						_cubic(fy,get<3>(x,p0),get<3>(x,p1),get<3>(x,p2),get<3>(x,p3)));
				} 
				else 
					set(x,y,0.0f,0.0f,0.0f,0.0f);
			}
	}
}

void rescaler::alloc(int dst_width, int dst_height, bool reassign)
{
	if(reassign)
	{
		_aspect_assert(owns_data_orig_ == false);

		data_orig_ = data_result_;
		src_stride_ = dst_stride_;
		src_width_ = dst_width_;
		src_height_ = dst_height_;
		owns_data_orig_ = true;
	}

	data_result_ = (uint8_t*)malloc(dst_width*dst_height*4*sizeof(uint8_t));
	dst_stride_ = dst_width*4;
	dst_width_ = dst_width;
	dst_height_ = dst_height;
}

void rescaler::dealloc()
{
	if ( owns_data_orig_ && data_orig_ )
	{
		free(data_orig_);
	}

	if ( data_result_ )
	{
		free(data_result_);
	}

	owns_data_orig_ = false;
	data_orig_ = NULL;
	data_result_ = NULL;
}

rescaler::rescaler()
	: owns_data_orig_(false),
	data_orig_(NULL),
	data_result_(NULL)
{
}

rescaler::~rescaler()
{
	dealloc();
}

void rescaler::rescale(uint8_t* pixels, image_size const& src_size, int mode, image_size const& dst_size,
		float xpos,float ypos,float xscale,float yscale)
{
	dealloc();

	owns_data_orig_ = false;
	data_orig_ = pixels;
	data_result_ = NULL;
	src_width_ = src_size.width;
	src_height_ = src_size.height;
	src_stride_ = src_size.width * 4;
	dst_stride_ = dst_size.width * 4;
	dst_width_ = -1;
	dst_height_ = -1;

	switch(mode)
	{
	default:
	case NEAREST:
		resize_nearest(dst_size.width,dst_size.height,xpos,ypos,xscale,yscale);
		break;
	case BILINEAR:
		resize_bilinear(dst_size.width,dst_size.height,xpos,ypos,xscale,yscale);
		break;
	case BICUBIC:
		resize_bicubic(dst_size.width,dst_size.height,xpos,ypos,xscale,yscale);
		break;
	}
}

} } // namespace aspect::codecs
