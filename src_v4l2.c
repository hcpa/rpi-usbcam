/* fswebcam - FireStorm.cx's webcam generator                 */
/*============================================================*/
/* Copyright (C)2005-2014 Philip Heron <phil@sanslogic.co.uk> */
/*                                                            */
/* This program is distributed under the terms of the GNU     */
/* General Public License, version 2. You may use, modify,    */
/* and redistribute it under the terms of this license. A     */
/* copy should be included with this source.                  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "test.h"
#include "videodev2.h"
#include "log.h"
#include "src_v4l2.h"

typedef struct {
	void *start;
	size_t length;
} v4l2_buffer_t;

typedef struct {
	
	int fd;
	char map;
	
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	
	v4l2_buffer_t *buffer;
	
	int pframe;
	
} src_v4l2_t;

typedef struct {
	uint16_t src;
	uint32_t v4l2;
} v4l2_palette_t;

v4l2_palette_t v4l2_palette[] = {
	{ SRC_PAL_YUYV,    V4L2_PIX_FMT_YUYV   },
	{ SRC_PAL_JPEG,    V4L2_PIX_FMT_JPEG   },
	{ SRC_PAL_MJPEG,   V4L2_PIX_FMT_MJPEG  },
	{ SRC_PAL_S561,    V4L2_PIX_FMT_SPCA561 },
	{ SRC_PAL_RGB24,   V4L2_PIX_FMT_RGB24  },
	{ SRC_PAL_BGR24,   V4L2_PIX_FMT_BGR24  },
	{ SRC_PAL_RGB32,   V4L2_PIX_FMT_RGB32  },
	{ SRC_PAL_BGR32,   V4L2_PIX_FMT_BGR32  },
	{ SRC_PAL_UYVY,    V4L2_PIX_FMT_UYVY   },
	{ SRC_PAL_YUV420P, V4L2_PIX_FMT_YUV420 },
	{ SRC_PAL_BAYER,   V4L2_PIX_FMT_SBGGR8 },
	{ SRC_PAL_SGBRG8,  V4L2_PIX_FMT_SGBRG8 },
	{ SRC_PAL_SGRBG8,  V4L2_PIX_FMT_SGRBG8 },
	{ SRC_PAL_RGB565,  V4L2_PIX_FMT_RGB565 },
	{ SRC_PAL_RGB555,  V4L2_PIX_FMT_RGB555 },
	{ SRC_PAL_Y16,     V4L2_PIX_FMT_Y16    },
	{ SRC_PAL_GREY,    V4L2_PIX_FMT_GREY   },
	{ 0, 0 }
};

int src_v4l2_get_capability(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	
	if(ioctl(s->fd, VIDIOC_QUERYCAP, &s->cap) < 0)
	{
		ERROR("%s: Not a V4L2 device?", src->source);
		return(-1);
	}
	
	DEBUG("%s information:", src->source);
	DEBUG("cap.driver: \"%s\"", s->cap.driver);
	DEBUG("cap.card: \"%s\"", s->cap.card);
	DEBUG("cap.bus_info: \"%s\"", s->cap.bus_info);
	DEBUG("cap.capabilities=0x%08X", s->cap.capabilities);
	if(s->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) DEBUG("- VIDEO_CAPTURE");
	if(s->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)  DEBUG("- VIDEO_OUTPUT");
	if(s->cap.capabilities & V4L2_CAP_VIDEO_OVERLAY) DEBUG("- VIDEO_OVERLAY");
	if(s->cap.capabilities & V4L2_CAP_VBI_CAPTURE)   DEBUG("- VBI_CAPTURE");
	if(s->cap.capabilities & V4L2_CAP_VBI_OUTPUT)    DEBUG("- VBI_OUTPUT");
	if(s->cap.capabilities & V4L2_CAP_RDS_CAPTURE)   DEBUG("- RDS_CAPTURE");
	if(s->cap.capabilities & V4L2_CAP_TUNER)         DEBUG("- TUNER");
	if(s->cap.capabilities & V4L2_CAP_AUDIO)         DEBUG("- AUDIO");
	if(s->cap.capabilities & V4L2_CAP_RADIO)         DEBUG("- RADIO");
	if(s->cap.capabilities & V4L2_CAP_READWRITE)     DEBUG("- READWRITE");
	if(s->cap.capabilities & V4L2_CAP_ASYNCIO)       DEBUG("- ASYNCIO");
	if(s->cap.capabilities & V4L2_CAP_STREAMING)     DEBUG("- STREAMING");
	if(s->cap.capabilities & V4L2_CAP_TIMEPERFRAME)  DEBUG("- TIMEPERFRAME");
	
	if(!s->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
	{
		ERROR("Device does not support capturing.");
		return(-1);
	}
	
	return(0);
}

int src_v4l2_set_input(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	struct v4l2_input input;
	int count = 0, i = -1;
	
	memset(&input, 0, sizeof(input));
	
	/* If no input was specified, use input 0. */
	if(!src->input)
	{
		MSG("No input was specified, using the first.");
		count = 1;
		i = 0;
	}
	
	/* Check if the input is specified by name. */
	if(i == -1)
	{
		input.index = count;
		while(!ioctl(s->fd, VIDIOC_ENUMINPUT, &input))
		{
			if(!strncasecmp((char *) input.name, src->input, 32))
				i = count;
			input.index = ++count;
		}
	}
	
	if(i == -1)
	{
		char *endptr;
		
		/* Is the input specified by number? */
		i = strtol(src->input, &endptr, 10);
		
		if(endptr == src->input) i = -1;
	}
	
	if(i == -1 || i >= count)
	{
		/* The specified input wasn't found! */
		ERROR("Unrecognised input \"%s\"", src->input);
		return(-1);
	}
	
	/* Set the input. */
	input.index = i;
	if(ioctl(s->fd, VIDIOC_ENUMINPUT, &input) == -1)
	{
		ERROR("Unable to query input %i.", i);
		ERROR("VIDIOC_ENUMINPUT: %s", strerror(errno));
		return(-1);
	}
	
	DEBUG("%s: Input %i information:", src->source, i);
	DEBUG("name = \"%s\"", input.name);
	DEBUG("type = %08X", input.type);
	if(input.type & V4L2_INPUT_TYPE_TUNER) DEBUG("- TUNER");
	if(input.type & V4L2_INPUT_TYPE_CAMERA) DEBUG("- CAMERA");
	DEBUG("audioset = %08X", input.audioset);
	DEBUG("tuner = %08X", input.tuner);
	DEBUG("status = %08X", input.status);
	if(input.status & V4L2_IN_ST_NO_POWER) DEBUG("- NO_POWER");
	if(input.status & V4L2_IN_ST_NO_SIGNAL) DEBUG("- NO_SIGNAL");
	if(input.status & V4L2_IN_ST_NO_COLOR) DEBUG("- NO_COLOR");
	if(input.status & V4L2_IN_ST_NO_H_LOCK) DEBUG("- NO_H_LOCK");
	if(input.status & V4L2_IN_ST_COLOR_KILL) DEBUG("- COLOR_KILL");
	if(input.status & V4L2_IN_ST_NO_SYNC) DEBUG("- NO_SYNC");
	if(input.status & V4L2_IN_ST_NO_EQU) DEBUG("- NO_EQU");
	if(input.status & V4L2_IN_ST_NO_CARRIER) DEBUG("- NO_CARRIER");
	if(input.status & V4L2_IN_ST_MACROVISION) DEBUG("- MACROVISION");
	if(input.status & V4L2_IN_ST_NO_ACCESS) DEBUG("- NO_ACCESS");
	if(input.status & V4L2_IN_ST_VTR) DEBUG("- VTR");
	
	if(ioctl(s->fd, VIDIOC_S_INPUT, &i) == -1)
	{
		ERROR("Error selecting input %i", i);
		ERROR("VIDIOC_S_INPUT: %s", strerror(errno));
		return(-1);
	}
	
	return(0);
}

