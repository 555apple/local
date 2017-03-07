#ifndef CLICK_TUNNEL_TBG_HH
#define CLICK_TUNNEL_TBG_HH
#include <click/element.hh>
//apple/t
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <malloc.h>
#include <memory.h>
#include <linux/sockios.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h> //sleep（）函数所需的头文件
 #include <click/string.hh>
#include <click/hashmap.hh>
#include <fstream>//文件操作
//
CLICK_DECLS
//*************************数据处理部分*************************
//***************************正向处理***************************
class TBG_CoLoR_Decapsulation : public Element{ public:

  TBG_CoLoR_Decapsulation();
  ~TBG_CoLoR_Decapsulation();

  const char *class_name() const	{ return "TBG_CoLoR_Decapsulation"; }
  const char *port_count() const	{ return "1/-"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};
//***************************反向处理***************************
class TBG_CoLoR_Encapsulation : public Element{ public:

 TBG_CoLoR_Encapsulation();
  ~TBG_CoLoR_Encapsulation();

  const char *class_name() const  { return "TBG_CoLoR_Encapsulation"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};

//*********************更新模块部分****************************
//********************更新包的处理*****************************
class Updatepid_TBG_Process : public Element
{
public:

  Updatepid_TBG_Process();
  ~Updatepid_TBG_Process();

  const char *class_name() const  { return "Updatepid_TBG_Process"; }
  const char *port_count() const  { return "1/-"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};
//***************发送更新包**************************
class Updatepid_TBG_Send : public Element
{
public:

  Updatepid_TBG_Send();
  ~Updatepid_TBG_Send();

  const char *class_name() const  { return "Updatepid_TBG_Send"; }
  const char *port_count() const  { return "1/2"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

//*************************隧道维护部分*************************
class TBG_MAINT : public Element{ public:

  TBG_MAINT();
  ~TBG_MAINT();

  const char *class_name() const  { return "TBG_MAINT"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};

CLICK_ENDDECLS
#endif
