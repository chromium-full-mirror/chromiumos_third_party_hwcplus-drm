// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// so we get dl_iterate_phdr from link.h
#define _GNU_SOURCE

#include <assert.h>
#include <dlfcn.h>
#include <link.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "hardware/gralloc.h"
#include "hardware/hardware.h"
#include "hardware/hwcomposer.h"
#include "log/log.h"
#include "vendor/gralloc_drm_defs.h"

typedef const char *str;

static void diff(str *a, str *b) {
    int c;
    while (*a || *b) {
        if (!*a) c=1;
        else if (!*b) c=-1;
        else c=strcmp(*a,*b);
        if (c<0) printf("del %s\n", *a++);
        else if (c>0) printf("add %s\n", *b++);
        else { ++a; ++b; }
    }
}

static int cb(struct dl_phdr_info *info, size_t size, void *data) {
    if(info->dlpi_name) {
        str **p = (str**)data;
        *(*p)++ = strdup(info->dlpi_name);
    }

    return 0;
    for (int i=0; i<info->dlpi_phnum; ++i) {
        //printf("  [%d] %p\n", i, info->dlpi_phdr[i]);
    }
}

static int cmpstr(const void *a, const void *b) {
    return strcmp(*(str*)a, *(str*)b);
}

static str* prt_shared(str *prev) {
    printf("shared objs\n");
    str e[]={NULL};
    if (!prev) prev=e;
    str *cur = (str *)malloc(99*sizeof(str));
    str *p = cur;
    dl_iterate_phdr(cb, &p);
    *p = NULL;
    qsort(cur, p-cur, sizeof(str), cmpstr);
    diff(prev, cur);
    return cur;
}

static void prt_module(const struct hw_module_t *module) {
    printf("module %p\n", module);
    printf("  tag %d %.4s\n", (int)module->tag, (char*)&module->tag);
    printf("  module_api_version %08x\n", (unsigned)module->module_api_version);
    printf("  hal_api_version %d\n", (int)module->hal_api_version);
    printf("  id %s\n", module->id);
    printf("  name %s\n", module->name);
    printf("  author %s\n", module->author);
    printf("  methods %p\n", module->methods);
    printf("    [0] %p\n", module->methods[0].open);
    printf("  dso %p\n", module->dso);
    printf("    sym %p\n", dlsym(module->dso, "drm_mod_open"));
    printf("----\n");
}

static void prt_device(struct hw_device_t *device) {
    printf("device %p\n", device);
    printf("  tag %u\n", device->tag);
    printf("  version %u\n", device->version);
    printf("  module %p\n", device->module);
}

static void prt_fb(framebuffer_device_t *fb) {
    printf("fb %p\n", fb);
    prt_device(&fb->common);
    printf("  flags %u\n", fb->flags);
    printf("  width height %u %u\n", fb->width, fb->height);
    printf("  stride %u\n", fb->stride);
    printf("  format %d %d\n", fb->format, fb->format);
    printf("  xdpi ydpi %f %f\n", fb->xdpi, fb->ydpi);
    printf("  fps %f\n", fb->fps);
    printf("  minSwapInterval %d\n", fb->minSwapInterval);
    printf("  maxSwapInterval %d\n", fb->maxSwapInterval);
    printf("  numFramebuffers %d\n", fb->numFramebuffers);
    printf("  setSwapInterval %p\n", fb->setSwapInterval);
    printf("  setUpdateRect %p\n", fb->setUpdateRect);
    printf("  post %p\n", fb->post);
    printf("  compositionComplete %p\n", fb->compositionComplete);
    printf("  dump %p\n", fb->dump);
    printf("  enableScreen %p\n", fb->enableScreen);
}

static void prt_alloc(struct alloc_device_t *alloc) {
    printf("alloc %p\n", alloc);
    prt_device(&alloc->common);
    printf("  alloc %p\n", alloc->alloc);
    printf("  free %p\n", alloc->free);
    printf("  dump %p\n", alloc->dump);
}

static double hat(double x) {
    x=fmod(x,3.);
    if (x<1.)
        return x;
    if (x<2.)
        return 2.-x;
    return 0.;
}