int src_v4l2_set_pix_format(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	struct v4l2_fmtdesc fmt;
	int v4l2_pal;
	
	/* Dump a list of formats the device supports. */
	DEBUG("Device offers the following V4L2 pixel formats:");
	
	v4l2_pal = 0;
	memset(&fmt, 0, sizeof(fmt));
	fmt.index = v4l2_pal;
	fmt.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	while(ioctl(s->fd, VIDIOC_ENUM_FMT, &fmt) != -1)
	{
		DEBUG("%i: [0x%08X] '%c%c%c%c' (%s)", v4l2_pal,
		      fmt.pixelformat,
		      fmt.pixelformat >> 0,  fmt.pixelformat >> 8,
		      fmt.pixelformat >> 16, fmt.pixelformat >> 24,
		      fmt.description);
		
		memset(&fmt, 0, sizeof(fmt));
		fmt.index = ++v4l2_pal;
		fmt.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	}
	
	/* Step through each palette type. */
	v4l2_pal = 0;
	
	if(src->palette != -1)
	{
		while(v4l2_palette[v4l2_pal].v4l2)
		{
			if(v4l2_palette[v4l2_pal].src == src->palette) break;
			v4l2_pal++;
		}
		
		if(!v4l2_palette[v4l2_pal].v4l2)
		{
			ERROR("Unable to handle palette format %s.",
			      src_palette[src->palette]);
			
			return(-1);
		}
	}
	
	while(v4l2_palette[v4l2_pal].v4l2)
	{
		/* Try the palette... */
		memset(&s->fmt, 0, sizeof(s->fmt));
		s->fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		s->fmt.fmt.pix.width       = src->width;
		s->fmt.fmt.pix.height      = src->height;
		s->fmt.fmt.pix.pixelformat = v4l2_palette[v4l2_pal].v4l2;
		s->fmt.fmt.pix.field       = V4L2_FIELD_ANY;
		
		if(ioctl(s->fd, VIDIOC_TRY_FMT, &s->fmt) != -1 &&
		   s->fmt.fmt.pix.pixelformat == v4l2_palette[v4l2_pal].v4l2)
		{
			src->palette = v4l2_palette[v4l2_pal].src;
			
			INFO("Using palette %s", src_palette[src->palette].name);
			
			if(s->fmt.fmt.pix.width != src->width ||
			   s->fmt.fmt.pix.height != src->height)
			{
				MSG("Adjusting resolution from %ix%i to %ix%i.",
				    src->width, src->height,
				    s->fmt.fmt.pix.width,
				    s->fmt.fmt.pix.height);
				src->width = s->fmt.fmt.pix.width;
				src->height = s->fmt.fmt.pix.height;
                        }
			
			if(ioctl(s->fd, VIDIOC_S_FMT, &s->fmt) == -1)
			{
				ERROR("Error setting pixel format.");
				ERROR("VIDIOC_S_FMT: %s", strerror(errno));
				return(-1);
			}
			
			if(v4l2_palette[v4l2_pal].v4l2 == V4L2_PIX_FMT_MJPEG)
			{
				struct v4l2_jpegcompression jpegcomp;
				
				memset(&jpegcomp, 0, sizeof(jpegcomp));
				ioctl(s->fd, VIDIOC_G_JPEGCOMP, &jpegcomp);
				jpegcomp.jpeg_markers |= V4L2_JPEG_MARKER_DHT;
				ioctl(s->fd, VIDIOC_S_JPEGCOMP, &jpegcomp);
			}
			
			return(0);
		}
		
		if(src->palette != -1) break;
		
		v4l2_pal++;
	}
	
	ERROR("Unable to find a compatible palette format.");
	
	return(-1);
}

