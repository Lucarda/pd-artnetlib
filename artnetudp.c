/*
Copyright (c) 2023 Lucas Cordiviola <lucarda27@hotmail.com>
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#else  

#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <arpa/inet.h> 
#include <ifaddrs.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include "m_pd.h"
#include "s_stuff.h"

#define debug(fmt, args...) fprintf(stderr, fmt, ##args);

#define ARTNETPORT 6454


static unsigned char POLLREPLY[239] = {
0x41, 0x72, 0x74, 0x2d, 0x4e, 0x65, 0x74, 0x00, 0x00, 0x21, 
0x7f, 0x00, 0x00, 0x01, 0x36, 0x19, 0x04, 0x20, 0x00, 0x00, 
0xff, 0xff, 0x00, 0xf0, 0xff, 0xff, 0x50, 0x75 ,0x72 ,0x65, 
0x2d ,0x44 ,0x61 ,0x74 ,0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x50 ,0x64 ,0x20, 0x75, 0x73, 0x69, 
0x6e, 0x67, 0x20, 0x61, 0x72, 0x74, 0x6e, 0x65, 0x74, 0x6c,
0x69, 0x62, 0x20, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x61,
0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x27, 0x22, 0x33, 0x44, 0x55, 0x66, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};


unsigned char ARTNETPOLL[14] = {0x41, 0x72, 0x74, 0x2d, 
0x4e, 0x65, 0x74, 0x00, 0x00, 0x20, 0x00, 0x0e, 0x02, 0x00
};


typedef struct _artnetudp {
    t_object  x_obj;
    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    t_outlet *x_outlet2;
    t_outlet *x_outlet3;
    struct sockaddr_in x_servaddr, x_sendtoaddr, x_recvfromaddr, x_pollreplyaddr;
    int x_sock;
    char x_goodsocket;
    char *x_buf;
    int x_buflength;
    char x_recbuf[1024];
    t_atom x_pollreply[3];
    t_atom x_pollipallnics[10];
    unsigned char x_pollipallnicscount;
    t_symbol *x_currserveraddress;
    t_symbol *x_currserveraddresstemp;
    uint32_t x_sendtoaddresstemp;
    unsigned char x_artnetpoll[14];
    unsigned char x_pollreplybuf[239];
    t_atom  x_artnetmsg[600];
    unsigned char x_artnetmsg_last[600];
    uint16_t x_artnetmsg_lenght;
    t_clock *x_clock;
    t_float x_outputalways;
} t_artnetudp;


t_class *artnetudp_class;


static char artnetudp_dmxchange(t_artnetudp *x)
{   
    char r = -1;
    
    // start at 18 to only compare DMX values
    
    for (int i = 18; i < x->x_artnetmsg_lenght; i++) {
        if (x->x_artnetmsg_last[i] != (unsigned char)x->x_recbuf[i]) {
            memcpy(x->x_artnetmsg_last, x->x_recbuf, x->x_artnetmsg_lenght);
            r = 1;
            break;
        } else {
            r = 0;
        }
    }
    return(r);    
}


static void artnetudp_listen(t_artnetudp *x)
{
    int n, i, validartnet;
    char ip[INET_ADDRSTRLEN];
    char shortname[18];
    char longname[64];
    char foo[8];
    char output_opt;
    struct sockaddr_in addr; 
    socklen_t addrlen = sizeof(addr); 
    uint32_t ourAddr, recvaddr;
    socklen_t len;
    len = sizeof(x->x_recvfromaddr);

    memset(&x->x_recvfromaddr, 0, sizeof(x->x_recvfromaddr)); 
    if(x->x_goodsocket)
    {
        n = recvfrom(x->x_sock, (char *)x->x_recbuf, 1024,
        0, ( struct sockaddr *) &x->x_recvfromaddr,
                &len);
        x->x_recbuf[n] = '\0';
        x->x_artnetmsg_lenght = n;            
    }
    strncpy(foo, x->x_recbuf, 8);       
    validartnet = strcmp(foo, "Art-Net"); 
    if (validartnet != 0)
    {
        goto NOTARTNET;
    } 
    if (x->x_recbuf[8] == 0x00 && x->x_recbuf[9] == 0x20) // RECEIVE_POLL
    {
        /* Find our address */ 
        if (getsockname(x->x_sock, (struct sockaddr *)&addr, (socklen_t *)&addrlen))
           pd_error(x, "artnetudp: problem getting our socket address and port");
        else
        {
            ourAddr = ntohl(addr.sin_addr.s_addr);
            // stick our ip adress to the poll reply
            x->x_pollreplybuf[10] = (ourAddr >> 24) & 0xff; 
            x->x_pollreplybuf[11] = (ourAddr >> 16) & 0xff;
            x->x_pollreplybuf[12] = (ourAddr >> 8) & 0xff;
            x->x_pollreplybuf[13] = (ourAddr & 0xff);
        }
        recvaddr = ntohl(x->x_recvfromaddr.sin_addr.s_addr);
        // change sendto address to unicast reply to the sender
        x->x_pollreplyaddr.sin_addr.s_addr = htonl(recvaddr);
        sendto(x->x_sock, x->x_pollreplybuf, sizeof(x->x_pollreplybuf),
            0, (const struct sockaddr *) &x->x_pollreplyaddr,
            sizeof(x->x_pollreplyaddr));                      
    }
    else if (x->x_recbuf[8] == 0x00 && x->x_recbuf[9] == 0x21) // RECEIVE_REPLY
    {
        inet_ntop(AF_INET, &x->x_recvfromaddr.sin_addr,
            ip, INET_ADDRSTRLEN);
        strncpy(shortname, x->x_recbuf+26, 18-1);
        strncpy(longname, x->x_recbuf+44, 64-1);
        SETSYMBOL(x->x_pollreply+0, gensym(ip));                       
        SETSYMBOL(x->x_pollreply+1, gensym(shortname));
        SETSYMBOL(x->x_pollreply+2, gensym(longname));            
        outlet_list(x->x_outlet1, 0, 3, x->x_pollreply);                    
    }
    else if(x->x_recbuf[8] == 0x00 && x->x_recbuf[9] == 0x50) // RECEIVE_DMX
    {            
        if (x->x_outputalways) output_opt = 1;
        else output_opt = artnetudp_dmxchange(x);    
        if (output_opt)
        {
            for (i = 0; i < x->x_artnetmsg_lenght; i++ )
            {
                SETFLOAT(&x->x_artnetmsg[i], (t_float)(unsigned char)x->x_recbuf[i]);
            }                
            outlet_list(x->x_outlet0, 0, x->x_artnetmsg_lenght, x->x_artnetmsg);
        }                        
    }   
    else // RECEIVE_ELSE
    {            
        for (i = 0; i < x->x_artnetmsg_lenght; i++ )
        {
            SETFLOAT(&x->x_artnetmsg[i], (t_float)(unsigned char)x->x_recbuf[i]);
        }            
        outlet_list(x->x_outlet2, 0, x->x_artnetmsg_lenght, x->x_artnetmsg);
    } 
    NOTARTNET:       
    memset(&x->x_recbuf , 0, sizeof(x->x_recbuf));
}


