#ifndef IMAGE_RESCALER_HPP_INCLUDED
#define IMAGE_RESCALER_HPP_INCLUDED

namespace aspect { namespace image {

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

}} // aspect::image

#endif // IMAGE_RESCALER_HPP_INCLUDED