int src_v4l2_free_mmap(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	int i;
	
	for(i = 0; i < s->req.count; i++)
		munmap(s->buffer[i].start, s->buffer[i].length);
	
	return(0);
}

int src_v4l2_set_mmap(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	enum v4l2_buf_type type;
	uint32_t b;
	
	/* Does the device support streaming? */
	if(~s->cap.capabilities & V4L2_CAP_STREAMING) return(-1);
	
	memset(&s->req, 0, sizeof(s->req));
	
	s->req.count  = 4;
	s->req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	s->req.memory = V4L2_MEMORY_MMAP;
	
	if(ioctl(s->fd, VIDIOC_REQBUFS, &s->req) == -1)
	{
		ERROR("Error requesting buffers for memory map.");
		ERROR("VIDIOC_REQBUFS: %s", strerror(errno));
		return(-1);
	}
	
	DEBUG("mmap information:");
	DEBUG("frames=%d", s->req.count);
	
	if(s->req.count < 2)
	{
		ERROR("Insufficient buffer memory.");
		return(-1);
        }
	
	s->buffer = calloc(s->req.count, sizeof(v4l2_buffer_t));
	if(!s->buffer)
	{
		ERROR("Out of memory.");
		return(-1);
	}
	
	for(b = 0; b < s->req.count; b++)
	{
		struct v4l2_buffer buf;
		
		memset(&buf, 0, sizeof(buf));
		
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = b;
		
		if(ioctl(s->fd, VIDIOC_QUERYBUF, &buf) == -1)
		{
			ERROR("Error querying buffer %i", b);
			ERROR("VIDIOC_QUERYBUF: %s", strerror(errno));
			free(s->buffer);
			s->buffer = NULL;
			return(-1);
		}
		
		s->buffer[b].length = buf.length;
		s->buffer[b].start = mmap(NULL, buf.length,
		   PROT_READ | PROT_WRITE, MAP_SHARED, s->fd, buf.m.offset);
		
		if(s->buffer[b].start == MAP_FAILED)
		{
			ERROR("Error mapping buffer %i", b);
			ERROR("mmap: %s", strerror(errno));
			s->req.count = b;
			src_v4l2_free_mmap(src);
			free(s->buffer);
			s->buffer = NULL;
			return(-1);
		}
		
		DEBUG("%i length=%d", b, buf.length);
	}
	
	s->map = -1;
	
	for(b = 0; b < s->req.count; b++)
	{
		memset(&s->buf, 0, sizeof(s->buf));
		
		s->buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		s->buf.memory = V4L2_MEMORY_MMAP;
		s->buf.index  = b;
		
		if(ioctl(s->fd, VIDIOC_QBUF, &s->buf) == -1)
		{
			ERROR("VIDIOC_QBUF: %s", strerror(errno));
			src_v4l2_free_mmap(src);
			free(s->buffer);
			s->buffer = NULL;
			return(-1);
		}
	}
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if(ioctl(s->fd, VIDIOC_STREAMON, &type) == -1)
	{
		ERROR("Error starting stream.");
		ERROR("VIDIOC_STREAMON: %s", strerror(errno));
		src_v4l2_free_mmap(src);
		free(s->buffer);
		s->buffer = NULL;
		return(-1);
	}
	
	return(0);
}

