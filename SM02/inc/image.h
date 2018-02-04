#ifndef __IMAGE_H__
#define __IMAGE_H__

struct image_t;

struct image_t* loadImage(const char* filename);

void drawImage(const struct image_t* img);

#endif // __IMAGE_H__
