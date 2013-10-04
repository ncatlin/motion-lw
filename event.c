/*
    event.c

    Generalised event handling for motion

    Copyright Jeroen Vreeken, 2002
    This software is distributed under the GNU Public License Version 2
    see also the file 'COPYING'.

*/

#include "ffmpeg.h"    /* must be first to avoid 'shadow' warning */
#include "picture.h"    /* already includes motion.h */
#include "event.h"
#if !defined(BSD) 
#include "video.h"
#endif

/*
 *    Various functions (most doing the actual action)
 */


/* 
 *    Event handlers
 */

static void event_newfile(struct context *cnt ATTRIBUTE_UNUSED,
            int type ATTRIBUTE_UNUSED, unsigned char *dummy ATTRIBUTE_UNUSED,
            char *filename, void *ftype, struct tm *tm ATTRIBUTE_UNUSED)
{
    motion_log(2, 0, "path:%s", filename);
}


const char *imageext(struct context *cnt)
{
    if (cnt->conf.ppm)
        return "ppm";
    return "jpg";
}

static void event_image_detect(struct context *cnt, int type ATTRIBUTE_UNUSED,
        unsigned char *newimg, char *dummy1 ATTRIBUTE_UNUSED,
        void *dummy2 ATTRIBUTE_UNUSED, struct tm *currenttime_tm)
{
    char fullfilename[PATH_MAX];
    char filename[PATH_MAX];
    
    //if (cnt->new_img & NEWIMG_ON) {
    if (cnt->event_nr != cnt->prev_event){
        const char *jpegpath;

        /* conf.jpegpath would normally be defined but if someone deleted it by control interface
           it is better to revert to the default than fail */
        if (cnt->conf.jpegpath)
            jpegpath = cnt->conf.jpegpath;
        else
            jpegpath = DEF_JPEGPATH;
            
        mystrftime(cnt, filename, sizeof(filename), jpegpath, currenttime_tm, NULL, 0);
        snprintf(fullfilename, PATH_MAX, "%s/%s.%s", cnt->conf.filepath, filename, imageext(cnt));
        put_picture(cnt, fullfilename, newimg, FTYPE_IMAGE);
    }
}

static void event_imagem_detect(struct context *cnt, int type ATTRIBUTE_UNUSED,
            unsigned char *newimg ATTRIBUTE_UNUSED, char *dummy1 ATTRIBUTE_UNUSED,
            void *dummy2 ATTRIBUTE_UNUSED, struct tm *currenttime_tm)
{
    struct config *conf=&cnt->conf;
    char fullfilenamem[PATH_MAX];
    char filename[PATH_MAX];
    char filenamem[PATH_MAX];

    if (conf->motion_img) {
        const char *jpegpath;

        /* conf.jpegpath would normally be defined but if someone deleted it by control interface
           it is better to revert to the default than fail */
        if (cnt->conf.jpegpath)
            jpegpath = cnt->conf.jpegpath;
        else
            jpegpath = DEF_JPEGPATH;
            
        mystrftime(cnt, filename, sizeof(filename), jpegpath, currenttime_tm, NULL, 0);
        /* motion images gets same name as normal images plus an appended 'm' */
        snprintf(filenamem, PATH_MAX, "%sm", filename);
        snprintf(fullfilenamem, PATH_MAX, "%s/%s.%s", cnt->conf.filepath, filenamem, imageext(cnt));

        put_picture(cnt, fullfilenamem, cnt->imgs.out, FTYPE_IMAGE_MOTION);
    }
}

static void event_image_snapshot(struct context *cnt, int type ATTRIBUTE_UNUSED,
            unsigned char *img, char *dummy1 ATTRIBUTE_UNUSED,
            void *dummy2 ATTRIBUTE_UNUSED, struct tm *currenttime_tm)
{
    char fullfilename[PATH_MAX];

    if (strcmp(cnt->conf.snappath, "lastsnap")) {
        char filename[PATH_MAX];
        char filepath[PATH_MAX];
        char linkpath[PATH_MAX];
        const char *snappath;
        /* conf.snappath would normally be defined but if someone deleted it by control interface
           it is better to revert to the default than fail */
        if (cnt->conf.snappath)
            snappath = cnt->conf.snappath;
        else
            snappath = DEF_SNAPPATH;
            
        mystrftime(cnt, filepath, sizeof(filepath), snappath, currenttime_tm, NULL, 0);
        snprintf(filename, PATH_MAX, "%s.%s", filepath, imageext(cnt));
        snprintf(fullfilename, PATH_MAX, "%s/%s", cnt->conf.filepath, filename);
        put_picture(cnt, fullfilename, img, FTYPE_IMAGE_SNAPSHOT);

        /* Update symbolic link *after* image has been written so that
           the link always points to a valid file. */
        snprintf(linkpath, PATH_MAX, "%s/lastsnap.%s", cnt->conf.filepath, imageext(cnt));
        remove(linkpath);

        if (symlink(filename, linkpath)) {
            motion_log(LOG_ERR, 1, "Could not create symbolic link [%s]", filename);
            return;
        }
    } else {
        snprintf(fullfilename, PATH_MAX, "%s/lastsnap.%s", cnt->conf.filepath, imageext(cnt));
        remove(fullfilename);
        put_picture(cnt, fullfilename, img, FTYPE_IMAGE_SNAPSHOT);
    }

    cnt->snapshot = 0;
}

/*  
 *    Starting point for all events
 */

struct event_handlers {
    int type;
    event_handler handler;
};

struct event_handlers event_handlers[] = {
    {
    EVENT_FILECREATE,
    event_newfile
    },
    {
    EVENT_IMAGE_DETECTED,
    event_image_detect
    },
    {
    EVENT_IMAGEM_DETECTED,
    event_imagem_detect
    },
    {
    EVENT_IMAGE_SNAPSHOT,
    event_image_snapshot
    },
    {0, NULL}
};


/* The event functions are defined with the following parameters:
 * - Type as defined in event.h (EVENT_...)
 * - The global context struct cnt
 * - image - A pointer to unsigned char as used for images
 * - filename - A pointer to typically a string for a file path
 * - eventdata - A void pointer that can be cast to anything. E.g. FTYPE_...
 * - tm - A tm struct that carries a full time structure
 * The split between unsigned images and signed filenames was introduced in 3.2.2
 * as a code reading friendly solution to avoid a stream of compiler warnings in gcc 4.0.
 */
void event(struct context *cnt, int type, unsigned char *image, char *filename, void *eventdata, struct tm *tm)
{
    int i = -1;

    while (event_handlers[++i].handler) {
        if (type & event_handlers[i].type)
            event_handlers[i].handler(cnt, type, image, filename, eventdata, tm);
    }
}
