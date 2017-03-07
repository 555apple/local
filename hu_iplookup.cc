#include <click/config.h>
#include <click/hashmap.hh>
#include <elements/ip/lineariplookup.hh>//线性	IP查找模块
#include <elements/ip/radixiplookup.hh>//树型IP查找模块
#include<unistd.h>//sleep()
#include<iostream>
#include "hu_iplookup.hh"
using namespace std;
CLICK_DECLS

RadixIPLookup hu_table;  //定义LinearIPLookup类的一个对象rt
IPRoute r; //作为add函数的参数
IPRoute r1; //作为add函数的参数
ErrorHandler *errh;
int f;
int f1;
//直接在构造函数中添加注册次信息
ADD_ROUTE::ADD_ROUTE(){
             //建立初始的路由表
	memset(&r,0,sizeof(IPRoute));
	r.addr=0x0045a8c0; //ip=192.168.69.0
	r.mask=0x00ffffff;    //掩码=255.255.255.0
	r.gw=0x0;   //gw=192.168.69.1
	r.port=1;	      //出口=1
	ErrorHandler *errh;
	//****添加一条路由*********
	f=hu_table.ADD_ROUTEe(r,true, &r,errh);//添加192.168.69.0网段的路由,添加成功则返回0
	memset(&r1,0,sizeof(IPRoute));
	r1.addr=0x0016a8c0; //ip=192.168.22.0
	r1.mask=0x00ffffff;    //掩码=255.255.255.0
	r1.gw=0x0;   //gw=192.168.69.1
	r1.port=2;	      //出口=2
	//****添加一条路由*********
	f1=hu_table.ADD_ROUTEe(r1,true, &r1,errh);//添加192.168.22.0网段路由
}
ADD_ROUTE::~ADD_ROUTE(){}
Packet *ADD_ROUTE::simple_action(Packet *p)
{
  return p;
}
