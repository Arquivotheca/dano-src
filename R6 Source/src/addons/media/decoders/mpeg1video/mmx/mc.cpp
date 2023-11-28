#include "mc_mmx.h"
#include "mc2_mmx.h"

void mc_hf_vf_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] ;
			p2[i] = c; 
		}
		p1 += stride;
		p2 += stride;
	}	  
}


void mc_hh_vf_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			p2[i] = c; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc_hf_vh_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + stride;
	
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			p2[i] = c; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc_hh_vh_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p1c = ref;
	unsigned char * p1d = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	p1c += delta + stride;
	p1d += delta + stride + 1;
	
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] + p1b[i] + p1c[i] + p1d[i] + 2;
			c = c / 4;
			p2[i] = c; 
		}
		p1 += stride;
		p1b += stride;
		p1c += stride;
		p1d += stride;
		p2 += stride;
	}	  
}


void mc_hf_vf_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] ;
			p2[i] = c; 
		}
		p1 += stride;
		p2 += stride;
	}	  
}


void mc_hh_vf_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			p2[i] = c; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc_hf_vh_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + stride;
	
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			p2[i] = c; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc_hh_vh_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p1c = ref;
	unsigned char * p1d = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	p1c += delta + stride;
	p1d += delta + stride + 1;
	
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] + p1b[i] + p1c[i] + p1d[i] + 2;
			c = c / 4;
			p2[i] = c; 
		}
		p1 += stride;
		p1b += stride;
		p1c += stride;
		p1d += stride;
		p2 += stride;
	}	  
}



void mc2_hf_vf_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] + p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p2 += stride;
	}	  
}


void mc2_hh_vf_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			c += p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc2_hf_vh_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + stride;
	
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			c += p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc2_hh_vh_8(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p1c = ref;
	unsigned char * p1d = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	p1c += delta + stride;
	p1d += delta + stride + 1;
	
	
	
	for(j=0;j<8;j++)
	{
		for(i=0;i<8;i++)
		{
			c = p1[i] + p1b[i] + p1c[i] + p1d[i] + 2;
			c = c / 4;
			c += p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p1b += stride;
		p1c += stride;
		p1d += stride;
		p2 += stride;
	}	  
}


void mc2_hf_vf_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] + p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p2 += stride;
	}	  
}


void mc2_hh_vf_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			c += p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc2_hf_vh_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + stride;
	
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] + p1b[i] + 1;
			c = c / 2;
			c += p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p1b += stride;
		p2 += stride;
	}	  
}


void mc2_hh_vh_16(unsigned char * ref,
			  unsigned char * dest,
			  int stride,
			  int delt)
{
	int i,j;
	short c;
	unsigned char * p1 = ref;
	unsigned char * p1b = ref;
	unsigned char * p1c = ref;
	unsigned char * p1d = ref;
	unsigned char * p2 = dest;
	int delta = delt / 8;
	p1 += delta;
	p1b += delta + 1;
	p1c += delta + stride;
	p1d += delta + stride + 1;
	
	
	
	for(j=0;j<16;j++)
	{
		for(i=0;i<16;i++)
		{
			c = p1[i] + p1b[i] + p1c[i] + p1d[i] + 2;
			c = c / 4;
			c += p2[i] + 1;
			p2[i] = c / 2; 
		}
		p1 += stride;
		p1b += stride;
		p1c += stride;
		p1d += stride;
		p2 += stride;
	}	  
}
