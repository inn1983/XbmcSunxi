/*
 *      Copyright (C) 2013 Yin Yi
 *	  inngi2011@gmail.com
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

 
#pragma once
 
#include "Texture.h"
 
//#if defined(HAS_GL) || defined(HAS_GLES)
 
//#include "system_gl.h"

struct comp_t
{
	uint8_t samp_h;
	uint8_t samp_v;
	uint8_t qt;
	uint8_t ht_ac;
	uint8_t ht_dc;
};

struct quant_t
{
	uint8_t coeff[64];
};

struct huffman_t
{
	uint8_t num[16];
	uint8_t codes[256];
};

struct jpeg_t
 {
	 uint8_t bits;
	 uint16_t width;
	 uint16_t height;
	 struct comp_t comp[3];
	 struct quant_t *quant[4];
	 struct huffman_t *huffman[8];
	 uint16_t restart_interval;
	 uint8_t *data;
	 uint32_t data_len;
 };
 
 
 /************************************************************************/
 /*    CA10Texture														*/
 /************************************************************************/
 class CA10Texture : public CBaseTexture
 {
 public:
   CA10Texture(unsigned int width = 0, unsigned int height = 0, unsigned int format = XB_FMT_YUV);
   virtual ~CA10Texture();
   bool LoadFromFileInternal(const CStdString& texturePath, unsigned int maxWidth, unsigned int maxHeight, bool autoRotate);
   void Allocate(unsigned int width, unsigned int height, unsigned int format);
   virtual void CreateTextureObject(){}
   virtual void DestroyTextureObject(){}
   virtual void LoadToGPU(){}
   virtual void BindToUnit(unsigned int unit){}
   unsigned char* GetPixelY(){}
   unsigned char* GetPixelUV(){}
   
   uint32_t GetPixelYphys();
   uint32_t GetPixelUVphys();

 };

int ve_open(void);
void ve_close(void);
void *ve_get_regs(void);
int ve_wait(int timeout);

void *ve_malloc(int size);
void ve_free(void *ptr);
uint32_t ve_virt2phys(void *ptr);


int parse_jpeg(struct jpeg_t *jpeg, const uint8_t *data, const int len);
void dump_jpeg(const struct jpeg_t *jpeg);

void set_quantization_tables(struct jpeg_t *jpeg, void *regs);
void set_huffman_tables(struct jpeg_t *jpeg, void *regs);
void set_format(struct jpeg_t *jpeg, void *regs);
void set_size(struct jpeg_t *jpeg, void *regs);

static inline void writeb(void *addr, uint8_t val)
{
	*((volatile uint8_t *)addr) = val;
}

static inline void writel(void *addr, uint32_t val)
{
	*((volatile uint32_t *)addr) = val;
}

static inline uint8_t readb(void *addr)
{
	return *((volatile uint8_t *) addr);
}

static inline uint32_t readl(void *addr)
{
	return *((volatile uint32_t *) addr);
}