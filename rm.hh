#ifndef CLICK_RM_HH
#define CLICK_RM_HH
#include <click/element.hh>
#include <elements/ip/radixiplookup.hh>
CLICK_DECLS

class FAULT_HANDING : public Element
{
public:

  FAULT_HANDING();
  ~FAULT_HANDING();

  const char *class_name() const  { return "FAULT_HANDING"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

class Register : public Element
{
public:

  Register();
  ~Register();

  const char *class_name() const  { return "Register"; }
  const char *port_count() const  { return "-/-"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

class SendTBG : public Element
{
public:

  SendTBG();
  ~SendTBG();

  const char *class_name() const  { return "SendTBG"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

class AddPID : public Element
{
public:

  AddPID();
  ~AddPID();

  const char *class_name() const	{ return "AddPID"; }
  const char *port_count() const	{ return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

class AddPID1 : public Element
{
public:

  AddPID1();
  ~AddPID1();

  const char *class_name() const	{ return "AddPID1"; }
  const char *port_count() const	{ return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

CLICK_ENDDECLS
#endif
