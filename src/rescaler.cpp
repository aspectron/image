#include "image.hpp"
#include "rescaler.hpp"

namespace aspect { namespace image {

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

}} // aspect::image
