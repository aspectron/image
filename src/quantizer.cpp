#include "image/image.hpp"
#include "image/quantizer.hpp"

namespace aspect { namespace image {

struct color24;
struct color32;

struct color32
{
	uint8_t b,g,r,a; //r,g,b,a;

	color32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: r(r), g(g), b(b), a(a)
	{
	}

	float get_r() const { return r / 255.0f; }
	float get_g() const { return g / 255.0f; }
	float get_b() const { return b / 255.0f; }
	float get_a() const { return a / 255.0f; }

	void set_r(float r) { r = (uint8_t)(r * 255.0f); }
	void set_g(float g) { g = (uint8_t)(g * 255.0f); }
	void set_b(float b) { b = (uint8_t)(b * 255.0f); }
	void set_a(float a) { a = (uint8_t)(a * 255.0f); }

	void set(float r, float g, float b, float a)
	{
		set_r(r);
		set_g(g);
		set_b(b);
		set_a(a);
	}

	void interpolate(color32 const& src, color32 const& dst, float f)
	{
		set_r((src.get_r() * (1.0f-f))+dst.get_r()*f);
		set_g((src.get_g() * (1.0f-f))+dst.get_g()*f);
		set_b((src.get_b() * (1.0f-f))+dst.get_b()*f);
		set_a((src.get_a() * (1.0f-f))+dst.get_a()*f);
	}

	uint16_t rgb565() const
	{
		return ( ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) );
	}

	color32 bgra() const { return color32(b, g, r, a); }

	color32& operator=(const color24 &src);
};

struct color24
{
	uint8_t b,g,r;//r,g,b;

	color24(uint8_t r, uint8_t g, uint8_t b)
		: r(r), g(g), b(b)
	{
	}

	uint16_t rgb565() const
	{
		return ( ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3) );
	}

	color24& operator=(const color32 &src)
	{
		r = src.r; g = src.g; b = src.b;
		return *this;
	}
};

