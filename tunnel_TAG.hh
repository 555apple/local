#ifndef CLICK_TUNNEL_start1_HH
#define CLICK_TUNNEL_start1_HH
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
//*************************向RM请求*************************
class REQUST_TBG : public Element{ public:

  REQUST_TBG();
  ~REQUST_TBG();

  const char *class_name() const  { return "REQUST_TBG"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};
//*************************接收RM的应答*************************
class RECV_TBG : public Element{ public:

  RECV_TBG();
  ~RECV_TBG();

  const char *class_name() const  { return "RECV_TBG"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};
//*************************正向处理*************************
class CoLoR_Encapsulation : public Element{ public:

  CoLoR_Encapsulation();
  ~CoLoR_Encapsulation();

  const char *class_name() const	{ return "CoLoR_Encapsulation"; }
  const char *port_count() const	{ return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};
//*************************反向处理*************************
class CoLoR_Decapsulation : public Element{ public:

  CoLoR_Decapsulation();
  ~CoLoR_Decapsulation();

  const char *class_name() const  { return "CoLoR_Decapsulation"; }
  const char *port_count() const  { return "1/-"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};

//*********************更新模块部分****************************
//*********************对控制模块的定义*******************
class Updatepid_TAG_Creat : public Element
{
public:

  Updatepid_TAG_Creat();
  ~Updatepid_TAG_Creat();

  const char *class_name() const  { return "Updatepid_TAG_Creat"; }
  const char *port_count() const  { return "-/-"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};
//*********************对发送模块的定义*******************
class Updatepid_TAG_Send : public Element
{
public:

  Updatepid_TAG_Send();
  ~Updatepid_TAG_Send();

  const char *class_name() const  { return "Updatepid_TAG_Send"; }
  const char *port_count() const  { return "1/2"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};
//***********************对处理模块的定义******************************
class Updatepid_TAG_Process : public Element
{
public:

  Updatepid_TAG_Process();
  ~Updatepid_TAG_Process();

  const char *class_name() const  { return "Updatepid_TAG_Process"; }
  const char *port_count() const  { return "-/-"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

//*************************隧道维护模块************************
//*************************定时发送保活包*************************
class TAG_MAINT_SEND : public Element{ public:

  TAG_MAINT_SEND();
  ~TAG_MAINT_SEND();

  const char *class_name() const  { return "TAG_MAINT_SEND"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};
//*************************接收保活应答包*************************
class TAG_MAINT_RECV : public Element{ public:

  TAG_MAINT_RECV();
  ~TAG_MAINT_RECV();

  const char *class_name() const  { return "TAG_MAINT_RECV"; }
  const char *port_count() const  { return "1/0"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};
//*************************故障上报RM*************************
class TROUBLE_REPORT : public Element{ public:

  TROUBLE_REPORT();
  ~TROUBLE_REPORT();

  const char *class_name() const  { return "TROUBLE_REPORT"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};

//*************************来自RM的故障确认*************************
class TROUBLE_CONFIRM : public Element{ public:

  TROUBLE_CONFIRM();
  ~TROUBLE_CONFIRM();

  const char *class_name() const  { return "TROUBLE_CONFIRM"; }
  const char *port_count() const  { return "1/-"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet); 

};

CLICK_ENDDECLS
#endif
