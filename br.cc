/*
 * CR.{cc,hh} -- do-nothing element
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include <iostream>
#include "br.hh"
 using namespace std;
CLICK_DECLS

 CR::CR()
{
}

CR::~CR()
{
}

Packet *
CR::simple_action(Packet *p)
{
  return p;
}

CR1::CR1()
{
}

CR1::~CR1()
{
}

Packet *
CR1::simple_action(Packet *p)
{
  return p;
}
//××××××××××××××××××××××××××××各种定义×××××××××××××××××××××//
#define bufsize     1024 * 5
unsigned char RecvBuf_cr[bufsize] = {0};
unsigned char RecvBuf_cr1[bufsize] = {0};
//CR 类用于进行正向处理：进行mac地址的判断，并将符合要求的数据包从指定网口发出
void
CR::push(int port, Packet *packet)
{
	cout<<"recv from h2 "<<endl;
	int RecvLength;
	RecvLength=packet->length();
	memcpy(RecvBuf_cr,packet->data(),packet->length());
	output(0).push(packet);
	/*//拒收源mac为border-eth0的数据包
	if(
	RecvBuf_cr[6] ==0xba &&
	RecvBuf_cr[7] ==0xe0 &&
	RecvBuf_cr[8] ==0x4e &&
	RecvBuf_cr[9] ==0x10 &&
	RecvBuf_cr[10]==0xc8 &&
	RecvBuf_cr[11]==0xfa)
	{ 
		printf("drop\n");  //暂时不确定自己发出的包会不会被自己接收
		return;
	}
	else
	{
		output(0).push(packet);
	}*/
}

//CR1 类用于进行反向处理：进行mac地址的判断，并将符合要求的数据包从指定网口发出
void
CR1::push(int port, Packet *packet1)
{
	cout<<"recv from h4 "<<endl;
	int RecvLength1;
	RecvLength1=packet1->length();
	memcpy(RecvBuf_cr1,packet1->data(),packet1->length());
	output(0).push(packet1);
	/*//拒收源mac为access-eth1的数据包
	if(
	RecvBuf_cr1[6] ==0xc6 &&
	RecvBuf_cr1[7] ==0xbd &&
	RecvBuf_cr1[8] ==0xdd &&
	RecvBuf_cr1[9] ==0x08 &&
	RecvBuf_cr1[10]==0x27 &&
	RecvBuf_cr1[11]==0x05)
	{ 
		printf("drop\n");  //暂时不确定自己发出的包会不会被自己接收
		return;
	}
	else
	{
		output(0).push(packet1);
	}*/
}

PushCR::PushCR()
{
}

PushCR::~PushCR()
{
}

void
PushCR::push(int, Packet *p)
{
  //output(0).push(p);
	output(2).push(p);
}

PullCR::PullCR()
{
}

PullCR::~PullCR()
{
}

Packet *
PullCR::pull(int)
{
  return input(0).pull();
}


CLICK_ENDDECLS
EXPORT_ELEMENT(CR)
EXPORT_ELEMENT(CR1)
EXPORT_ELEMENT(PushCR)
EXPORT_ELEMENT(PullCR)
ELEMENT_MT_SAFE(CR)
ELEMENT_MT_SAFE(CR1)
ELEMENT_MT_SAFE(PushCR)
ELEMENT_MT_SAFE(PullCR)
