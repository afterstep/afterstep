#ifndef GENERAL_H
#define GENERAL_H

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