int src_v4l2_set_read(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	
	if(~s->cap.capabilities & V4L2_CAP_READWRITE) return(-1);
	
	s->buffer = calloc(1, sizeof(v4l2_buffer_t));
	if(!s->buffer)
	{
		ERROR("Out of memory.");
		return(-1);
	}
	
	s->buffer[0].length = s->fmt.fmt.pix.sizeimage;
	s->buffer[0].start  = malloc(s->buffer[0].length);
	
	if(!s->buffer[0].start)
	{
		ERROR("Out of memory.");
		
		free(s->buffer);
		s->buffer = NULL;
		
		return(-1);
	}
	
	return(0);
}

int src_v4l2_open(src_t *src)
{
	src_v4l2_t *s;
	
	if(!src->source)
	{
		ERROR("No device name specified.");
		return(-2);
	}
	
	/* Allocate memory for the state structure. */
	s = calloc(sizeof(src_v4l2_t), 1);
	if(!s)
	{
		ERROR("Out of memory.");
		return(-2);
	}
	
	src->state = (void *) s;
	
	/* Open the device. */
	s->fd = open(src->source, O_RDWR | O_NONBLOCK);
	if(s->fd < 0)
	{
		ERROR("Error opening device: %s", src->source);
		ERROR("open: %s", strerror(errno));
		free(s);
		return(-2);
	}
	
	MSG("%s opened.", src->source);
	
	/* Get the device capabilities. */
	if(src_v4l2_get_capability(src))
	{
		src_v4l2_close(src);
		return(-2);
	}
	
	/* Set the input. */
	if(src_v4l2_set_input(src))
	{
		src_v4l2_close(src);
		return(-1);
	}
	
	/* Set the pixel format. */
	if(src_v4l2_set_pix_format(src))
	{
		src_v4l2_close(src);
		return(-1);
	}
	
	/* Try to setup mmap. */
	if(!src->use_read && src_v4l2_set_mmap(src))
	{
		WARN("Unable to use mmap. Using read instead.");
		src->use_read = -1;
	}
	
	/* If unable to use mmap or user requested read(). */
	if(src->use_read)
	{
		if(src_v4l2_set_read(src))
		{
			ERROR("Unable to use read.");
			src_v4l2_close(src);
			return(-1);
		}
	}
	
	s->pframe = -1;
	
	return(0);
}

