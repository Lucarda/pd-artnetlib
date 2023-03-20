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




typedef struct _artnetroute {
    t_object  x_obj;
    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    unsigned char x_artnetphysical;
    unsigned char x_artnetuniverse[2];
} t_artnetroute;


t_class *artnetroute_class;



static void artnetroute_physical(t_artnetroute *x, t_float f)
{
    int n = (int)f;
    if (n > 255)
    {
        n = 255;
        pd_error(x,"physical can't be greater than 255. setting to 255");
    } 
    else if (n < 0)
    {
        n= 0;
        pd_error(x,"physical can't be smaller than 0. setting to 0");
    } 
    else 
    {    
        x->x_artnetphysical = (unsigned char)n;
    }
    return;    
}


static void artnetroute_universe(t_artnetroute *x, t_float f)
{
    int n = (int)f;
    if (n > 32768)
    {
        n = 0;
        pd_error(x,"universe can't be greater than 32768. setting to 0");
    } 
    else if (n < 0)
    {
        n= 0;
        pd_error(x,"universe can't be smaller than 0. setting to 0");
    } 
    else 
    {    
        x->x_artnetuniverse[0] = (unsigned char) n & 0xff;
        x->x_artnetuniverse[1] = (unsigned char)(n >> 8) & 0xff;
    }
    return;    
}


static void artnetroute_list(t_artnetroute *x, t_symbol *s, int argc, t_atom *argv)
{
    unsigned char phys = atom_getfloatarg(13, argc, argv);
    unsigned char univ0 = atom_getfloatarg(14, argc, argv);
    unsigned char univ1 = atom_getfloatarg(15, argc, argv);
    
    
    if (phys == x->x_artnetphysical && univ0 == x->x_artnetuniverse[0] && \
        univ1 == x->x_artnetuniverse[1])
    {
        outlet_list(x->x_outlet0, &s_list, argc, argv);
    }
    else
    {
        outlet_list(x->x_outlet1, &s_list, argc, argv);
    }    
    return;
}


static void artnetroute_free(t_artnetroute *x)
{

}


static void *artnetroute_new(t_symbol *s, int argc, t_atom *argv)
{
    t_artnetroute *x = (t_artnetroute *)pd_new(artnetroute_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("universe"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("physical"));
    x->x_outlet0 = outlet_new(&x->x_obj, 0);
    x->x_outlet1 = outlet_new(&x->x_obj, 0);
    
    /* check argument types */
    if (argc > 0)
    {
        if ((argc != 2) ||
            argv[0].a_type != A_FLOAT ||
            argv[1].a_type != A_FLOAT)
        {
            pd_error(x, "artnetroute: bad creation arguments. using defaults 0 0");
            artnetroute_physical(x, 0);
            artnetroute_universe(x, 0);
        }
        else
        {
            artnetroute_universe(x, argv[0].a_w.w_float);
            artnetroute_physical(x, argv[1].a_w.w_float);
        }
    }
 
    return (void *)x;
}


void artnetroute_setup(void)
{
    artnetroute_class = class_new(gensym("artnetroute"),
                    (t_newmethod)artnetroute_new,
                    (t_method)artnetroute_free,
                    sizeof(t_artnetroute),
                    0, A_GIMME, 0);

    class_addmethod(artnetroute_class, (t_method)artnetroute_universe, gensym("universe"), A_FLOAT, 0);
    class_addmethod(artnetroute_class, (t_method)artnetroute_physical, gensym("physical"), A_FLOAT, 0);
    class_addlist(artnetroute_class, artnetroute_list);
}