color32& color32::operator=(color24 const& src)
{
	r = src.r; g = src.g; b = src.b; a = 255;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/**********************************************************************
	    C Implementation of Wu's Color Quantizer (v. 2)
	    (see Graphics Gems vol. II, pp. 126-133)

Author:	Xiaolin Wu
	Dept. of Computer Science
	Univ. of Western Ontario
	London, Ontario N6A 5B7
	wu@csd.uwo.ca

Algorithm: Greedy orthogonal bipartition of RGB space for variance
	   minimization aided by inclusion-exclusion tricks.
	   For speed no nearest neighbor search is done. Slightly
	   better performance can be expected by more sophisticated
	   but more expensive versions.

The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much of
additional documentation and a cure to a previous bug.

Free to distribute, comments and suggestions are appreciated.
**********************************************************************/	

static size_t const MAXCOLOR = 8192;

enum direction { RED = 2, GREEN = 1, BLUE = 0 };

struct rgb_box {
    int r0;			 /* min value, exclusive */
    int r1;			 /* max value, inclusive */
    int g0;  
    int g1;  
    int b0;  
    int b1;
    int vol;
};

/* Histogram is in elements 1..HISTSIZE along each axis,
 * element 0 is for base or marginal value
 * NB: these must start out 0!
 */

typedef struct QuantizeContext {
	float		gm2[33][33][33];
	int			wt[33][33][33];
	int			mr[33][33][33];
	int			mg[33][33][33];
	int			mb[33][33][33];
//	SduColor**	ppcolor;		// Original image data
//	color32	*pcolor32;
//	color24 *pcolor24;
	uint8_t const* pcolor;
	int			bpp;
	int			left;
	int			top;
	int			cx;				// Image width
	int			cy;				// Image height
	int			stride;			// number of bytes between beginnings of lines
	int			K;				// Desired number of colors
	unsigned short* Qadd;		// The new image
} QuantizeContext;


// build 3-D color histogram of counts, r/g/b, c^2
static void Hist3d(int* vwt, int* vmr, int* vmg, int* vmb, float* m2, QuantizeContext* pqc) 
{
	 int ind, r, g, b;
	int	     inr, ing, inb, table[256];
	 long int i;

	for(i=0; i<256; ++i) table[i]=i*i;
	unsigned short* pqadd = pqc->Qadd;
//	int size = pqc->cy*pqc->cx;
	uint8_t const* pcolor = pqc->pcolor;
	
	for(int y = 0; y < pqc->cy; y++)
	for(int x = 0; x < pqc->cx; x++)
//	for(i = 0; i < iSize; ++i)
//	for (i = 0; i<pqc->cy; ++i)
	{
//		SduColor* pcolor = pqc->ppcolor[i];
//		for (long int j = 0; j < pqc->cx; ++j) {
//			SduColor c = *pcolor++;
			//GAPI_Color32 c = *pcolor++;

		uint8_t const* src = pcolor+((y+pqc->top)*pqc->stride+(x+pqc->left)*4);

		color24 *pc = (color24 *)src; //pcolor;
//		pcolor += pqc->bpp;
		    r = pc->r; g = pc->g; b = pc->b;
			inr=(r>>3)+1; 
			ing=(g>>3)+1; 
			inb=(b>>3)+1; 
			ind=(inr<<10)+(inr<<6)+inr+(ing<<5)+ing+inb;
			*pqadd=(unsigned short)ind;
			++pqadd;
			/*[inr][ing][inb]*/
			++vwt[ind];
			vmr[ind] += r;
			vmg[ind] += g;
			vmb[ind] += b;
   			m2[ind] += (float)(table[r]+table[g]+table[b]);
//		}
	}
}

/* At conclusion of the histogram step, we can interpret
 *   wt[r][g][b] = sum over voxel of P(c)
 *   mr[r][g][b] = sum over voxel of r*P(c)  ,  similarly for mg, mb
 *   m2[r][g][b] = sum over voxel of c^2*P(c)
 * Actually each of these should be divided by 'size' to give the usual
 * interpretation of P() as ranging from 0 to 1, but we needn't do that here.
 */

/* We now convert histogram into moments so that we can rapidly calculate
 * the sums of the above quantities over any desired box.
 */


/* compute cumulative moments. */
static void M3d(int* vwt, int* vmr, int* vmg, int* vmb, float* m2) 
{
	 unsigned short int ind1, ind2;
	 unsigned char i, r, g, b;
	int line, line_r, line_g, line_b,
		 area[33], area_r[33], area_g[33], area_b[33];
	float    line2, area2[33];

    for(r=1; r<=32; ++r){
		for(i=0; i<=32; ++i) {
			area[i]=area_r[i]=area_g[i]=area_b[i]=0;
			area2[i]=(float)area[i];
		}
		for(g=1; g<=32; ++g){
			line = line_r = line_g = line_b = 0;
			line2 = (float)line;
			for(b=1; b<=32; ++b){
				ind1 = (unsigned short)((r<<10) + (r<<6) + r + (g<<5) + g + b); /* [r][g][b] */
				line += vwt[ind1];
				line_r += vmr[ind1]; 
				line_g += vmg[ind1]; 
				line_b += vmb[ind1];
				line2 += m2[ind1];
				area[b] += line;
				area_r[b] += line_r;
				area_g[b] += line_g;
				area_b[b] += line_b;
				area2[b] += line2;
				ind2 = (unsigned short)(ind1 - 1089); /* [r-1][g][b] */
				vwt[ind1] = vwt[ind2] + area[b];
				vmr[ind1] = vmr[ind2] + area_r[b];
				vmg[ind1] = vmg[ind2] + area_g[b];
				vmb[ind1] = vmb[ind2] + area_b[b];
				m2[ind1] = m2[ind2] + area2[b];
			}
		}
    }
}


/* Compute sum over a box of any given statistic */
static long int Vol(rgb_box* cube, int mmt[33][33][33]) 
{
    return( mmt[cube->r1][cube->g1][cube->b1] 
	   -mmt[cube->r1][cube->g1][cube->b0]
	   -mmt[cube->r1][cube->g0][cube->b1]
	   +mmt[cube->r1][cube->g0][cube->b0]
	   -mmt[cube->r0][cube->g1][cube->b1]
	   +mmt[cube->r0][cube->g1][cube->b0]
	   +mmt[cube->r0][cube->g0][cube->b1]
	   -mmt[cube->r0][cube->g0][cube->b0] );
}

/* The next two routines allow a slightly more efficient calculation
 * of Vol() for a proposed subbox of a given box.  The sum of Top()
 * and Bottom() is the Vol() of a subbox split in the given direction
 * and with the specified new upper bound.
 */

/* Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1 */
/* (depending on dir) */
static long int Bottom(rgb_box *cube, direction dir, int mmt[33][33][33])
{
	switch(dir)
	{
	case RED:
		return( -mmt[cube->r0][cube->g1][cube->b1]
		    +mmt[cube->r0][cube->g1][cube->b0]
		    +mmt[cube->r0][cube->g0][cube->b1]
		    -mmt[cube->r0][cube->g0][cube->b0] );
	default:
	case GREEN:
	return( -mmt[cube->r1][cube->g0][cube->b1]
		    +mmt[cube->r1][cube->g0][cube->b0]
		    +mmt[cube->r0][cube->g0][cube->b1]
		    -mmt[cube->r0][cube->g0][cube->b0] );
	case BLUE:
		return( -mmt[cube->r1][cube->g1][cube->b0]
		    +mmt[cube->r1][cube->g0][cube->b0]
		    +mmt[cube->r0][cube->g1][cube->b0]
		    -mmt[cube->r0][cube->g0][cube->b0] );
	}
}


/* Compute remainder of Vol(cube, mmt), substituting pos for */
/* r1, g1, or b1 (depending on dir) */
static long int Top(rgb_box *cube, direction dir, int pos, int mmt[33][33][33])
{
	switch(dir)
	{
	case RED:
		return( mmt[pos][cube->g1][cube->b1] 
		   -mmt[pos][cube->g1][cube->b0]
		   -mmt[pos][cube->g0][cube->b1]
		   +mmt[pos][cube->g0][cube->b0] );
	default:
	case GREEN:
		return( mmt[cube->r1][pos][cube->b1] 
		   -mmt[cube->r1][pos][cube->b0]
		   -mmt[cube->r0][pos][cube->b1]
		   +mmt[cube->r0][pos][cube->b0] );
	case BLUE:
		return( mmt[cube->r1][cube->g1][pos]
		   -mmt[cube->r1][cube->g0][pos]
		   -mmt[cube->r0][cube->g1][pos]
		   +mmt[cube->r0][cube->g0][pos] );
	}
}


/* Compute the weighted variance of a box */
/* NB: as with the raw statistics, this is really the variance * size */
static float Var(rgb_box* cube, QuantizeContext* pqc)
{
	float dr, dg, db, xx;

    dr = (float)Vol(cube, pqc->mr); 
    dg = (float)Vol(cube, pqc->mg); 
    db = (float)Vol(cube, pqc->mb);
    xx =  pqc->gm2[cube->r1][cube->g1][cube->b1] 
		 -pqc->gm2[cube->r1][cube->g1][cube->b0]
		 -pqc->gm2[cube->r1][cube->g0][cube->b1]
		 +pqc->gm2[cube->r1][cube->g0][cube->b0]
		 -pqc->gm2[cube->r0][cube->g1][cube->b1]
		 +pqc->gm2[cube->r0][cube->g1][cube->b0]
		 +pqc->gm2[cube->r0][cube->g0][cube->b1]
		 -pqc->gm2[cube->r0][cube->g0][cube->b0];

    return( xx - (dr*dr+dg*dg+db*db)/(float)Vol(cube,pqc->wt) );    
}

/* We want to minimize the sum of the variances of two subboxes.
 * The sum(c^2) terms can be ignored since their sum over both subboxes
 * is the same (the sum for the whole box) no matter where we split.
 * The remaining terms have a minus sign in the variance formula,
 * so we drop the minus sign and MAXIMIZE the sum of the two terms.
 */


static float Maximize(
	rgb_box *cube,
	direction dir,
	int first, int last, int *cut,
	int whole_r, int whole_g, int whole_b, int whole_w,
	QuantizeContext* pqc
)
{
	 int half_r, half_g, half_b, half_w;
	int base_r, base_g, base_b, base_w;
	 int i;
	 float temp, max;

    base_r = Bottom(cube, dir, pqc->mr);
    base_g = Bottom(cube, dir, pqc->mg);
    base_b = Bottom(cube, dir, pqc->mb);
    base_w = Bottom(cube, dir, pqc->wt);
    max = 0.0;
    *cut = -1;
    for(i=first; i<last; ++i){
	half_r = base_r + Top(cube, dir, i, pqc->mr);
	half_g = base_g + Top(cube, dir, i, pqc->mg);
	half_b = base_b + Top(cube, dir, i, pqc->mb);
	half_w = base_w + Top(cube, dir, i, pqc->wt);
        /* now half_x is sum over lower half of box, if split at i */
        if (half_w == 0) {      /* subbox could be empty of pixels! */
          continue;             /* never split into an empty box */
	} else
        temp = ((float)half_r*half_r + (float)half_g*half_g +
                (float)half_b*half_b)/half_w;

	half_r = whole_r - half_r;
	half_g = whole_g - half_g;
	half_b = whole_b - half_b;
	half_w = whole_w - half_w;
        if (half_w == 0) {      /* subbox could be empty of pixels! */
          continue;             /* never split into an empty box */
	} else
        temp += ((float)half_r*half_r + (float)half_g*half_g +
                 (float)half_b*half_b)/half_w;

    	if (temp > max) {max=temp; *cut=i;}
    }
    return(max);
}

static int Cut(rgb_box *set1, rgb_box *set2, QuantizeContext* pqc)
{
	direction dir;
	int cutr, cutg, cutb;
	float maxr, maxg, maxb;
	int whole_r, whole_g, whole_b, whole_w;

    whole_r = Vol(set1, pqc->mr);
    whole_g = Vol(set1, pqc->mg);
    whole_b = Vol(set1, pqc->mb);
    whole_w = Vol(set1, pqc->wt);

    maxr = Maximize(set1, RED, set1->r0+1, set1->r1, &cutr,
		    whole_r, whole_g, whole_b, whole_w, pqc);
    maxg = Maximize(set1, GREEN, set1->g0+1, set1->g1, &cutg,
		    whole_r, whole_g, whole_b, whole_w, pqc);
    maxb = Maximize(set1, BLUE, set1->b0+1, set1->b1, &cutb,
		    whole_r, whole_g, whole_b, whole_w, pqc);

    if( (maxr>=maxg)&&(maxr>=maxb) ) {
		dir = RED;
		if (cutr < 0) return 0; /* can't split the box */
    }
    else
    if( (maxg>=maxr)&&(maxg>=maxb) ) 
		dir = GREEN;
    else
		dir = BLUE; 

    set2->r1 = set1->r1;
    set2->g1 = set1->g1;
    set2->b1 = set1->b1;

    switch (dir){
		case RED:
			set2->r0 = set1->r1 = cutr;
			set2->g0 = set1->g0;
			set2->b0 = set1->b0;
			break;
		case GREEN:
			set2->g0 = set1->g1 = cutg;
			set2->r0 = set1->r0;
			set2->b0 = set1->b0;
			break;
		case BLUE:
			set2->b0 = set1->b1 = cutb;
			set2->r0 = set1->r0;
			set2->g0 = set1->g0;
			break;
    }
    set1->vol=(set1->r1-set1->r0)*(set1->g1-set1->g0)*(set1->b1-set1->b0);
    set2->vol=(set2->r1-set2->r0)*(set2->g1-set2->g0)*(set2->b1-set2->b0);
    return 1;
}


static void Mark(rgb_box *cube, int label, unsigned char *tag)
{
	 int r, g, b;

	for(r=cube->r0+1; r<=cube->r1; ++r)
		for(g=cube->g0+1; g<=cube->g1; ++g)
			for(b=cube->b0+1; b<=cube->b1; ++b)
				tag[(r<<10) + (r<<6) + r + (g<<5) + g + b] = (unsigned char)label;
}

///////////////////////////////////////////////////////////////////////////
//
// aspect::quantizer
//
void quantizer::quantize(uint8_t const* pixels, size_t stride, image_rect const& rect, size_t num_colors)
{
	if ( num_colors >= MAXCOLOR )
	{
		num_colors = MAXCOLOR - 1;
	}

	QuantizeContext qc = {};
	qc.pcolor = pixels; //tx.get_data(); //pData; //ppGetData();
	qc.bpp = 4; // tx.get_bpp();
	// we don't support other color depths
	assert(qc.bpp == 4 || qc.bpp == 3);
	qc.stride = static_cast<int>(stride);
	qc.left = rect.left;
	qc.top = rect.top;
	qc.cx = rect.width; //tx.get_width();//iWidth;//iGetWidth();
	qc.cy = rect.height; //tx.get_height(); //iHeight;//iGetHeight();
	qc.K = static_cast<int>(num_colors);

	std::vector<uint16_t> Qadd(qc.cx * qc.cy);
	qc.Qadd = &Qadd[0];

	Hist3d((int*)qc.wt, (int*)qc.mr, (int*)qc.mg, (int*)qc.mb, (float*)qc.gm2, &qc);
	//printf("Histogram done\n");
	//free(Ig); free(Ib); free(Ir);

	M3d((int*)qc.wt, (int*)qc.mr, (int*)qc.mg, (int*)qc.mb, (float*)qc.gm2);
	//printf("Moments done\n");

	rgb_box cube[MAXCOLOR];
	cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
	cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;
	size_t next = 0;
	float vv[MAXCOLOR];
	for (size_t i = 1; i < num_colors; ++i)
	{
		if ( Cut(&cube[next], &cube[i], &qc) )
		{
			/* volume test ensures we won't try to cut one-cell box */
			vv[next] = (cube[next].vol>1) ? Var(&cube[next], &qc) : 0.0f;
			vv[i] = (cube[i].vol>1) ? Var(&cube[i], &qc) : 0.0f;
		}
		else
		{
			vv[next] = 0.0;   /* don't try to split this box again */
			i--;              /* didn't create box i */
		}
		next = 0;
		float max = vv[0];
		for (size_t k = 1; k <= i; ++k)
		{
			if ( vv[k] > max )
			{
				max = vv[k]; next = k;
			}
		}
		if (max <= 0.0 )
		{
			num_colors = i + 1;
//			fprintf(stderr, "Only got %d boxes\n", iNumColors);
			break;
		}
	}
//	printf("Partition done\n");

	/* the space for array gm2 can be freed now */

	memset(&lut_, 0, sizeof(lut_));
	std::vector<uint8_t> tag(33*33*33);

	for (size_t k = 0; k < num_colors; ++k)
	{
		Mark(&cube[k], static_cast<int>(k), &tag[0]);
		long const weight = Vol(&cube[k], qc.wt);
		if ( weight )
		{
			uint8_t const r = (uint8_t)(Vol(&cube[k], qc.mr) / weight);
			uint8_t const g = (uint8_t)(Vol(&cube[k], qc.mg) / weight);
			uint8_t const b = (uint8_t)(Vol(&cube[k], qc.mb) / weight);

			lut_.rgb[k].r = lut_.rgba[k].r = r;
			lut_.rgb[k].g = lut_.rgba[k].g = g;
			lut_.rgb[k].b = lut_.rgba[k].b = b;
			lut_.rgba[k].a = 255;
		}
		else
		{
			lut_.rgb[k].r = lut_.rgb[k].g = lut_.rgb[k].b = 0;
			lut_.rgba[k].r = lut_.rgba[k].g = lut_.rgba[k].b = lut_.rgba[k].a = 0;
		}
	}
//	      fprintf(stderr, "bogus box %d\n", k);


	/* output lut_r, lut_g, lut_b as color look-up table contents,
	   Qadd as the quantized image (array of table addresses). */

	size_t const size = rect.width * rect.height; //iWidth * iHeight;
	result_data_.resize(size);
//	if(tx.bpp == 4)
	{
		for (size_t i = 0; i < size; ++i)
		{
			result_data_[i] = tag[qc.Qadd[i]];
		}
	}
// 		else
// 		if(tx.bpp == 3)
// 		{
// 			color24 *pcolor24 = tx.get_data_24();
// 			for(i = 0; i < iSize; ++i)
// 			{
// 				int iIndex = tag[*pqadd++];
// 				*pcolor24++ = color24(lut_r[iIndex], lut_g[iIndex], lut_b[iIndex]);
// 			}
// 		}
/*
	for (i = 0; i < iGetHeight(); ++i) {
	SduColor* pcolor = ppGetData()[i];
	for (int j = 0; j < iGetWidth(); ++j) {
		int iIndex = tag[*pqadd++];
		int a = pcolor->a;
		*pcolor++ = SduColor(lut_r[iIndex], lut_g[iIndex], lut_b[iIndex], a);
	}
*/
	//}

	lut_size_ = num_colors;
}

}} // aspect::image