static void artnetudp_do_usenic(t_artnetudp *x, t_symbol *ipsymbol, char dobind) 
{
    int opt = 1; 
    
    if (x->x_goodsocket)
    {
        sys_rmpollfn(x->x_sock);
        sys_closesocket(x->x_sock);
    }
    if ( (x->x_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    { 
        pd_error(x,"artnetudp: socket creation failed");
        sys_closesocket(x->x_sock);
        x->x_goodsocket = 0;
        return; 
    }    
    if (setsockopt(x->x_sock, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) 
    {
        pd_error(x,"artnetudp: setsockopt failed");
        sys_closesocket(x->x_sock);
        x->x_goodsocket = 0;
        return;
    } 
    memset(&x->x_servaddr, 0, sizeof(x->x_servaddr));
    x->x_servaddr.sin_family = AF_INET;
    x->x_servaddr.sin_addr.s_addr = inet_addr(ipsymbol->s_name);
    //store current address somewhere
    x->x_currserveraddress = ipsymbol;    
    x->x_servaddr.sin_port = htons(ARTNETPORT);    
    if (dobind)
    {
        if (bind(x->x_sock, (struct sockaddr*)&x->x_servaddr, sizeof(x->x_servaddr)) < 0)
        {
            pd_error(x,"artnetudp: socket bind failed.");
            sys_closesocket(x->x_sock);
            x->x_goodsocket = 0;
            return;
        }
    }
#ifdef _WIN32    
    ULONG NonBlock = 1;
    if (ioctlsocket(x->x_sock, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        pd_error(x,"artnetudp: ioctlsocket() failed with error %d\n", WSAGetLastError());
        sys_closesocket(x->x_sock);
        x->x_goodsocket = 0;
        return;
    }
#else
    fcntl(x->x_sock, F_SETFL, O_NONBLOCK);
    int broadcastEnable=1;
    setsockopt(x->x_sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));    
#endif
    x->x_goodsocket = 1;
    sys_addpollfn(x->x_sock, (t_fdpollfn)artnetudp_listen, x);

    return;
}


static void artnetudp_usenic(t_artnetudp *x, t_symbol *ipsymbol) 
{
    artnetudp_do_usenic(x, ipsymbol, 1);
}


static void artnetudp_targetip_init(t_artnetudp *x) 
{
    memset(&x->x_sendtoaddr, 0, sizeof(x->x_sendtoaddr));
    x->x_sendtoaddr.sin_family = AF_INET; 
    x->x_sendtoaddr.sin_addr.s_addr = inet_addr("192.168.1.1");
    x->x_sendtoaddr.sin_port = htons(ARTNETPORT);       
    return;
}


static void artnetudp_targetip(t_artnetudp *x, t_symbol *ipsymbol) 
{ 
    x->x_sendtoaddr.sin_addr.s_addr = inet_addr(ipsymbol->s_name);       
    return;
}


static void artnetudp_pollreplyaddr_init(t_artnetudp *x) 
{
    memset(&x->x_pollreplyaddr, 0, sizeof(x->x_pollreplyaddr));
    x->x_pollreplyaddr.sin_family = AF_INET;
    //this ip address will change in the reply
    x->x_pollreplyaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    x->x_pollreplyaddr.sin_port = htons(ARTNETPORT);    
    return;
}


static void artnetudp_localhost(t_artnetudp *x) 
{
    artnetudp_do_usenic(x, gensym("127.0.0.1"), 1);
    artnetudp_targetip(x, gensym("127.0.0.1"));    
}


static void artnetudp_outputalways(t_artnetudp *x, t_float f)
{
    if (f > 0) x->x_outputalways = 1;
    else x->x_outputalways = 0;
    return;
}


static void artnetudp_sendartnetpoll(t_artnetudp *x) 
{
    uint32_t currentaddr = ntohl(x->x_sendtoaddr.sin_addr.s_addr);

    uint32_t byte1 = (currentaddr >> 24);
    uint32_t byte2 = (currentaddr >> 16) & 0xff;
    uint32_t byte3 = (currentaddr >> 8)  & 0xff;
   
    uint32_t broadcastip =  (uint32_t)byte1 << 24 |
                            (uint32_t)byte2 << 16 |
                            (uint32_t)byte3 << 8  |
                            (uint32_t)255;

    // change send address to a broadcast ip
    x->x_sendtoaddr.sin_addr.s_addr = htonl(broadcastip);
    sendto(x->x_sock, x->x_artnetpoll, sizeof(x->x_artnetpoll),
        0, (const struct sockaddr *) &x->x_sendtoaddr,
            sizeof(x->x_sendtoaddr));
    // change back send address to where it was
    x->x_sendtoaddr.sin_addr.s_addr = htonl(currentaddr);
}


static void artnetudp_send(t_artnetudp *x) 
{   
    sendto(x->x_sock, x->x_buf, x->x_buflength,
        0, (const struct sockaddr *) &x->x_sendtoaddr,
            sizeof(x->x_sendtoaddr));
}


static void artnetudp_list(t_artnetudp *x, t_symbol *s, int argc, t_atom *argv) 
{
    int i;
    x->x_buf = getbytes(argc);
    x->x_buflength = argc;   
    for (i = 0; i < argc; i++)
    {
        ((unsigned char *)x->x_buf)[i] = atom_getfloatarg(i, argc, argv);
    }
    artnetudp_send(x);
}


static void artnetudp_nicinfo(t_artnetudp *x, char output) 
{
    t_atom nics[3];
    unsigned char niccount = 0;
    char ctl = 0;
    
#ifdef _WIN32
    DWORD asize = 20000;
    PIP_ADAPTER_ADDRESSES adapters;
    do {
        adapters = (PIP_ADAPTER_ADDRESSES)malloc(asize);
        if (!adapters) {
            pd_error(x,"artnetudp: Couldn't allocate %ld bytes for adapters.\n", asize);
            return;
        }
        int r = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, 0,
                adapters, &asize);
        if (r == ERROR_BUFFER_OVERFLOW) {
            pd_error(x,"artnetudp: GetAdaptersAddresses wants %ld bytes.\n", asize);
            free(adapters);
        } else if (r == ERROR_SUCCESS) {
            break;
        } else {
            pd_error(x,"artnetudp: Error from GetAdaptersAddresses: %d\n", r);
            free(adapters);
            return;
        }
    } while (!adapters);
    
    PIP_ADAPTER_ADDRESSES adapter = adapters;
    while (adapter) {
        //printf("\nAdapter name: %S\n", adapter->FriendlyName);        
        char *foo = (char *)malloc( sizeof(adapter->FriendlyName)+1 );
        size_t i;
        wcstombs_s(&i, foo, (size_t) sizeof(adapter->FriendlyName)+1,
               adapter->FriendlyName, (size_t)sizeof(adapter->FriendlyName) );
        SETSYMBOL(nics+0, gensym(foo));
        PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress;
        while (address) {
            //printf("\t%s",
            //        address->Address.lpSockaddr->sa_family == AF_INET ?
            //        "IPv4" : "IPv6");
            SETSYMBOL(nics+1, address->Address.lpSockaddr->sa_family == AF_INET ?
                    gensym("IPv4") : gensym("IPv6"));
            char ap[100];

            getnameinfo(address->Address.lpSockaddr,
                    address->Address.iSockaddrLength,
                    ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            //printf("\t%s\n", ap);
            SETSYMBOL(nics+2, gensym(ap));
            if(output) outlet_list(x->x_outlet3, 0, 3, nics);
            // for poll all nics
            if(address->Address.lpSockaddr->sa_family == AF_INET) ctl = 1;
            else ctl = 0;
            if(ctl)
            {
                SETSYMBOL(x->x_pollipallnics+niccount, gensym(ap));
                niccount++;
            }
            // </>for poll all nics           
            address = address->Next;
        }
        adapter = adapter->Next;
    }
    free(adapters);
#else    
    struct ifaddrs *addresses;

    if (getifaddrs(&addresses) == -1) {
        pd_error(x,"artnetudp: getifaddrs call failed\n");
        return;
    }    
    struct ifaddrs *address = addresses;
    while(address) {
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {
            //printf("%s\t", address->ifa_name);
            SETSYMBOL(nics+0, gensym(address->ifa_name));
            //printf("%s\t", family == AF_INET ? "IPv4" : "IPv6");
            SETSYMBOL(nics+1, family == AF_INET ? gensym("IPv4") : gensym("IPv6"));
            char ap[100];
            const int family_size = family == AF_INET ?
                sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            getnameinfo(address->ifa_addr,
                    family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            //printf("\t%s\n", ap);
            SETSYMBOL(nics+2, gensym(ap));
            if(output) outlet_list(x->x_outlet3, 0, 3, nics);
            // for poll all nics
            if(family == AF_INET) ctl = 1;
            else ctl = 0;
            if(ctl)
            {
                SETSYMBOL(x->x_pollipallnics+niccount, gensym(ap));
                niccount++;
            }
            // </>for poll all nics
        }
        address = address->ifa_next;
    }
    freeifaddrs(addresses);    
#endif
    x->x_pollipallnicscount = niccount;
}


static void artnetudp_nicinfo_msg(t_artnetudp *x) 
{
    artnetudp_nicinfo(x,1);
}


static void artnetudp_clock(t_artnetudp *x)
{
    artnetudp_usenic(x,x->x_currserveraddresstemp);
    x->x_sendtoaddr.sin_addr.s_addr = htonl(x->x_sendtoaddresstemp);
}


static void artnetudp_pollallnics(t_artnetudp *x)
{   
    unsigned char i;
    x->x_currserveraddresstemp = x->x_currserveraddress;
    x->x_sendtoaddresstemp = ntohl(x->x_sendtoaddr.sin_addr.s_addr);
    artnetudp_usenic(x,gensym("0.0.0.0"));
    artnetudp_nicinfo(x, 0);
    for (i=0; i < x->x_pollipallnicscount; i++)
    {
        artnetudp_targetip(x, atom_getsymbol(x->x_pollipallnics+i));
        artnetudp_sendartnetpoll(x);
    }
    clock_delay(x->x_clock, 2000);
}


static void artnetudp_free(t_artnetudp *x)
{   
    sys_rmpollfn(x->x_sock);
    sys_closesocket(x->x_sock);
    clock_free(x->x_clock);
}


static void *artnetudp_new(void)
{
    t_artnetudp *x = (t_artnetudp *)pd_new(artnetudp_class);
    x->x_outlet0 = outlet_new(&x->x_obj, 0);
    x->x_outlet1 = outlet_new(&x->x_obj, 0);
    x->x_outlet2 = outlet_new(&x->x_obj, 0);
    x->x_outlet3 = outlet_new(&x->x_obj, 0);
    memcpy(x->x_artnetpoll, ARTNETPOLL, 14);
    memcpy(x->x_pollreplybuf, POLLREPLY, 239);
    x->x_clock = clock_new(x, (t_method)artnetudp_clock);
    artnetudp_targetip_init(x);
    artnetudp_usenic(x,gensym("0.0.0.0"));
    artnetudp_pollreplyaddr_init(x);
    return (void *)x;
}


void artnetudp_setup(void)
{
    artnetudp_class = class_new(gensym("artnetudp"),
                    (t_newmethod)artnetudp_new,
                    (t_method)artnetudp_free,
                    sizeof(t_artnetudp),
                    CLASS_DEFAULT,
                    0);

    class_addlist(artnetudp_class, artnetudp_list);
    class_addmethod(artnetudp_class, (t_method)artnetudp_localhost, gensym("localhost"), 0);
    class_addmethod(artnetudp_class, (t_method)artnetudp_localhost, gensym("loopback"), 0);
    class_addmethod(artnetudp_class, (t_method)artnetudp_nicinfo_msg, gensym("nicinfo"), 0);
    class_addmethod(artnetudp_class, (t_method)artnetudp_usenic, gensym("usenic"),  A_DEFSYM, 0);
    class_addmethod(artnetudp_class, (t_method)artnetudp_targetip, gensym("targetip"),  A_DEFSYM, 0);
    class_addmethod(artnetudp_class, (t_method)artnetudp_sendartnetpoll, gensym("find_nodes"), 0);
    class_addmethod(artnetudp_class, (t_method)artnetudp_pollallnics, gensym("find_nodes_all_nics"), 0);
    class_addmethod(artnetudp_class, (t_method)artnetudp_outputalways, gensym("outputalways"), A_FLOAT, 0);
}