int src_v4l2_close(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	
	if(s->buffer)
	{
		if(!s->map) free(s->buffer[0].start);
		else src_v4l2_free_mmap(src);
		free(s->buffer);
	}
	if(s->fd >= 0) close(s->fd);
	free(s);
	
	return(0);
}

int src_v4l2_grab(src_t *src)
{
	src_v4l2_t *s = (src_v4l2_t *) src->state;
	
	if(src->timeout)
	{
		fd_set fds;
		struct timeval tv;
		int r;
		
		/* Is a frame ready? */
		FD_ZERO(&fds);
		FD_SET(s->fd, &fds);
		
		tv.tv_sec = src->timeout;
		tv.tv_usec = 0;
		
		r = select(s->fd + 1, &fds, NULL, NULL, &tv);
		
		if(r == -1)
		{
			ERROR("select: %s", strerror(errno));
			return(-1);
		}
		
		if(!r)
		{
			ERROR("Timed out waiting for frame!");
			return(-1);
		}
	}
	
	if(s->map)
	{
		if(s->pframe >= 0)
		{
			if(ioctl(s->fd, VIDIOC_QBUF, &s->buf) == -1)
			{
				ERROR("VIDIOC_QBUF: %s", strerror(errno));
				return(-1);
			}
		}
		
		memset(&s->buf, 0, sizeof(s->buf));
		
		s->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		s->buf.memory = V4L2_MEMORY_MMAP;
		
		if(ioctl(s->fd, VIDIOC_DQBUF, &s->buf) == -1)
		{
			DEBUG("errno %d",errno);
			ERROR("VIDIOC_DQBUF: %s", strerror(errno));
			return(-1);
		}
		
		src->img    = s->buffer[s->buf.index].start;
		src->length = s->buffer[s->buf.index].length;
		
		s->pframe = s->buf.index;
	}
	else
	{
		ssize_t r;
		
		r = read(s->fd, s->buffer[0].start, s->buffer[0].length);
		if(r <= 0)
		{
			ERROR("Unable to read a frame.");
			ERROR("read: %s", strerror(errno));
			return(-1);
		}
		
		src->img = s->buffer[0].start;
		src->length = r;
	}
	
	return(0);
}

