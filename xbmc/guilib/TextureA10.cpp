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

#include "system.h"
#include "TextureA10.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "windowing/WindowingFactory.h"
#include "utils/log.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/mman.h>

#include "cores/VideoRenderers/LinuxRendererA10.h"

#define DEVICE "/dev/cedar_dev"
#define PAGE_OFFSET (0xc0000000) // from kernel
#define PAGE_SIZE (4096)

enum IOCTL_CMD
{
	IOCTL_UNKOWN = 0x100,
	IOCTL_GET_ENV_INFO,
	IOCTL_WAIT_VE,
	IOCTL_RESET_VE,
	IOCTL_ENABLE_VE,
	IOCTL_DISABLE_VE,
	IOCTL_SET_VE_FREQ,

	IOCTL_CONFIG_AVS2 = 0x200,
	IOCTL_GETVALUE_AVS2 ,
	IOCTL_PAUSE_AVS2 ,
	IOCTL_START_AVS2 ,
	IOCTL_RESET_AVS2 ,
	IOCTL_ADJUST_AVS2,
	IOCTL_ENGINE_REQ,
	IOCTL_ENGINE_REL,
	IOCTL_ENGINE_CHECK_DELAY,
	IOCTL_GET_IC_VER,
	IOCTL_ADJUST_AVS2_ABS,
	IOCTL_FLUSH_CACHE
};

struct ve_info
{
	uint32_t reserved_mem;
	int reserved_mem_size;
	uint32_t registers;
};

static int fd = -1;
static void *regs = NULL;

struct memchunk_t
{
	uint32_t phys_addr;
	int size;
	void *virt_addr;
	struct memchunk_t *next;
};

static struct memchunk_t first_memchunk = { 0x0, 0, NULL, NULL };

using namespace std;

/************************************************************************/
/*    CA10Texture                                                       */
/************************************************************************/

CA10Texture::CA10Texture(unsigned int width, unsigned int height, unsigned int format)
: CBaseTexture(width, height, format)
{

}

CA10Texture::~CA10Texture()
{
  DestroyTextureObject();
  ve_free(m_pixelsY);
  ve_free(m_pixelsUV);
}

