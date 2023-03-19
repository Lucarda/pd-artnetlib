/*
Copyright (c) 2023 Lucas Cordiviola <lucarda27@hotmail.com>
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <stdio.h>
#include "m_pd.h"


void artnetfromarray_setup(void);
void artnetsend_setup(void);
void artnetudp_setup(void);
void artnetroute_setup(void);
void artnettoarray_setup(void);


typedef struct artnetlib {
    t_object t_ob;
} t_artnetlib;


t_class *artnetlib_class=NULL;


static void *artnetlib_new(void)
{
    t_artnetlib *x = (t_artnetlib *)pd_new(artnetlib_class);
    return (x);
}

void artnetlib_setup(void)
{
  
    post("---");
    post("  artnetlib v0.1.0");
    post("---");

    artnetlib_class = class_new(gensym("artnetlib"), artnetlib_new, 0, sizeof(t_artnetlib), 0, 0);

    artnetfromarray_setup();
    artnetsend_setup();
    artnetudp_setup();
    artnetroute_setup();
    artnettoarray_setup();
}
