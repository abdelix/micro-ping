#include <stdio.h>
#include <stdlib.h>

#include <netdb.h> 
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "include/libutil.h"

#define STR_LEN 120
#define BUFF_LEN 500
#define DATA_LEN 44
#define SECS_PER_REQ 1
#define OPTIONS "c:i:p:fah"
//Global variables

struct sockaddr_in dst;
int sock_fd;

//packets sent
int send_packs=0;
//packets received
int recv_packs;
//mean response time
float mean_rtt;
int count=0;
int interval=SECS_PER_REQ;
int  patern=0;
int flood=0;
int audio=0;



void usage()
{
  printf("Usage ping [-i interval] [-c count] [-p pattern] [-f] [-a] [-h]\n");
}

void statistics(int n)
{
  printf("\r\n--------- Statitics ----------\n");
  printf("Send %d packets , received %d : %.2f%% loss\nmean rtt : %.2f\n",send_packs,recv_packs,
    (100.0*(send_packs-recv_packs))/send_packs,mean_rtt );
  exit(0);
}
void proc_opts(char opt)
{
  switch(opt)
  {
    case 'c' :
      
      count=atoi(optarg);
      if(count==0) 
      {
	fprintf(stderr,"Error : se esperaba -c [numero de paquetes]\n");
	exit(1);
      }
      
      
      break;
      
    case 'i' :
      
      interval=atoi(optarg);
      if(interval==0) 
      {
	fprintf(stderr,"Error : se esperaba -i [segundos]\n");
	 exit(1);
      }
     
      
      break;
      
      case 'p' :
      
	patern=atoi(optarg);
       if(patern==0) 
      {
	fprintf(stderr,"Error : se esperaba -p [4 bytes hex(0x) or dec patern]\n");
      printf("Usage ping [-i interval] [-c count] [-p pattern] [-f] [-a] [-h]\n");
	exit(1);
      }
      
      break;
      
      case 'f' :
	flood=1;
	break;
      case 'a':
	audio=1;
	break;
      case 'h' :
	usage();
	exit(0);
	break;
      
      default :
	
	fprintf(stderr,"Error en los argumentos\n");
	exit(1);
	
      
  }
}




float ms_eleapsed(struct timeval *fin, struct timeval *inicio)
{
  float elapsed;
  elapsed=1000*(fin->tv_sec-inicio->tv_sec)+0.001*(fin->tv_usec- inicio->tv_usec);
 return elapsed;
 

  
}

///MANEJADOR DE ALARMA
void echo_rqst(int n)
{
  char buff[BUFF_LEN];
  static uint16_t req=0;
  struct icmphdr *icmp_h;
  
  //Configuramos los parametros icmp
///Ponemos a 0 el buffer

memset(buff,patern,BUFF_LEN);

//Relenamos la cabecera icmp
icmp_h=(struct icmphdr *)(buff);
 
icmp_h->type=ICMP_ECHO;
icmp_h->code=0;
icmp_h->checksum=0;

icmp_h->un.echo.id=31337;
icmp_h->un.echo.sequence=req++;

//ponemos en el paquete la hora del sistema para calcular el retardo
gettimeofday((struct timeval *)(buff+8),NULL);

//Calculamos el checksum
icmp_h->checksum=in_cksum((uint16_t*)buff,DATA_LEN);






if(sendto(sock_fd,buff,DATA_LEN,0,(struct sockaddr*)&dst,sizeof(dst))<0)
{
  perror("sendto");
}

else 
  
{
//temporizamos el siguiente envio
send_packs++;

}
  alarm(interval);
}



int main(int argc, char **argv) {
  
  
  char str[STR_LEN];
  char buf[BUFF_LEN];
  float rtt;
  struct hostent *host;
  struct icmphdr *rcv_h;
  struct iphdr *ip_h;
  struct timeval *tv_env;
  struct timeval tv_rec;
  struct sockaddr_in from;
  char opt;
  socklen_t len;
  
  
  
  int i;
  
  
 
  
  while((opt=getopt(argc,argv,OPTIONS))!=-1)
  {
    proc_opts(opt);
  }
  
   if((optind)!=argc-1)
  {
   
    fprintf(stderr,"ERROR:incorrect number of arguments \n");
    exit(1);
   
  }
  
  /** Convertimos nombre a ip **/
  host=gethostbyname(argv[optind]);
  if(!host)
  {
   perror("Error en gethostbyname ");
    exit(1);
  }
  memset(&dst,0,sizeof dst);
  dst.sin_family=AF_INET;
  dst.sin_port=htons(IPPROTO_ICMP);
  dst.sin_addr.s_addr=*((uint32_t *)host->h_addr_list[0]);

 ///Creamos un nuevo socket "crudo" y con protocolo ICMP
 sock_fd=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
 
 if(sock_fd<0)
 {
   perror("RAW socket creation failed  ");
   exit(1);
 }
///Enlazamos las señal alarm a echo_rqst
signal(SIGALRM,echo_rqst);

///enlazamos sigint a satatistics para mostrar la estadisticas

signal(SIGINT,statistics);



alarm(1);
ip_h=(struct iphdr *)buf;
printf("Ping a %s ... \n",inet_ntoa(dst.sin_addr));
do
{
  
  if(recvfrom(sock_fd,buf,BUFF_LEN,0,(struct sockaddr*)&from,&len)<0)
  {
    perror("recvfrom");
  }
  
 
  
  rcv_h=(struct icmphdr *)(buf+4*ip_h->ihl);
  gettimeofday(&tv_rec,NULL);
  tv_env=(struct timeval *)(buf+4*ip_h->ihl+8);
  if(rcv_h->type==ICMP_ECHOREPLY)
  {
    recv_packs++; 
    rtt=ms_eleapsed(&tv_rec,tv_env);
    mean_rtt=(rtt*recv_packs+rtt)/(recv_packs+1);
    if(audio) putchar('\a');
   printf("%d bytes from %s : icmp_req=%u\t ttl=%u\t time=%.4f ms !!\n",
	  ntohs(ip_h->tot_len),host->h_name,rcv_h->un.echo.sequence,
	  ip_h->ttl,rtt);
    if(recv_packs==count) 
   {
     kill(0,SIGINT);
   }
  }
  
   
   
//si alcanzamos el numero pedido acabamos , 0 enviar de forma indefinida
}while(1);
 

    
    return 0;
}
