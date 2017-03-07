#ifndef CLICK_CR_HH
#define CLICK_CR_HH
#include <click/element.hh>
CLICK_DECLS

/*
=c
Null

=s basictransfer
null element: passes packets unchanged

=d
Just passes packets along without doing anything else.

=a
PushNull, PullNull
*/

class CR : public Element{ public:

  CR();
  ~CR();

  const char *class_name() const	{ return "CR"; }
  const char *port_count() const	{ return "1/1"; }

  void push(int port, Packet *packet);

  Packet *simple_action(Packet *packet);

};

class CR1 : public Element{ public:

  CR1();
  ~CR1();

  const char *class_name() const  { return "CR1"; }
  const char *port_count() const  { return "1/1"; }

  void push(int port, Packet *packet1);

  Packet *simple_action(Packet *packet1);

};

/*
=c
PushNull

=s basictransfer
push-only null element

=d
Responds to each pushed packet by pushing it unchanged out its first output.

=a
Null, PullNull
*/

class PushCR : public Element { public:

  PushCR();
  ~PushCR();

  const char *class_name() const	{ return "PushCR"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PUSH; }

  void push(int, Packet *);

};

/*
=c
PullNull

=s basictransfer
pull-only null element

=d
Responds to each pull request by pulling a packet from its input and returning
that packet unchanged.

=a
Null, PushNull */

class PullCR : public Element { public:

  PullCR();
  ~PullCR();

  const char *class_name() const	{ return "PullCR"; }
  const char *port_count() const	{ return PORTS_1_1; }
  const char *processing() const	{ return PULL; }

  Packet *pull(int);

};

CLICK_ENDDECLS
#endif
