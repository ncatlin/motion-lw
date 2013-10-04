/*
gutting this file was quicker than removing the things that required this structure
 */
typedef struct netcam_image_buff {
    char *ptr;
    int content_length;
    size_t size;                    /* total allocated size */
    size_t used;                    /* bytes already used */
    struct timeval image_time;      /* time this image was received */
} netcam_buff;
typedef netcam_buff *netcam_buff_ptr;