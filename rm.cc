/*
1����get��������һ��4�ֽڵ�reserved�ֶΣ����ڴ洢������
*/
#include <click/config.h>
#include <click/hashmap.hh>
#include <elements/ip/lineariplookup.hh>//����	IP����ģ��
#include <elements/ip/radixiplookup.hh>//����IP����ģ��
#include<unistd.h>//sleep()
#include<iostream>
#include "rm.hh"
using namespace std;
CLICK_DECLS

RadixIPLookup hu_table;  //����LinearIPLookup���һ������rt
IPRoute r; //��Ϊadd�����Ĳ���
IPRoute r1; //��Ϊadd�����Ĳ���
ErrorHandler *errh;
int f;
int f1;
//ֱ���ڹ��캯�������ע�����Ϣ
Register::Register(){
             //������ʼ��·�ɱ�
	memset(&r,0,sizeof(IPRoute));
	r.addr=0x0045a8c0; //ip=192.168.69.0
	r.mask=0x00ffffff;    //����=255.255.255.0
	r.gw=0x0;   //gw=192.168.69.1
	r.port=1;	      //����=1
	ErrorHandler *errh;
	//****���һ��·��*********
	f=hu_table.add_route(r,true, &r,errh);//���192.168.69.0���ε�·��,��ӳɹ��򷵻�0
	memset(&r1,0,sizeof(IPRoute));
	r1.addr=0x0016a8c0; //ip=192.168.22.0
	r1.mask=0x00ffffff;    //����=255.255.255.0
	r1.gw=0x0;   //gw=192.168.69.1
	r1.port=2;	      //����=2
	//****���һ��·��*********
	f1=hu_table.add_route(r1,true, &r1,errh);//���192.168.22.0����·��

}
Register::~Register(){}
Packet *Register::simple_action(Packet *p)
{
  return p;
}

SendTBG::SendTBG(){}//���캯����������������û�з���ֵ��
SendTBG::~SendTBG(){}
Packet *SendTBG::simple_action(Packet *p)
{
  return p;
}

AddPID::AddPID(){}//���캯����������������û�з���ֵ��
AddPID::~AddPID(){}
Packet *AddPID::simple_action(Packet *p)
{
  return p;
}

AddPID1::AddPID1(){}//���캯����������������û�з���ֵ��
AddPID1::~AddPID1(){}
Packet *AddPID1::simple_action(Packet *p)
{
  return p;
}

FAULT_HANDING::FAULT_HANDING(){}//���캯����������������û�з���ֵ��
FAULT_HANDING::~FAULT_HANDING(){}
Packet *FAULT_HANDING::simple_action(Packet *p)
{
  return p;
}

#define bufsize     1024 
//�洢get�����SID�ֽ���Ϣ��SID��L�����ܳ���Ϊ20��
typedef struct TEST_SID   TEST_SID;
struct TEST_SID
{
	char tunnel_sid[15];//���һλ�����桮\0
	IPAddress dst_ip;
};

//new GET packet format
typedef struct CoLoR_NEW_GET          CoLoR_NEW_GET;
//***************************�µİ���ʽ***************************
struct CoLoR_NEW_GET // 14+108+4=126Byte
{
	//Ethernet_header=14�ֽ�
	uint8_t ether_dhost[6]; 
	uint8_t ether_shost[6]; 
	uint16_t ether_type; 

	//Color_Get_header = 108Byte
	uint8_t version_type;////�汾4λ������4λ
	uint8_t ttl;//����ʱ��
	uint16_t total_len;//�ܳ���

	uint16_t port_no1;//�˿ں�1
	uint16_t port_no2;//�˿ں�2

	uint16_t minpid;//��Сpid change time
	uint8_t pids_o;//pid������λ������offset��־��λ
	uint8_t res;//�������ֽ�

	uint8_t offset[4];//ƫ����
	uint8_t length[4];//ƫ�Ƴ���

	uint16_t content_len;//���ݳ���
	uint16_t mtu;//����䵥Ԫ

	uint16_t publickey_len;//��Կ����
	uint16_t checksum;//�����

	//�����ֶ�
	uint8_t reserved[4];//4�ֽڵı����ֶΣ�reserved[0]����RM��ʶͬ�������reserved[1���ڴ洢������
	
	uint8_t n_sid[16];//SID��NID����
	uint8_t l_sid[20];//SID��L����
	uint8_t nid[16];//NID

	//*************NID����Ϊ�̶��ײ�80+4�ֽ�+����28************************
	