bool CA10Texture::LoadFromFileInternal(const CStdString& texturePath, unsigned int maxWidth, unsigned int maxHeight, bool autoRotate)
{
  //ImageLib is sooo sloow for jpegs. Try our own decoder first. If it fails, fall back to ImageLib.
  CStdString Ext = URIUtils::GetExtension(texturePath);
  Ext.ToLower(); // Ignore case of the extension
  if (Ext.Equals(".jpg") || Ext.Equals(".jpeg") || Ext.Equals(".tbn"))
  {
  	int in;
	struct stat s;
	uint8_t *data;
	int ret = 0;
  	in = open(texturePath, O_RDONLY);
	fstat(in, &s);
	data = (uint8_t*)mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, in, 0);

	struct jpeg_t jpeg;
	memset(&jpeg, 0, sizeof(jpeg));
	if (!parse_jpeg(&jpeg, data, s.st_size)){
		CLog::Log(LOGERROR, "TextureA10: Can't parse JPEG");
	}
	/*
	if (!ve_open()){
		CLog::Log(LOGERROR, "TextureA10: Can't open VE");
	}
	*/
	Allocate(jpeg.width, jpeg.height, XB_FMT_YUV);

	void *ve_regs = ve_get_regs();
	int input_size =(jpeg.data_len + 65535) & ~65535;
	uint8_t* input_buffer = (uint8_t*)ve_malloc(input_size);
	memcpy(input_buffer, jpeg.data, jpeg.data_len);

	// activate MPEG engine
	writel(ve_regs + 0x00, 0x00130000);

	// set restart interval
	writel(ve_regs + 0x100 + 0xc0, jpeg.restart_interval);

	// set JPEG format
	set_format(&jpeg, ve_regs);

	// set output buffers (Luma / Croma)
	writel(ve_regs + 0x100 + 0xcc, ve_virt2phys(m_pixelsY));
	writel(ve_regs + 0x100 + 0xd0, ve_virt2phys(m_pixelsUV));

	// set size
	set_size(&jpeg, ve_regs);

	// ??
	writel(ve_regs + 0x100 + 0xd4, 0x00000000);

	// input end
	writel(ve_regs + 0x100 + 0x34, ve_virt2phys(input_buffer) + input_size - 1);

	// ??
	writel(ve_regs + 0x100 + 0x14, 0x0000007c);

	// set input offset in bits
	writel(ve_regs + 0x100 + 0x2c, 0 * 8);

	// set input length in bits
	writel(ve_regs + 0x100 + 0x30, jpeg.data_len * 8);

	// set input buffer
	writel(ve_regs + 0x100 + 0x28, ve_virt2phys(input_buffer) | 0x70000000);

	// set Quantisation Table
	set_quantization_tables(&jpeg, ve_regs);

	// set Huffman Table
	writel(ve_regs + 0x100 + 0xe0, 0x00000000);
	set_huffman_tables(&jpeg, ve_regs);

	// start
	writeb(ve_regs + 0x100 + 0x18, 0x0e);

	// wait for interrupt
	ve_wait(1);

	// clean interrupt flag (??)
	writel(ve_regs + 0x100 + 0x1c, 0x0000c00f);

	// stop MPEG engine
	writel(ve_regs + 0x0, 0x00130007);
	
	//ve_close();
	ve_free(input_buffer);
	
	munmap(data, s.st_size);
	close(in);
	switch ((jpeg.comp[0].samp_h << 4) | jpeg.comp[0].samp_v)
	{
	case 0x11:
	case 0x21:
		m_color= COLOR_YUV422;
		break;
	case 0x12:
	case 0x22:
	default:
		m_color = COLOR_YUV420;
		break;
	}
	return true;
  }
}

void CA10Texture::Allocate(unsigned int width, unsigned int height, unsigned int format)
{
	CBaseTexture::Allocate(width, height, format);
	ve_free(m_pixelsY);
	ve_free(m_pixelsUV);
	int output_size = ((width + 31) & ~31) * ((height + 31) & ~31);
	m_pixelsY = (uint8_t*)ve_malloc(output_size);
	m_pixelsUV = (uint8_t*)ve_malloc(output_size);
}

uint32_t CA10Texture::GetPixelYphys()
{
	return ve_virt2phys(m_pixelsY);
}

uint32_t CA10Texture::GetPixelUVphys()
{
	return ve_virt2phys(m_pixelsUV);

}


int ve_open(void)
{
	struct ve_info ve;

	fd = open(DEVICE, O_RDWR);
	if (fd == -1)
		return 0;

	if (ioctl(fd, IOCTL_GET_ENV_INFO, (void *)(&ve)) == -1)
	{
		close(fd);
		fd = -1;
		return 0;
	}

	regs = mmap(NULL, 0x800, PROT_READ | PROT_WRITE, MAP_SHARED, fd, ve.registers);
	first_memchunk.phys_addr = ve.reserved_mem - PAGE_OFFSET;
	first_memchunk.size = ve.reserved_mem_size;

	ioctl(fd, IOCTL_ENGINE_REQ, 0);
	ioctl(fd, IOCTL_ENABLE_VE, 0);
	ioctl(fd, IOCTL_SET_VE_FREQ, 160);
	ioctl(fd, IOCTL_RESET_VE, 0);

	return 1;
}

void ve_close(void)
{
	ioctl(fd, IOCTL_DISABLE_VE, 0);
	ioctl(fd, IOCTL_ENGINE_REL, 0);

	munmap(regs, 0x800);

	close(fd);
}

void *ve_get_regs(void)
{
	return regs;
}