static uint32_t hue(double x) {
    return
        ((int)(hat(x+0.)*255.99) << 0) +
        ((int)(hat(x+1.)*255.99) << 8) +
        ((int)(hat(x+2.)*255.99) << 16);
}

static double distance(int a,int b,int x,int y) {
    return sqrt((a-x)*(a-x)+(b-y)*(b-y));
}

static void target(uint32_t *p, double o, int w, int h) {
    int x=w/2, y=h/2;
    for (int j=0; j<h; ++j) {
        for (int i=0; i<w; ++i) {
            *p++=hue(o+distance(i,j,x,y)/200.);
        }
    }
}

int main(int argc, char *argv[]) {
    const struct hw_module_t *mod;
    const struct gralloc_module_t *gr_mod;
    const struct hwc_module_t *hwc_mod;
    framebuffer_device_t *fb = NULL;
    alloc_device_t *alloc;
    str *shared=0;

    printf("start #############################################\n");
    shared=prt_shared(shared);
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &mod) != 0) {
        LOG_FATAL("cannot open gralloc module\n");
        return 1;
    }
    prt_module(mod);
    gr_mod = (const struct gralloc_module_t *) mod;
    shared=prt_shared(shared);

    if (hw_get_module(HWC_HARDWARE_MODULE_ID, &mod) != 0) {
        printf("cannot open hw composer module\n");
    } else {
        prt_module(mod);
        hwc_mod = (const struct hwc_module_t *) mod;
    }

    if (framebuffer_open(&gr_mod->common, &fb)) {
        printf("cannot open fb device\n");
        return 2;
    }
    prt_fb(fb);
    shared=prt_shared(shared);

    if (gralloc_open(&gr_mod->common, &alloc)) {
        printf("cannot open alloc device\n");
        return 3;
    }
    prt_alloc(alloc);
    shared=prt_shared(shared);

    int usage = GRALLOC_USAGE_HW_FB |
            GRALLOC_USAGE_HW_COMPOSER |
            GRALLOC_USAGE_HW_RENDER |
            GRALLOC_USAGE_SW_WRITE_OFTEN;
    int stride0,stride1;
    buffer_handle_t h0,h1;

    if (alloc->alloc(alloc, fb->width, fb->height, fb->format, usage, &h0,
                     &stride0)) {
        printf("alloc 0 failed\n");
        return 4;
    }
    if (alloc->alloc(alloc, fb->width, fb->height, fb->format, usage, &h1,
                     &stride1)) {
        printf("alloc 1 failed\n");
        return 4;
    }
    printf("stride %d %d\n", stride0, stride1);

    void *p0, *p1;
    if (gr_mod->lock(gr_mod, h0, GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0,
                     fb->width, fb->height, &p0)) {
        printf("lock 0 failed\n");
        return 5;
    }
    if (gr_mod->lock(gr_mod, h1, GRALLOC_USAGE_SW_WRITE_OFTEN, 0, 0,
                     fb->width, fb->height, &p1)) {
        printf("lock 1 failed\n");
        return 5;
    }
    printf("p0 %p\n", p0);
    printf("p1 %p\n", p1);

    uint32_t *x0=(uint32_t *)p0;
    uint32_t *x1=(uint32_t *)p1;

    memset(p0, 50, stride0 * fb->height * 4);
    memset(p1, 200, stride1 * fb->height * 4);

    // 0x00RRGGBB
    for (unsigned i=0;i<stride0 * fb->height;++i) x0[i]=0x0011ff11;
    for (unsigned i=0;i<stride1 * fb->height;++i) x1[i]=0x0011ff11;

    for (int j=0; j<99; ) {

        target(x0, j/10., fb->width, fb->height);
        j++;
        if (fb->post(fb, h0)) printf("post 0 failed\n");

        target(x1, j/10., fb->width, fb->height);
        j++;
        if (fb->post(fb, h1)) printf("post 1 failed\n");
    }

    gr_mod->unlock(gr_mod, h0);
    gr_mod->unlock(gr_mod, h1);

    if (alloc->free(alloc, h0)) printf("free failed\n");
    if (alloc->free(alloc, h1)) printf("free failed\n");

    return 0;
}