	uint8_t content_Ch[4];//content_Characterestics
	uint8_t publickey[4];
	uint8_t PID[5*4];//PID�ֶ�Ԥ��5���ռ�	
};
struct CoLoR_NEW_DATA//��������DATA����һ��14+80+20+1000=1114�ֽ�
{
	//Ethernet_header14�ֽڵ�mac��ͷ��
	uint8_t ether_dhost[6]; 
	uint8_t ether_shost[6]; 
	uint16_t ether_type; 

	uint8_t version_type;///�����ֶΣ�10100000��ʾget����10100001��ʾData��
	uint8_t ttl;//����ʱ��
	uint16_t total_len;//�����ܳ���

	uint16_t port_no1;//�˿�1
	uint16_t port_no2;//�˿�2

	uint16_t minpid;//��СPID����ʱ��
	uint8_t pids_o;//pid����ռ7λ��offset�ֶ�ռ1λ
	uint8_t res;//�����ֶ�

	uint8_t offset[4];//ƫ���ֶ�
	uint8_t length[4];//�����ֶ�

	uint16_t next_header;//��һ��ͷ������
	uint16_t checksum;//�����

	uint8_t reserved[4];//4�ֽڵı����ֶΣ�reserved[0]����RM��ʶͬ�������reserved[1���ڴ洢������
	uint8_t n_sid[16];//SID��NID����
	uint8_t l_sid[20];//SID��L����
	uint8_t nid[16];//NID

	//*************NID����λ�̶��ײ�80�ֽ�+���Ӳ���40�ֽ�************************
	
	uint8_t PID[5*4];//20�ֽڵ�PID�����Դ�5����ÿ��PIDռ4�ֽڣ�32λ
	uint8_t data[250*4];//1000���ֽڵ����ݲ��ֳ���
};

void Register::push(int port, Packet *packet)  //����TBG��ע����Ϣ�������·�ɱ���
{	
	/*//����ip����ģ���ʹ��
	IPRoute r; //��Ϊadd�����Ĳ���
	memset(&r,0,sizeof(IPRoute));
	r.addr=0x0045a8c0; //ip=192.168.69.0
	r.mask=0x00ffffff;    //����=255.255.255.0
	r.gw=0x0045a8c0;   //gw=192.168.69.1
	r.port=1;	      //����=1
	ErrorHandler *errh;
	//****���һ��·��*********
	int f=hu_table.add_route(r,true, &r,errh);
	if (f==0)
	{
		printf("Route Add Success!\n");
	}
	printf("**************Add a Route**********************\n");
	printf("Dst_IP		Mask		Gateway		OUT\n");
	cout<<r.addr.unparse().c_str()<<"\t";//���ʮ���ƴ�ӡ
	cout<<r.mask.unparse().c_str()<<"\t";//���ʮ���ƴ�ӡ
	cout<<r.gw.unparse().c_str()<<"\t";//���ʮ���ƴ�ӡ
	cout<<port<<endl;
	*/
}

