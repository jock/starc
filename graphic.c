/*
 * Copyright(C) 2010-2011, Jocelyn Falempe jock@inpactien.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "graphic.h"
#include "pnglite.h"

GLuint grLoadTexture(char * filename) {
	png_t tex;
	unsigned char* data;
	GLuint textureHandle;

	png_init(0, 0);
	png_open_file_read(&tex, filename);

	data = (unsigned char*) malloc(tex.width * tex.height * tex.bpp);
	png_get_data(&tex, data);

	glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glEnable(GL_TEXTURE_2D);

	png_close_file(&tex);
	free(data);
	return textureHandle;
}

void grSetBlendAdd(GLuint text) {
	glBindTexture(GL_TEXTURE_2D, text);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

void grSetBlend(GLuint text) {
	glBindTexture(GL_TEXTURE_2D, text);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0,1.0,1.0,1.0);
}

void grBlitSquare(float x,float y, float size, float c) {
    glColor4f(c,c,0,c);
    glBegin(GL_QUADS);
    glTexCoord2f(0., 0.);
    glVertex2f(x - size, y);
    glTexCoord2f(0., 1.);
    glVertex2f(x , y + size);
    glTexCoord2f(1., 1.);
    glVertex2f(x + size, y);
    glTexCoord2f(1., 0.);
    glVertex2f(x , y - size);
    glEnd();
}

void grBlitRot(float x, float y, float r, float size) {
	float nr;
	float s;
	float a;
	float b;
	nr = r - M_PI_4;
	s = size * M_SQRT1_2;
	a = s * cos(nr);
	b = s * sin(nr);
	grBlit(x,y,a,b);
}

void grBlit(float x,float y, float a, float b) {
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex2f(x + a, y + b);
    glTexCoord2f(0.f, 1.f);
    glVertex2f(x + b , y - a);
    glTexCoord2f(1., 1.);
    glVertex2f(x - a, y - b);
    glTexCoord2f(1., 0.);
    glVertex2f(x - b, y + a);
    glEnd();
}

void grReshape(int width, int height) {
	int height_u;
	glViewport(0, 0, width, height);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    /* coordinate tricks : we want width to be 10000 unit */
    height_u = (WIDTH_UNIT * height) / width;
    glOrtho (0, WIDTH_UNIT, 0, height_u, 0, 1);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    printf("w,%d, h, %d\n",width,height);
}

