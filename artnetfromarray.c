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


typedef struct _artnetfromarray
{
    t_object  x_obj;
    t_symbol *x_arrayname;
    t_garray *x_a;
    t_word *x_vec;
    int x_vecsize;
    unsigned char x_listbytes[512];
    unsigned char x_listbyteslast[512];
    t_atom x_listatoms[512];
    t_outlet *x_outlet1;
    t_clock *x_clock;
    t_float x_polltime;
    t_float x_mfade;
    t_float x_fullframe;
    t_float x_outputalways;
    char x_polling;

} t_artnetfromarray;

t_class *artnetfromarray_class;


static void artnetfromarray_set(t_artnetfromarray *x, t_symbol *aname)
{
    x->x_arrayname = aname;
    return;
}


static void artnetfromarray_outputalways(t_artnetfromarray *x, t_float f)
{
    if (f > 0) x->x_outputalways = 1;
    else x->x_outputalways = 0;
    return;
}


static void artnetfromarray_fullframe(t_artnetfromarray *x, t_float f)
{
    if (f != 0) x->x_fullframe = 1;
    else x->x_fullframe = 0;
    memset(x->x_listbyteslast, 0, sizeof(x->x_listbyteslast));
    return;
}


static void artnetfromarray_mfade_set(t_artnetfromarray *x, t_float f)
{
    if (f > 1) x->x_mfade = 1;
    else if (f < 0) x->x_mfade = 0;
    else x->x_mfade = f;
    return;
}


static char artnetfromarray_change(t_artnetfromarray *x)
{   
    char r = -1;
    
    for (int i = 0; i < x->x_vecsize; i++) 
    {
        if (x->x_listbyteslast[i] != x->x_listbytes[i]) 
        {
            memcpy(x->x_listbyteslast, x->x_listbytes, sizeof(x->x_listbytes));
            r = 1;
            break;
        } 
        else r = 0;
    }
    return(r);    
}


static void artnetfromarray_poll(t_artnetfromarray *x, t_float f)
{
    x->x_polltime = f;
    memset(x->x_listbyteslast, 0, sizeof(x->x_listbyteslast));
    if (f != 0) 
    {
        x->x_polling = 1;
        clock_delay(x->x_clock, 0);
    }
    else 
    {
        x->x_polling = 0;
        clock_unset(x->x_clock);
    }
}


static void artnetfromarray_output(t_artnetfromarray *x)
{
    int i, mod;
    char output_opt;
        
    if (x->x_outputalways) output_opt = 1;
    else output_opt = artnetfromarray_change(x);    
    if (output_opt)
    {
        for (i = 0; i < x->x_vecsize; i++ )
        {
            SETFLOAT(&x->x_listatoms[i], (t_float)x->x_listbytes[i]);
        }
        if (!x->x_fullframe)
        {
            mod = x->x_vecsize % 2;
            if (mod) 
            {
                x->x_vecsize = x->x_vecsize + 1;
                SETFLOAT(&x->x_listatoms[x->x_vecsize - 1], 0);
            }
            outlet_list(x->x_outlet1, 0, x->x_vecsize, x->x_listatoms);
        } 
        else 
        {
            x->x_vecsize = 512;
            for (i; i < x->x_vecsize; i++ )
            {
                SETFLOAT(&x->x_listatoms[i], 0);
            }
            outlet_list(x->x_outlet1, 0, x->x_vecsize, x->x_listatoms);
        }     
    }    
    
}


static void artnetfromarray_tick(t_artnetfromarray *x)
{
    int i;
    t_float n;
  
    if (!(x->x_a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class))) 
    {
        pd_error(x,"artnetfromarray: no array '%s'", x->x_arrayname->s_name);
        pd_error(x,"artnetfromarray: stopped polling");
        return;
    }
  
    if (!garray_getfloatwords(x->x_a, &x->x_vecsize, &x->x_vec)) 
    {
        pd_error(x,"artnetfromarray: no array '%s'", x->x_arrayname->s_name);
        pd_error(x,"artnetfromarray: stopped polling");
        return;
    }
    
    if (x->x_vecsize > 512) x->x_vecsize = 512; 

    for (i = 0; i < x->x_vecsize; i++ )
    {
        if (x->x_vec->w_float > 1) n = 1;
        else if (x->x_vec->w_float < 0) n = 0;
        else n = x->x_vec->w_float;
        x->x_listbytes[i] = (unsigned char)((n * x->x_mfade) * 255);
        x->x_vec++;
    }    
    artnetfromarray_output(x);
    if (x->x_polling) clock_delay(x->x_clock, x->x_polltime);
}


static void artnetfromarray_list(t_artnetfromarray *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_float n;
    x->x_vecsize = argc;
    
    if (x->x_vecsize > 512) x->x_vecsize = 512;
    
    for (i = 0; i < x->x_vecsize; i++ )
    {
        if (atom_getfloatarg(i, argc, argv) > 1) n = 1;
        else if (atom_getfloatarg(i, argc, argv) < 0) n = 0;
        else n = atom_getfloatarg(i, argc, argv);
        x->x_listbytes[i] = (unsigned char)((n * x->x_mfade) * 255);
    }    
    artnetfromarray_output(x);    
}

static void artnetfromarray_free(t_artnetfromarray *x)
{
    clock_free(x->x_clock);  
}


static void *artnetfromarray_new(t_symbol *ary)
{
    t_artnetfromarray *x = (t_artnetfromarray *)pd_new(artnetfromarray_class);
    x->x_clock = clock_new(x, (t_method)artnetfromarray_tick);
    x->x_arrayname = ary;
    x->x_mfade = 1;   
    x->x_outlet1 = outlet_new(&x->x_obj, 0);
 
    return (void *)x;
}


void artnetfromarray_setup(void)
{
    artnetfromarray_class = class_new(gensym("artnetfromarray"),
                   (t_newmethod)artnetfromarray_new,
                   (t_method)artnetfromarray_free,
                   sizeof(t_artnetfromarray),       
                   CLASS_DEFAULT,
                   A_DEFSYM,
                   0);

    class_addbang(artnetfromarray_class, (t_method)artnetfromarray_tick);
    class_addlist(artnetfromarray_class, artnetfromarray_list);
    class_addmethod(artnetfromarray_class, (t_method)artnetfromarray_set,  gensym("set"),  A_DEFSYM, 0);  
    class_addmethod(artnetfromarray_class, (t_method)artnetfromarray_poll, gensym("poll"), A_FLOAT, 0);
    class_addmethod(artnetfromarray_class, (t_method)artnetfromarray_outputalways, gensym("outputalways"), A_FLOAT, 0);
    class_addmethod(artnetfromarray_class, (t_method)artnetfromarray_mfade_set, gensym("mainfade"), A_FLOAT, 0);
    class_addmethod(artnetfromarray_class, (t_method)artnetfromarray_fullframe, gensym("fullframe"), A_FLOAT, 0);
}