//****����TAG�����󲢻���TBG��Ϣ(��data��������TBG������Ϣ������Ϊ��һ�θ��°�)**************************
int Register=1;
uint8_t TUNNEL_SID_RM[15]={'t','u','n','n','e','l','_','s','e','r','v','i','c','e','\0'}; //���ڲ��ԵĲ���sid
HashMap<int,String >TBG_tbl;	//��ϣ��Ĳ������Ϳ����Լ�����Ŷ
void SendTBG::push(int port, Packet *packet)
{	
	//����ע����Ϣ
	if (Register == 1)
	{
		cout<<"***********These are TBG`s registration information!***********"<<endl;
		if (f==0)
		{
			printf("f(192.168.69.0) Route Add Success!\n");
		}
		if (f1==0)
		{
			printf("f1(192.168.22.0)  Route Add Success!\n\n");
		}
		//�������ڵ�TBG NID��hash��(Ӧ����ע��ģ�����)
		int num = 1;
		int num1 = 2;
		char TBGNID[16];
		memcpy(TBGNID, "TBG1",4);//t/
		//HashMap<int,String >TBG_tbl;	//��ϣ��Ĳ������Ϳ����Լ�����Ŷ
		TBG_tbl.insert(num, "TBG1");
		TBG_tbl.insert(num1, "TBG2");
		Register=0;
	}
	//�����յ���get�����в�ѯ
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_GET* pPKG;
	pPKG=(CoLoR_NEW_GET*)RecvBuf;
	TEST_SID get;
	memcpy(get.tunnel_sid, pPKG->l_sid,14);
	get.tunnel_sid[14]='\0';//��������sid�����־
	memcpy(&get.dst_ip,(unsigned char *)pPKG->l_sid+14,4);//��������Ŀ��ip 
	printf("pPKG->l_sid=%s\n",pPKG->l_sid);

	cout<<"11111"<<endl;
			printf("dst_ip:%d.%d.%d.%d\n",pPKG->l_sid[14],pPKG->l_sid[15],pPKG->l_sid[16],pPKG->l_sid[17]);
			char test_sid[15];
			memset(test_sid,0,sizeof(test_sid));
			for (int i = 0; i < 14; ++i)
			test_sid[i]=pPKG->l_sid[i];
			test_sid[14]='\0';
			printf("%s\n",test_sid);
	cout<<"1111"<<endl;
	if (!strcmp((char *)TUNNEL_SID_RM,get.tunnel_sid))//ƥ��SID,˵���ǲ�ѯTBG��Ϣ��get��
	{
		cout<<"\nGet_Recv : SID match successfully! This is a TBG_info requst packet from TAG"<<endl;
		//************����·�ɱ�: ���TBG���**********************
		IPAddress gw;//������Ų��ҵõ�������;
		printf("********Looking up IP Table....*******************\n");
		cout<<"GET_DST_IP="<<get.dst_ip.unparse().c_str()<<endl;    //���ʮ���ƴ�ӡ,����.c_str()�ǰ�stringת��Ϊ�ַ����飬��/0����
		int port=hu_table.lookup_route(get.dst_ip, gw);
		//�ǰ׺ƥ��ʧ�ܷ���-1
		if (port==-1)
		{
			cout<<"cann't find ! fail!"<<endl; 
		}
		else
		{
			printf("************Found Success !**********************\n");
			cout<<"port="<<port<<endl;
			cout<<"gw="<<gw.unparse().c_str()<<endl<<endl;

			//************���ҹ�ϣ�� ���TBG��NID**********************
			String val;
			val=TBG_tbl.find(port);
			char Find_NID[17];//�洢�ӹ�ϣ�����ҵ���NID
			memcpy(Find_NID,val.c_str(),16);
			cout<<"obtain TBG's NID ="<<Find_NID<<endl;
			
			//******************�޸�data������***********************
			int pkglength;
			pkglength = packet->length();
			uint32_t addlength;
			addlength=134-pkglength;//Data����14+80+40=134�ֽ�
			packet->put(addlength);
			CoLoR_NEW_DATA PKG;
			memset(&PKG,0,sizeof(CoLoR_NEW_DATA));
			
			//*******************��װdata��: SID�ֶΡ�NID�ֶΡ�reserverd[0]�ֶ�*************************
			//���mac���ַ
			PKG.ether_dhost[0] = 0xAA;//Ŀ��
			PKG.ether_dhost[1] = 0xAA;
			PKG.ether_dhost[2] = 0xAA;
			PKG.ether_dhost[3] = 0xAA;
			PKG.ether_dhost[4] = 0xAA;
			PKG.ether_dhost[5] = 0xAA;
			PKG.ether_shost[0] = 0xCC;//Դ
			PKG.ether_shost[1] = 0xCC;
			PKG.ether_shost[2] = 0xCC;
			PKG.ether_shost[3] = 0xCC;
			PKG.ether_shost[4] = 0xCC;
			PKG.ether_shost[5] = 0xCC;
			PKG.ether_type = 0x0008;	

			PKG.version_type=161;//10100001
			PKG.ttl=255;
			PKG.total_len=134;

			PKG.port_no1=1;
			PKG.port_no2=2;

			PKG.minpid=6; //��С��PID����ʱ����RM����
			PKG.pids_o=12;//00001100,PID����ռ5λ������λo=1,c=0,n=0;
			PKG.res=0; //���ı��

			int iii;
			uint8_t offset[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.offset[iii]=offset[iii];

			uint8_t length[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.length[iii]=length[iii];

			PKG.next_header=0;
			PKG.checksum=123;

			memset(PKG.reserved,0,sizeof(PKG.reserved));
			PKG.reserved[0]=0;//1Ϊͬ��0Ϊ����
			cout<<" TBG position = "<<PKG.reserved[0]<<endl;
			//*******************��ƥ�䵽��TBGNID��װ��n_sid***************************
			for(iii=0;iii<16;iii++)
			PKG.n_sid[iii]=Find_NID[iii];

			memcpy(PKG.l_sid,pPKG->l_sid,14);
			//��Ŀ�������װ��l_SID���ظ�TAG
			if (port==1)
			{
				memcpy(PKG.l_sid+14,r.addr.data(),4);
				cout<<"sizeof(r.addr.data() ="<<sizeof(r.addr.data()) <<endl;
				cout<<"r.addr.unparse().c_str() = "<<r.addr.unparse().c_str()<<endl;
			}
			else if (port == 2)
			{
				memcpy(PKG.l_sid+14,r1.addr.data(),4);
				cout<<"sizeof(r1.addr.data() ="<<sizeof(r.addr.data()) <<endl;
				cout<<"r1.addr.unparse().c_str() = "<<r1.addr.unparse().c_str()<<endl;
			}
			printf("dst_ip:%d.%d.%d.%d\n",pPKG->l_sid[14],pPKG->l_sid[15],pPKG->l_sid[16],pPKG->l_sid[17] );
			printf("dst_net:%d.%d.%d.%d\n",PKG.l_sid[14],PKG.l_sid[15],PKG.l_sid[16],PKG.l_sid[17] );
		
			//���յ��İ����ú󷢳�
			memcpy(PKG.nid,pPKG->nid+8,8); //Ŀ��NIDΪ�յ�����ԴNID=TAG
			memcpy(PKG.nid+8,pPKG->nid,8); //ԴNIDΪ�յ�����Ŀ��NID=RM1
			
			//*******************���PID����Ϊ��һ�θ��°���**********************
			uint8_t PID[4]={'P','I','D','1'};
			for(iii=0;iii<4;iii++)
			PKG.PID[iii]=PID[iii];
			////*******************���data�ֶ�**********************
			memcpy(PKG.data,PKG.PID,sizeof(PKG.PID));
			//cout<<PKG.PID<<endl;
			//cout<<PKG.data<<endl;

			//****************************����data��**********************************
			memcpy((unsigned char *)packet->data(),&PKG,sizeof(CoLoR_NEW_DATA));
			cout<<"TBG send"<<endl; 
			output(0).push(packet);
		}
	}
}
//****************************���ƹ���ȷ��***************************************
uint8_t  TROUBULE_L_SID[15]={'t','r','o','u','b','l','e','_','r','e','p','o','r','t','\0'}; //���ϱ�־��
void FAULT_HANDING::push(int port, Packet *packet)
{
	//�����յ���get�����в�ѯ
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_GET* pPKG;
	pPKG=(CoLoR_NEW_GET*)RecvBuf;
	char trouble_test_sid[15];//ȡ�����ϰ��е�sid
	memcpy(trouble_test_sid, pPKG->l_sid,14);
	trouble_test_sid[14]='\0';
	//ƥ��SID,˵��ΪTAGά��ģ��Ĺ����ϱ���
	if (!strcmp((char *)TROUBULE_L_SID,trouble_test_sid))
	{
		cout<<"This is a TROUTBLE_REPORT packet!"<<endl;
		//ȡ�����Ͻڵ���Ϣ
		char trouble_node_nid[16];
		memcpy(trouble_node_nid,pPKG->n_sid,sizeof(pPKG->n_sid));
		//****************ȷ�Ϲ���****************************
		char TROUBLE_YES[20]={'t','r','o','u','b','l','e','_','c','o','n','f','i','r','m','_','y','e','s','\0'}; //���ϱ�־��
		memcpy(pPKG->l_sid,TROUBLE_YES,20);
		//cout<<pPKG->l_sid<<endl;
		cout<<trouble_node_nid<<" has acctualy already broken down ! --- confirmation packet sent "<<endl;
		/*
		//**************����ȷ���޹���*************************
		char TROUBLE_NO[19]={'t','r','o','u','b','l','e','_','c','o','n','f','i','r','m','_','n','o','\0'}; //�޹��ϱ�־��
		memcpy(pPKG->l_sid,TROUBLE_NO,19);
		//cout<<pPKG->l_sid<<endl;
		cout<<trouble_node_nid<<" is working  properly ! --- confirmation packet sent "<<endl;
		*/
		//**************����ȷ�ϰ�-GET��************************
		//�޸�Դ��Ŀ��NID
		memcpy(pPKG->nid,"TAG1",4);         //Ŀ��NID
		memcpy(pPKG->nid+8,"RM1",3);     //ԴNID
		memcpy((char *)packet->data(),pPKG,packet->length());
		output(0).push(packet);	
	}
}

//Ϊ��eth0�ڽ�����get�����PID
int jj=0;
char UPDATE_SID_RM[10]={'p','i','d','u','p','d','a','t','e','\0'};     
void AddPID::push(int port, Packet *packet)
{
	//*******��ȡ����������+�޸�PID�����ֶ�+�޸�minPID����ʱ��+���PID**************************
	unsigned char pkg[bufsize];
	memset(pkg,0,sizeof(pkg));
	CoLoR_NEW_GET* pPKG;
	pPKG = (CoLoR_NEW_GET* )pkg;
	int RecvLength=0;
	RecvLength=packet->length();
	memcpy(pkg,packet->data(),RecvLength);
	//ȡ������sid
	char test_sid[10];
	memcpy(test_sid, pPKG->l_sid,10);//get�����PID���±�־
	//**************************Ϊ������°�����**************************
	if (!strcmp(UPDATE_SID_RM,test_sid)/*Ϊpid���°�*/)
	{
		cout<<"RM add PIDs for PID_UPDATE_PACKET"<<endl;
		//���յ���get�����������һ��PID��Ϣ�����޸�PID�����ֶΣ�
		pPKG->minpid=6;//�޸�PID����С����ʱ��
		pPKG->pids_o=3;//00000011,��ʾPID��������1������offset��־λ
		if (jj=0)//���һ����PID
		{
			int iii;
			uint8_t PID[4]={'P','I','D','1'};
			for(iii=0;iii<4;iii++)
			pPKG->PID[iii]=PID[iii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj=0;
		}
		else //�ı�jj��ֵʹÿ����Ӳ�ͬ��PID
		{
			int ii;
			uint8_t PID2[4]={'P','I','D','2'};
			for(ii=0;ii<4;ii++)
			pPKG->PID[ii]=PID2[ii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj=0;
		}	
	}
	//****************************�������ݰ�get��**********************************
	memcpy((unsigned char *)packet->data(),pPKG,126);
	output(0).push(packet);
	
}

//Ϊ��eth1�ڽ����İ����PID
int jj1=0;
void AddPID1::push(int port, Packet *packet)
{
	//******************��ȡ����������+�޸�PID�����ֶ�+���PID**************************
	unsigned char pkg[bufsize];
	memset(pkg,0,sizeof(pkg));
	CoLoR_NEW_GET* pPKG;
	pPKG = (CoLoR_NEW_GET* )pkg;
	int RecvLength=0;
	RecvLength=packet->length();
	memcpy(pkg,packet->data(),RecvLength);
	//ȡ������sid
	char test_sid[10];
	memcpy(test_sid, pPKG->l_sid,10);//get�����PID���±�־
	//**************************Ϊ������°�����**************************
	if (!strcmp(UPDATE_SID_RM,test_sid)/*Ϊpid���°�*/)
	{
		cout<<"RM add PIDs for PID_UPDATE_PACKET"<<endl;
		//���յ���get�����������PID��Ϣ�����޸�PID�����ֶΣ�
		pPKG->minpid=6;//�޸�PID����С����ʱ��
		pPKG->pids_o=3;//00000011,��ʾPID��������1������offset��־λ
		if ( jj1==0)
		{
			int iii;
			uint8_t PID[4]={'P','I','D','1'};
			for(iii=0;iii<4;iii++)
			pPKG->PID[iii]=PID[iii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj1=0;
		}
		else //��Ӳ�ͬ��PID 
		{
			int ii;
			uint8_t PID2[4]={'P','I','D','2'};
			for(ii=0;ii<4;ii++)
			pPKG->PID[ii]=PID2[ii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj=0;
		}
	}	
	//****************************�������ݰ�get��**********************************
	memcpy((unsigned char *)packet->data(),pPKG,126);
	output(0).push(packet);
	
}

CLICK_ENDDECLS
EXPORT_ELEMENT(AddPID)
ELEMENT_MT_SAFE(AddPID)

EXPORT_ELEMENT(AddPID1)
ELEMENT_MT_SAFE(AddPID1)

EXPORT_ELEMENT(SendTBG)
ELEMENT_MT_SAFE(SendTBG)

EXPORT_ELEMENT(Register)
ELEMENT_MT_SAFE(Register)

EXPORT_ELEMENT(FAULT_HANDING)
ELEMENT_MT_SAFE(FAULT_HANDING)