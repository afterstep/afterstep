#ifndef GENERAL_H
#define GENERAL_H

typedef struct ColorPair
  {
    unsigned long fore;
    unsigned long back;
  }
ColorPair;

extern char *MyName;
extern Display *dpy;
extern int screen;

typedef struct
  {
    int flags, x, y;
    unsigned width, height;
  }
MyGeometry;

#endif /* GENERAL_H */