int ve_wait(int timeout)
{
	return ioctl(fd, IOCTL_WAIT_VE, timeout);
}

void *ve_malloc(int size)
{
	size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	struct memchunk_t *c, *best_chunk = NULL;
	for (c = &first_memchunk; c != NULL; c = c->next)
		if(c->virt_addr == NULL && c->size >= size)
		{
			if (best_chunk == NULL || c->size < best_chunk->size)
				best_chunk = c;

			if (c->size == size)
				break;
		}

	if (!best_chunk)
		return NULL;

	int left_size = best_chunk->size - size;

	best_chunk->virt_addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, best_chunk->phys_addr + PAGE_OFFSET);
	best_chunk->size = size;

	if (left_size > 0)
	{
		c = (struct memchunk_t*)malloc(sizeof(struct memchunk_t));
		c->phys_addr = best_chunk->phys_addr + size;
		c->size = left_size;
		c->virt_addr = NULL;
		c->next = best_chunk->next;
		best_chunk->next = c;
	}

	return best_chunk->virt_addr;
}

void ve_free(void *ptr)
{
	if (ptr == NULL)
		return;

	struct memchunk_t *c;
	for (c = &first_memchunk; c != NULL; c = c->next)
		if (c->virt_addr == ptr)
		{
			munmap(ptr, c->size);
			c->virt_addr = NULL;
			break;
		}

	for (c = &first_memchunk; c != NULL; c = c->next)
		if (c->virt_addr == NULL)
			while (c->next != NULL && c->next->virt_addr == NULL)
			{
				struct memchunk_t *n = c->next;
				c->size += n->size;
				c->next = n->next;
				free(n);
			}
}

uint32_t ve_virt2phys(void *ptr)
{
	struct memchunk_t *c;
	for (c = &first_memchunk; c != NULL; c = c->next)
	{
		if (c->virt_addr == NULL)
			continue;

		if (c->virt_addr == ptr)
			return c->phys_addr;
		else if (ptr > c->virt_addr && ptr < (c->virt_addr + c->size))
			return c->phys_addr + ((char*)ptr - (char*)c->virt_addr);
	}

	return 0;
}

#define M_SOF0  0xc0
#define M_SOF1  0xc1
#define M_SOF2  0xc2
#define M_SOF3  0xc3
#define M_SOF5  0xc5
#define M_SOF6  0xc6
#define M_SOF7  0xc7
#define M_SOF9  0xc9
#define M_SOF10 0xca
#define M_SOF11 0xcb
#define M_SOF13 0xcd
#define M_SOF14 0xce
#define M_SOF15 0xcf
#define M_SOI   0xd8
#define M_EOI   0xd9
#define M_SOS   0xda
#define M_DQT   0xdb
#define M_DRI   0xdd
#define M_DHT   0xc4
#define M_DAC   0xcc

const char comp_types[5][3] = { "Y", "Cb", "Cr" };

static int process_dqt(struct jpeg_t *jpeg, const uint8_t *data, const int len)
{
	int pos;

	for (pos = 0; pos < len; pos += 65)
	{
		if ((data[pos] >> 4) != 0)
		{
			fprintf(stderr, "Only 8bit Quantization Tables supported\n");
			return 0;
		}
		jpeg->quant[data[pos] & 0x03] = (struct quant_t *)&(data[pos + 1]);
	}

	return 1;
}

static int process_dht(struct jpeg_t *jpeg, const uint8_t *data, const int len)
{
	int pos = 0;

	while (pos < len)
	{
		uint8_t id = ((data[pos] & 0x03) << 1) | ((data[pos] & 0x10) >> 4);

		jpeg->huffman[id] = (struct huffman_t *)&(data[pos + 1]);

		int i;
		pos += 17;
		for (i = 0; i < 16; i++)
			pos += jpeg->huffman[id]->num[i];
	}
	return 1;
}

