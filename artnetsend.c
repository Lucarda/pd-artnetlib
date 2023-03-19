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




typedef struct _artnetsend {
    t_object  x_obj;
    t_outlet *x_outlet1;
    t_atom x_listatoms[530];
    unsigned char x_artnetheader[12];
    unsigned char x_artnetseq;
    unsigned char x_artnetphysical;
    unsigned char x_artnetuniverse[2];
    unsigned char x_artnetpacketlenght[2];
    unsigned char x_artnetdata[512];
    int x_packetlen;
    int x_argc;
    t_atom *x_argv;


} t_artnetsend;


t_class *artnetsend_class;


static unsigned char artnetsend_seq(t_artnetsend *x)
{
    if (x->x_artnetseq > (unsigned char)255)
    {
        x->x_artnetseq = 0;
    } 
    else
    {
        x->x_artnetseq += 1;
    }
    return (x->x_artnetseq);    
}


static void artnetsend_physical(t_artnetsend *x, t_float f)
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


static void artnetsend_universe(t_artnetsend *x, t_float f)
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


static void artnetsend_packetlenght(t_artnetsend *x)
{
    int n = x->x_argc;
    if (n > 512)
    {
        n = 512;
        pd_error(x,"DMX data can't be greater than 512. setting to 512");
    } 
    else if (n < 2)
    {
        n= 2;
        pd_error(x,"DMX data can't be smaller than 2. setting to 2");
    } 
    else 
    {    
        x->x_artnetpacketlenght[1] = (unsigned char) n & 0xff;
        x->x_artnetpacketlenght[0] = (unsigned char)(n >> 8) & 0xff;;
    }
    return;    
}


static void artnetsend_send(t_artnetsend *x)
{
    
    int i;
    
    for (i = 0; i < (int)sizeof(x->x_artnetheader); i++)
    {
        SETFLOAT(&x->x_listatoms[i], (t_float)x->x_artnetheader[i]);
    }
    x->x_packetlen = i;
    
    for (i=0; i < 1; i++)
    {
        artnetsend_seq(x);
        SETFLOAT(&x->x_listatoms[i+x->x_packetlen], (t_float)x->x_artnetseq);
    }
    x->x_packetlen += i;
    for (i=0; i < 1; i++)
    {
        SETFLOAT(&x->x_listatoms[i+x->x_packetlen], (t_float)x->x_artnetphysical);
    }
    x->x_packetlen += i;
    for (i=0; i < 2; i++)
    {
        SETFLOAT(&x->x_listatoms[i+x->x_packetlen], (t_float)x->x_artnetuniverse[i]);
    }
    x->x_packetlen += i;

    for (i=0; i < 2; i++)
    {
        artnetsend_packetlenght(x);
        SETFLOAT(&x->x_listatoms[i+x->x_packetlen], (t_float)x->x_artnetpacketlenght[i]);
    }
    x->x_packetlen += i;
    for (i=0; i < x->x_argc; i++)
    {
        x->x_listatoms[i+x->x_packetlen] = x->x_argv[i];
    }
    x->x_packetlen += i;    
    outlet_list(x->x_outlet1, &s_list, x->x_packetlen, x->x_listatoms);
    
}


static void artnetsend_list(t_artnetsend *x, t_symbol *s, int argc, t_atom *argv)
{
    x->x_argc = argc;
    x->x_argv = argv;
    artnetsend_packetlenght(x);
    artnetsend_send(x);
}


static void artnetsend_free(t_artnetsend *x)
{

}


static void *artnetsend_new(t_symbol *s, int argc, t_atom *argv)
{
    t_artnetsend *x = (t_artnetsend *)pd_new(artnetsend_class);
    x->x_outlet1 = outlet_new(&x->x_obj, 0);
    unsigned char pre[12] = {0x41, 0x72, 0x74, 0x2d, 0x4e, 0x65,
                        0x74, 0x00, 0x00, 0x50, 0x00, 0x0e};
    memcpy(x->x_artnetheader, pre,12);
    
    /* check argument types */
    if (argc > 0)
    {
        if ((argc != 2) ||
            argv[0].a_type != A_FLOAT ||
            argv[1].a_type != A_FLOAT)
        {
            pd_error(x, "artnetsend: bad creation arguments. using defaults 0 0");
            artnetsend_physical(x, 0);
            artnetsend_universe(x, 0);
        }
        else
        {
            artnetsend_physical(x, argv[0].a_w.w_float);
            artnetsend_universe(x, argv[1].a_w.w_float);
        }
    }
    return (void *)x;
}


void artnetsend_setup(void)
{
    artnetsend_class = class_new(gensym("artnetsend"),
                    (t_newmethod)artnetsend_new,
                    (t_method)artnetsend_free,
                    sizeof(t_artnetsend),
                    0, A_GIMME, 0);

    class_addmethod(artnetsend_class, (t_method)artnetsend_universe, gensym("universe"), A_FLOAT, 0);
    class_addmethod(artnetsend_class, (t_method)artnetsend_physical, gensym("physical"), A_FLOAT, 0);
    class_addlist(artnetsend_class, artnetsend_list);
}
