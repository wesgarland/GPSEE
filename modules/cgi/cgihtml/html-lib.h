/* html-lib.h - header file for html-lib.c
   Eugene Kim, <eekim@eekim.com>
   $Id: html-lib.h,v 1.3 2010/12/02 21:59:42 wes Exp $

   Copyright (C) 1996 Eugene Eric Kim
   All Rights Reserved
*/

#ifndef _HTML_LIB
#define _HTML_LIB 1

void html_header(void);
void mime_header(char *mime);
void nph_header(char *status);
void show_html_page(char *loc);
void http_status(char *status);
void pragma(char *msg);
void set_cookie(char *name, char *value, char *expires, char *path,
		char *domain, short secure);
void html_begin(char *title);
void html_end(void);

/* better to do printf inside of function, or return string? */
void h1(char *header);
void h2(char *header);
void h3(char *header);
void h4(char *header);
void h5(char *header);
void h6(char *header);
void hidden(char *name, char *value);

#endif