int parse_jpeg(struct jpeg_t *jpeg, const uint8_t *data, const int len)
{
	if (len < 2 || data[0] != 0xff || data[1] != M_SOI)
		return 0;

	int pos = 2;
	int sos = 0;
	while (!sos)
	{
		int i;

		while (pos < len && data[pos] == 0xff)
			pos++;

		if (!(pos + 2 < len))
			return 0;

		uint8_t marker = data[pos++];
		uint16_t seg_len = ((uint16_t)data[pos]) << 8 | (uint16_t)data[pos + 1];

		if (!(pos + seg_len < len))
			return 0;

		switch (marker)
		{
		case M_DQT:
			if (!process_dqt(jpeg, &data[pos + 2], seg_len - 2))
				return 0;

			break;

		case M_DHT:
			if (!process_dht(jpeg, &data[pos + 2], seg_len - 2))
				return 0;

			break;

		case M_SOF0:
			jpeg->bits = data[pos + 2];
			jpeg->width = ((uint16_t)data[pos + 5]) << 8 | (uint16_t)data[pos + 6];
			jpeg->height = ((uint16_t)data[pos + 3]) << 8 | (uint16_t)data[pos + 4];
			for (i = 0; i < data[pos + 7]; i++)
			{
				uint8_t id = data[pos + 8 + 3 * i] - 1;
				if (id > 2)
				{
					fprintf(stderr, "only YCbCr supported\n");
					return 0;
				}
				jpeg->comp[id].samp_h = data[pos + 9 + 3 * i] >> 4;
				jpeg->comp[id].samp_v = data[pos + 9 + 3 * i] & 0x0f;
				jpeg->comp[id].qt = data[pos + 10 + 3 * i];
			}
			break;

		case M_DRI:
			jpeg->restart_interval = ((uint16_t)data[pos + 2]) << 8 | (uint16_t)data[pos + 3];
			break;

		case M_SOS:
			for (i = 0; i < data[pos + 2]; i++)
			{
				uint8_t id = data[pos + 3 + 2 * i] - 1;
				if (id > 2)
				{
					fprintf(stderr, "only YCbCr supported\n");
					return 0;
				}
				jpeg->comp[id].ht_dc = data[pos + 4 + 2 * i] >> 4;
				jpeg->comp[id].ht_ac = data[pos + 4 + 2 * i] & 0x0f;
			}
			sos = 1;
			break;

		case M_DAC:
			fprintf(stderr, "Arithmetic Coding unsupported\n");
			return 0;

		case M_SOF1:
		case M_SOF2:
		case M_SOF3:
		case M_SOF5:
		case M_SOF6:
		case M_SOF7:
		case M_SOF9:
		case M_SOF10:
		case M_SOF11:
		case M_SOF13:
		case M_SOF14:
		case M_SOF15:
			fprintf(stderr, "only Baseline DCT supported (yet?)\n");
			return 0;

		case M_SOI:
		case M_EOI:
			fprintf(stderr, "corrupted file\n");
			return 0;

		default:
			//fprintf(stderr, "unknown marker: 0x%02x len: %u\n", marker, seg_len);
			break;
		}
		pos += seg_len;
	}

	jpeg->data = (uint8_t *)&(data[pos]);
	jpeg->data_len = len - pos;

	return 1;
}

