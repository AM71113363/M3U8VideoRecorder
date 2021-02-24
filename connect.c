#include "library/mbedtls/config.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2tcpip.h>

WSADATA wsaData;
void AddQuality(UCHAR*);

#include "library/mbedtls/net_sockets.h"
#include "library/mbedtls/ssl.h"
#include "library/mbedtls/entropy.h"
#include "library/mbedtls/ctr_drbg.h"
#include "library/mbedtls/debug.h"
#include "library/mbedtls/error.h"
#include "library/mbedtls/certs.h"

#define BUFFER_SIZE 4*1024


static void my_debug( void *ctx, int level,  const char *file, int line, const char *str ){ }
static int initLib = 0;

static mbedtls_ctr_drbg_context ctr_drbg;  
static mbedtls_x509_crt cacert;  
static mbedtls_entropy_context entropy; 
static mbedtls_ssl_config conf;


//Gracefully close the connection version 2020
void secure_close( int *fd )
{
     int ret; 
     int error_code, opt_len;    
     if(*fd < 1)
        return;  
    error_code = 0;
    opt_len = sizeof(error_code);
    ret=getsockopt( *fd, SOL_SOCKET, SO_ERROR, (char *)&error_code, &opt_len);
    if(ret!=0)
        return;
	if(error_code == WSAENOTSOCK)
        return;
    if(error_code == WSAECONNRESET)
	    return;
   closesocket( *fd );
}
//-1 error with this socket
//0 timeout
//1 ok have data
int net_isrecv(int fd,fd_set *read_fds, int sec )
{
    int ret;
    struct timeval tv; 
    if(fd <1)
      return -1;
    FD_ZERO( read_fds );
    FD_SET( fd, read_fds );
    tv.tv_sec  = sec;
    tv.tv_usec = 0;
    ret = select(fd+1, read_fds, NULL, NULL, &tv );
    if(ret == SOCKET_ERROR)
       return -1; //error
    if(FD_ISSET( fd, read_fds ))
        return 1; //ok have data
  return 0;
}

//-1 host dont exists
//0 no connect for this host in this port
//1 ok,socket valid,call closesocket after use
int SOCK_CONNECT(int *fd,UCHAR *host, UCHAR *port)
{
    struct addrinfo hints, *addr_list, *cur;

    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if( getaddrinfo( host, port, &hints, &addr_list ) != 0 )
       return -1;
    for( cur = addr_list; cur != NULL; cur = cur->ai_next )
    {
        *fd = socket( cur->ai_family, cur->ai_socktype, cur->ai_protocol );
        if( *fd < 1 ){ continue; }
        if(connect( *fd, cur->ai_addr, cur->ai_addrlen ) == 0)
        {
            freeaddrinfo( addr_list );
            return 1;
        }
        closesocket(*fd);
    }
    freeaddrinfo( addr_list );
    return 0;
}


BOOL StartLib()
{
    int ret;
    if(initLib == 1)
       return TRUE; 
    WSAStartup( MAKEWORD(2,0), &wsaData );
    mbedtls_ctr_drbg_init( &ctr_drbg );     
    mbedtls_x509_crt_init( &cacert );  
    mbedtls_entropy_init( &entropy );    
    mbedtls_ssl_config_init( &conf );
    while(1) 
    {  
         ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, "VideoRecorder",9 ); 
         if(ret != 0)
            break;     
         ret = mbedtls_x509_crt_parse( &cacert,mbedtls_test_ca_crt_ec_pem, mbedtls_test_ca_crt_ec_pem_len);
         if(ret < 0)
            break;
         ret = mbedtls_ssl_config_defaults( &conf,0,0,0);
         if(ret != 0)
            break;
         mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
         mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
         mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );
         mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );
         initLib = 1;
         return TRUE;
    }
     
return FALSE;
}


void FreeNetSSL(int *fd, mbedtls_ssl_context *ssl)
{
       secure_close(fd); mbedtls_ssl_free(ssl);
}


//-1xsleep = NOT 200 OK stop
//return received length
int ReadFromSock(int fd,mbedtls_ssl_context *ssl,UCHAR *buffer,int bufferLen)
{
    int ret,rest,AllLen=0;
    UCHAR *q = buffer;
    rest = bufferLen;
    fd_set read_fds;
    while(1)
    {
        if(net_isrecv(fd,&read_fds,2 ) != 1)
           break;    
        ret = mbedtls_ssl_read( ssl, q, rest);
        if(ret <= 0)
           break;
        AllLen+=ret;
        if(AllLen >= bufferLen)  //overflow buffer
           break;
        q+=ret;
        rest-=ret;
    }
    return AllLen;
}


void GetInformation(UCHAR *LINK)
{
    UCHAR host[64];    UCHAR *P,*H;
    int ret;
    UCHAR buf[1024];    
    mbedtls_net_context server_fd;
    mbedtls_ssl_context ssl;  

    int len,AllDataLen;

    UCHAR Tmpbuf[BUFFER_SIZE];

    if(StartLib() == FALSE)
    { 
        AddQuality("AUTO");
        return;
    } 
    
    H=LINK;
    H+=8;
    P=strchr(H,'/');
    if(!P)
    { 
      AddQuality("AUTO");
      return;
    }
    memset(host,0,64);
    snprintf(host,(UINT)(P-H),"%s\0",H);
    
    len= sprintf(buf,"GET %s HTTP/1.0\r\nHost: %s\r\nAccept: text/html\r\nConnection: close\r\n\r\n\0",P,host);	

    mbedtls_net_init( &server_fd ); 
    mbedtls_ssl_init( &ssl );
            
    if(SOCK_CONNECT(&server_fd.fd ,host, "443") != 1)
    {
        FreeNetSSL(&server_fd.fd, &ssl ); 
        AddQuality("AUTO");
        return;
    }
    if(mbedtls_ssl_setup( &ssl, &conf ) != 0 )
    {
		FreeNetSSL(&server_fd.fd, &ssl ); 
		AddQuality("AUTO");
		return;
    } 
    if(mbedtls_ssl_set_hostname( &ssl, host) != 0 )
    {
		FreeNetSSL(&server_fd.fd, &ssl ); 
		AddQuality("AUTO");
		return;
	}
	mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL );
    if(mbedtls_ssl_handshake( &ssl ) != 0 )
    {
		FreeNetSSL(&server_fd.fd, &ssl ); 
        AddQuality("AUTO");
        return;
    }
    ret = mbedtls_ssl_write( &ssl, buf, len );
	if(ret <= 0 )
	{
		FreeNetSSL(&server_fd.fd, &ssl ); 
		AddQuality("AUTO");
		return;
    }
	memset(Tmpbuf,0,BUFFER_SIZE);
	AllDataLen = ReadFromSock(server_fd.fd, &ssl,Tmpbuf,BUFFER_SIZE);
    FreeNetSSL(&server_fd.fd, &ssl );  
    
    if(AllDataLen<=0)
    {
       	AddQuality("AUTO");
		return;
    }          
    //set all to lowercase
    for(len=0;len<AllDataLen;len++){ Tmpbuf[len] = tolower(Tmpbuf[len]); }
    H=strstr(Tmpbuf,"resolution=");
    while(H)
    {
        H+=11;
        P=strchr(H,0x0A);
        if(!P){ P=strchr(H,0x0D); }
        if(P)
        {
             memset(host,0,64);  
             snprintf(host,(UINT)(P-H),"%s\0",H);
             AddQuality(host);
        }
        else{ break; }
        H=strstr(P,"resolution=");
    }
    AddQuality("AUTO");
}

