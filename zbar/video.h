/*------------------------------------------------------------------------
 *  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/
#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <config.h>

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include <zbar.h>

#include "error.h"

/* number of images to preallocate */
#define ZBAR_VIDEO_IMAGES_MAX  4

typedef enum video_interface_e {
    VIDEO_INVALID = 0,          /* uninitialized */
    VIDEO_V4L1,                 /* v4l protocol version 1 */
    VIDEO_V4L2,                 /* v4l protocol version 2 */
} video_interface_t;

typedef enum video_iomode_e {
    VIDEO_READWRITE = 1,        /* standard system calls */
    VIDEO_MMAP,                 /* mmap interface */
    VIDEO_USERPTR,              /* userspace buffers */
} video_iomode_t;

struct zbar_video_s {
    errinfo_t err;              /* error reporting */
    int fd;                     /* open camera device */
    unsigned width, height;     /* video frame size */

    video_interface_t intf;     /* input interface type */
    video_iomode_t iomode;      /* video data transfer mode */
    unsigned initialized : 1;   /* format selected and images mapped */
    unsigned active      : 1;   /* current streaming state */

    uint32_t format;            /* selected fourcc */
    unsigned palette;           /* v4l1 format index corresponding to format */
    uint32_t *formats;          /* 0 terminated list of supported formats */

    unsigned long datalen;      /* size of image data for selected format */
    unsigned long buflen;       /* total size of image data buffer */
    void *buf;                  /* image data buffer */

    int num_images;             /* number of allocated images */
    zbar_image_t **images;      /* indexed list of images */
    zbar_image_t *nq_image;     /* last image enqueued */
    zbar_image_t *dq_image;     /* first image to dequeue (when ordered) */
    zbar_image_t *shadow_image; /* special case internal double buffering */

#ifdef HAVE_LIBPTHREAD
    pthread_mutex_t qlock;      /* lock image queue */
#endif

#ifdef HAVE_LIBJPEG
    struct jpeg_decompress_struct *jpeg; /* JPEG decompressor */
    zbar_image_t *jpeg_img;    /* temporary image */
#endif

    unsigned frame;             /* frame count */

    /* interface dependent methods */
    int (*init)(zbar_video_t*, uint32_t);
    int (*cleanup)(zbar_video_t*);
    int (*start)(zbar_video_t*);
    int (*stop)(zbar_video_t*);
    int (*nq)(zbar_video_t*, zbar_image_t*);
    zbar_image_t* (*dq)(zbar_video_t*);
};


#ifdef HAVE_LIBPTHREAD

/* video.next_image and video.recycle_image have to be thread safe
 * wrt/other apis
 */
static inline int video_lock (zbar_video_t *vdo)
{
    int rc = 0;
    if((rc = pthread_mutex_lock(&vdo->qlock))) {
        err_capture(vdo, SEV_FATAL, ZBAR_ERR_LOCKING, __func__,
                    "unable to acquire lock");
        vdo->err.errnum = rc;
        return(-1);
    }
    return(0);
}

static inline int video_unlock (zbar_video_t *vdo)
{
    int rc = 0;
    if((rc = pthread_mutex_unlock(&vdo->qlock))) {
        err_capture(vdo, SEV_FATAL, ZBAR_ERR_LOCKING, __func__,
                    "unable to release lock");
        vdo->err.errnum = rc;
        return(-1);
    }
    return(0);
}

#else
# define video_lock(...) (0)
# define video_unlock(...) (0)
#endif

extern int _zbar_video_open(zbar_video_t*, const char*);
extern int _zbar_v4l2_probe(zbar_video_t*);

#endif