void dump_jpeg(const struct jpeg_t *jpeg)
{
	int i, j, k;
	printf("Width: %u  Height: %u  Bits: %u\nRestart interval: %u\nComponents:\n", jpeg->width, jpeg->height, jpeg->bits, jpeg->restart_interval);
	for (i = 0; i < 3; i++)
	{
		if (jpeg->comp[i].samp_h && jpeg->comp[i].samp_v)
			printf("\tType: %s  Sampling: %u:%u  QT: %u  HT: %u/%u\n", comp_types[i], jpeg->comp[i].samp_h, jpeg->comp[i].samp_v, jpeg->comp[i].qt, jpeg->comp[i].ht_dc, jpeg->comp[i].ht_dc);
	}
	printf("Quantization Tables:\n");
	for (i = 0; i < 4; i++)
	{
		if (jpeg->quant[i])
		{
			printf("\tID: %u\n", i);
			for (j = 0; j < 64 / 8; j++)
			{
				printf("\t\t");
				for (k = 0; k < 8; k++)
				{
					printf("0x%02x ", jpeg->quant[i]->coeff[j * 8 + k]);
				}
				printf("\n");
			}
		}
	}
	printf("Huffman Tables:\n");
	for (i = 0; i < 8; i++)
	{
		if (jpeg->huffman[i])
		{
			printf("\tID: %u (%cC)\n", (i & 0x06) >> 1, i & 0x01 ? 'A' : 'D');
			int sum = 0;
			for (j = 0; j < 16; j++)
			{
				if (jpeg->huffman[i]->num[j])
				{
					printf("\t\t%u bits:", j + 1);
					for (k = 0; k < jpeg->huffman[i]->num[j]; k++)
					{
						printf(" 0x%02x", jpeg->huffman[i]->codes[sum + k]);
					}
					printf("\n");
					sum += jpeg->huffman[i]->num[j];
				}
			}
		}
	}
	printf("Data length: %u\n", jpeg->data_len);
}


//main.c
void set_quantization_tables(struct jpeg_t *jpeg, void *regs)
{
	int i;
	for (i = 0; i < 64; i++)
		writel(regs + 0x100 + 0x80, (uint32_t)(64 + i) << 8 | jpeg->quant[0]->coeff[i]);
	for (i = 0; i < 64; i++)
		writel(regs + 0x100 + 0x80, (uint32_t)(i) << 8 | jpeg->quant[1]->coeff[i]);
}

void set_huffman_tables(struct jpeg_t *jpeg, void *regs)
{
	uint32_t buffer[512];
	memset(buffer, 0, 4*512);
	int i;
	for (i = 0; i < 4; i++)
	{
		if (jpeg->huffman[i])
		{
			int j, sum, last;

			last = 0;
			sum = 0;
			for (j = 0; j < 16; j++)
			{
				((uint8_t *)buffer)[i * 64 + 32 + j] = sum;
				sum += jpeg->huffman[i]->num[j];
				if (jpeg->huffman[i]->num[j] != 0)
					last = j;
			}
			memcpy(&(buffer[256 + 64 * i]), jpeg->huffman[i]->codes, sum);
			sum = 0;
			for (j = 0; j <= last; j++)
			{
				((uint16_t *)buffer)[i * 32 + j] = sum;
				sum += jpeg->huffman[i]->num[j];
				sum *= 2;
			}
			for (j = last + 1; j < 16; j++)
			{
				((uint16_t *)buffer)[i * 32 + j] = 0xffff;
			}
		}
	}

	for (i = 0; i < 512; i++)
	{
		writel(regs + 0x100 + 0xe4, buffer[i]);
	}
}

void set_format(struct jpeg_t *jpeg, void *regs)
{
	uint8_t fmt = (jpeg->comp[0].samp_h << 4) | jpeg->comp[0].samp_v;

	switch (fmt)
	{
	case 0x11:
		writeb(regs + 0x100 + 0x1b, 0x1b);
		break;
	case 0x21:
		writeb(regs + 0x100 + 0x1b, 0x13);
		break;
	case 0x12:
		writeb(regs + 0x100 + 0x1b, 0x23);
		break;
	case 0x22:
		writeb(regs + 0x100 + 0x1b, 0x03);
		break;
	}
}

void set_size(struct jpeg_t *jpeg, void *regs)
{
	uint16_t h = (jpeg->height - 1) / (8 * jpeg->comp[0].samp_v);
	uint16_t w = (jpeg->width - 1) / (8 * jpeg->comp[0].samp_h);
	writel(regs + 0x100 + 0xb8, (uint32_t)h << 16 | w);
}


