/*
Copyright (c) 2023 Lucas Cordiviola <lucarda27@hotmail.com>
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "m_pd.h"




typedef struct _artnettoarray {
    t_object  x_obj;
    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
} t_artnettoarray;


t_class *artnettoarray_class;



static void artnettoarray_list(t_artnettoarray *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_float *array;
    t_atom *out;
    unsigned char lenhi = atom_getfloatarg(16, argc, argv);
    unsigned char lenlow = atom_getfloatarg(17, argc, argv);
    
    int len = lenhi << 8 | lenlow;
    
    array = getbytes(len*sizeof(t_float));
    
    for (i=0; i < len; i++)
    {
        array[i] = atom_getfloatarg(18+i, argc, argv) / 255.;
    }
    
    out = getbytes(len*sizeof(t_atom));
    
    for (i=0; i < len; i++)
    {
        SETFLOAT(&out[i], array[i]);
    }
    outlet_float(x->x_outlet1, (t_float)len);
    outlet_list(x->x_outlet0, &s_list, len, out);
   
    return;
}


static void artnettoarray_free(t_artnettoarray *x)
{

}


static void *artnettoarray_new(void)
{
    t_artnettoarray *x = (t_artnettoarray *)pd_new(artnettoarray_class);
    x->x_outlet0 = outlet_new(&x->x_obj, 0);
    x->x_outlet1 = outlet_new(&x->x_obj, 0);
 
    return (void *)x;
}


void artnettoarray_setup(void)
{
    artnettoarray_class = class_new(gensym("artnettoarray"),
                    (t_newmethod)artnettoarray_new,
                    (t_method)artnettoarray_free,
                    sizeof(t_artnettoarray),
                    CLASS_DEFAULT,
                    0);
                    
    class_addlist(artnettoarray_class, artnettoarray_list);
}