/*
1、将get包增加了一个4字节的reserved字段，用于存储隧道编号
*/
#include <click/config.h>
#include <click/hashmap.hh>
#include <elements/ip/lineariplookup.hh>//线性	IP查找模块
#include <elements/ip/radixiplookup.hh>//树型IP查找模块
#include<unistd.h>//sleep()
#include<iostream>
#include "rm.hh"
using namespace std;
CLICK_DECLS

RadixIPLookup hu_table;  //定义LinearIPLookup类的一个对象rt
IPRoute r; //作为add函数的参数
IPRoute r1; //作为add函数的参数
ErrorHandler *errh;
int f;
int f1;
//直接在构造函数中添加注册次信息
Register::Register(){
             //建立初始的路由表
	memset(&r,0,sizeof(IPRoute));
	r.addr=0x0045a8c0; //ip=192.168.69.0
	r.mask=0x00ffffff;    //掩码=255.255.255.0
	r.gw=0x0;   //gw=192.168.69.1
	r.port=1;	      //出口=1
	ErrorHandler *errh;
	//****添加一条路由*********
	f=hu_table.add_route(r,true, &r,errh);//添加192.168.69.0网段的路由,添加成功则返回0
	memset(&r1,0,sizeof(IPRoute));
	r1.addr=0x0016a8c0; //ip=192.168.22.0
	r1.mask=0x00ffffff;    //掩码=255.255.255.0
	r1.gw=0x0;   //gw=192.168.69.1
	r1.port=2;	      //出口=2
	//****添加一条路由*********
	f1=hu_table.add_route(r1,true, &r1,errh);//添加192.168.22.0网段路由

}
Register::~Register(){}
Packet *Register::simple_action(Packet *p)
{
  return p;
}

SendTBG::SendTBG(){}//构造函数和析构函数都是没有返回值的
SendTBG::~SendTBG(){}
Packet *SendTBG::simple_action(Packet *p)
{
  return p;
}

AddPID::AddPID(){}//构造函数和析构函数都是没有返回值的
AddPID::~AddPID(){}
Packet *AddPID::simple_action(Packet *p)
{
  return p;
}

AddPID1::AddPID1(){}//构造函数和析构函数都是没有返回值的
AddPID1::~AddPID1(){}
Packet *AddPID1::simple_action(Packet *p)
{
  return p;
}

FAULT_HANDING::FAULT_HANDING(){}//构造函数和析构函数都是没有返回值的
FAULT_HANDING::~FAULT_HANDING(){}
Packet *FAULT_HANDING::simple_action(Packet *p)
{
  return p;
}

#define bufsize     1024 
//存储get包里的SID分解信息（SID的L部分总长度为20）
typedef struct TEST_SID   TEST_SID;
struct TEST_SID
{
	char tunnel_sid[15];//最后一位用来存‘\0
	IPAddress dst_ip;
};

//new GET packet format
typedef struct CoLoR_NEW_GET          CoLoR_NEW_GET;
//***************************新的包格式***************************
struct CoLoR_NEW_GET // 14+108+4=126Byte
{
	//Ethernet_header=14字节
	uint8_t ether_dhost[6]; 
	uint8_t ether_shost[6]; 
	uint16_t ether_type; 

	//Color_Get_header = 108Byte
	uint8_t version_type;////版本4位，类型4位
	uint8_t ttl;//生存时间
	uint16_t total_len;//总长度

	uint16_t port_no1;//端口号1
	uint16_t port_no2;//端口号2

	uint16_t minpid;//最小pid change time
	uint8_t pids_o;//pid数量７位，有无offset标志１位
	uint8_t res;//保留１字节

	uint8_t offset[4];//偏移量
	uint8_t length[4];//偏移长度

	uint16_t content_len;//内容长度
	uint16_t mtu;//最大传输单元

	uint16_t publickey_len;//公钥长度
	uint16_t checksum;//检验和

	//新增字段
	uint8_t reserved[4];//4字节的保留字段：reserved[0]用于RM标识同域或异域，reserved[1用于存储隧道编号
	
	uint8_t n_sid[16];//SID的NID部分
	uint8_t l_sid[20];//SID的L部分
	uint8_t nid[16];//NID

	//*************NID以上为固定首部80+4字节+附加28************************
	
	uint8_t content_Ch[4];//content_Characterestics
	uint8_t publickey[4];
	uint8_t PID[5*4];//PID字段预留5条空间	
};
struct CoLoR_NEW_DATA//承载数据DATA包，一共14+80+20+1000=1114字节
{
	//Ethernet_header14字节的mac层头部
	uint8_t ether_dhost[6]; 
	uint8_t ether_shost[6]; 
	uint16_t ether_type; 

	uint8_t version_type;///类型字段，10100000表示get包，10100001表示Data包
	uint8_t ttl;//生存时间
	uint16_t total_len;//包的总长度

	uint16_t port_no1;//端口1
	uint16_t port_no2;//端口2

	uint16_t minpid;//最小PID更新时间
	uint8_t pids_o;//pid数量占7位，offset字段占1位
	uint8_t res;//保留字段

	uint8_t offset[4];//偏移字段
	uint8_t length[4];//长度字段

	uint16_t next_header;//下一个头部类型
	uint16_t checksum;//检验和

	uint8_t reserved[4];//4字节的保留字段：reserved[0]用于RM标识同域或异域，reserved[1用于存储隧道编号
	uint8_t n_sid[16];//SID的NID部分
	uint8_t l_sid[20];//SID的L部分
	uint8_t nid[16];//NID

	//*************NID以上位固定首部80字节+附加部分40字节************************
	
	uint8_t PID[5*4];//20字节的PID，可以存5条，每条PID占4字节，32位
	uint8_t data[250*4];//1000个字节的数据部分长度
};

void Register::push(int port, Packet *packet)  //接收TBG的注册信息并添加在路由表里
{	
	/*//测试ip查找模块的使用
	IPRoute r; //作为add函数的参数
	memset(&r,0,sizeof(IPRoute));
	r.addr=0x0045a8c0; //ip=192.168.69.0
	r.mask=0x00ffffff;    //掩码=255.255.255.0
	r.gw=0x0045a8c0;   //gw=192.168.69.1
	r.port=1;	      //出口=1
	ErrorHandler *errh;
	//****添加一条路由*********
	int f=hu_table.add_route(r,true, &r,errh);
	if (f==0)
	{
		printf("Route Add Success!\n");
	}
	printf("**************Add a Route**********************\n");
	printf("Dst_IP		Mask		Gateway		OUT\n");
	cout<<r.addr.unparse().c_str()<<"\t";//点分十进制打印
	cout<<r.mask.unparse().c_str()<<"\t";//点分十进制打印
	cout<<r.gw.unparse().c_str()<<"\t";//点分十进制打印
	cout<<port<<endl;
	*/
}

//****接收TAG的请求并回送TBG信息(用data包来承载TBG回送信息，并作为第一次更新包)**************************
int Register=1;
uint8_t TUNNEL_SID_RM[15]={'t','u','n','n','e','l','_','s','e','r','v','i','c','e','\0'}; //用于测试的部分sid
HashMap<int,String >TBG_tbl;	//哈希表的参数类型可以自己定义哦
void SendTBG::push(int port, Packet *packet)
{	
	//加载注册信息
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
		//建立出口到TBG NID的hash表(应该在注册模块完成)
		int num = 1;
		int num1 = 2;
		char TBGNID[16];
		memcpy(TBGNID, "TBG1",4);//t/
		//HashMap<int,String >TBG_tbl;	//哈希表的参数类型可以自己定义哦
		TBG_tbl.insert(num, "TBG1");
		TBG_tbl.insert(num1, "TBG2");
		Register=0;
	}
	//根据收到的get包进行查询
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_GET* pPKG;
	pPKG=(CoLoR_NEW_GET*)RecvBuf;
	TEST_SID get;
	memcpy(get.tunnel_sid, pPKG->l_sid,14);
	get.tunnel_sid[14]='\0';//请求包里的sid隧道标志
	memcpy(&get.dst_ip,(unsigned char *)pPKG->l_sid+14,4);//请求包里的目的ip 
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
	if (!strcmp((char *)TUNNEL_SID_RM,get.tunnel_sid))//匹配SID,说明是查询TBG信息的get包
	{
		cout<<"\nGet_Recv : SID match successfully! This is a TBG_info requst packet from TAG"<<endl;
		//************查找路由表: 获得TBG标号**********************
		IPAddress gw;//用来存放查找得到的网关;
		printf("********Looking up IP Table....*******************\n");
		cout<<"GET_DST_IP="<<get.dst_ip.unparse().c_str()<<endl;    //点分十进制打印,其中.c_str()是把string转换为字符数组，以/0结束
		int port=hu_table.lookup_route(get.dst_ip, gw);
		//最长前缀匹配失败返回-1
		if (port==-1)
		{
			cout<<"cann't find ! fail!"<<endl; 
		}
		else
		{
			printf("************Found Success !**********************\n");
			cout<<"port="<<port<<endl;
			cout<<"gw="<<gw.unparse().c_str()<<endl<<endl;

			//************查找哈希表： 获得TBG的NID**********************
			String val;
			val=TBG_tbl.find(port);
			char Find_NID[17];//存储从哈希表中找到的NID
			memcpy(Find_NID,val.c_str(),16);
			cout<<"obtain TBG's NID ="<<Find_NID<<endl;
			
			//******************修改data包长度***********************
			int pkglength;
			pkglength = packet->length();
			uint32_t addlength;
			addlength=134-pkglength;//Data长度14+80+40=134字节
			packet->put(addlength);
			CoLoR_NEW_DATA PKG;
			memset(&PKG,0,sizeof(CoLoR_NEW_DATA));
			
			//*******************封装data包: SID字段、NID字段、reserverd[0]字段*************************
			//填充mac层地址
			PKG.ether_dhost[0] = 0xAA;//目的
			PKG.ether_dhost[1] = 0xAA;
			PKG.ether_dhost[2] = 0xAA;
			PKG.ether_dhost[3] = 0xAA;
			PKG.ether_dhost[4] = 0xAA;
			PKG.ether_dhost[5] = 0xAA;
			PKG.ether_shost[0] = 0xCC;//源
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

			PKG.minpid=6; //最小的PID更新时间由RM决定
			PKG.pids_o=12;//00001100,PID数量占5位，其他位o=1,c=0,n=0;
			PKG.res=0; //包的编号

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
			PKG.reserved[0]=0;//1为同域，0为异域
			cout<<" TBG position = "<<PKG.reserved[0]<<endl;
			//*******************将匹配到的TBGNID封装进n_sid***************************
			for(iii=0;iii<16;iii++)
			PKG.n_sid[iii]=Find_NID[iii];

			memcpy(PKG.l_sid,pPKG->l_sid,14);
			//将目的网络封装进l_SID返回给TAG
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
		
			//按收到的包倒置后发出
			memcpy(PKG.nid,pPKG->nid+8,8); //目的NID为收到包的源NID=TAG
			memcpy(PKG.nid+8,pPKG->nid,8); //源NID为收到包的目的NID=RM1
			
			//*******************填充PID（作为第一次更新包）**********************
			uint8_t PID[4]={'P','I','D','1'};
			for(iii=0;iii<4;iii++)
			PKG.PID[iii]=PID[iii];
			////*******************填充data字段**********************
			memcpy(PKG.data,PKG.PID,sizeof(PKG.PID));
			//cout<<PKG.PID<<endl;
			//cout<<PKG.data<<endl;

			//****************************发送data包**********************************
			memcpy((unsigned char *)packet->data(),&PKG,sizeof(CoLoR_NEW_DATA));
			cout<<"TBG send"<<endl; 
			output(0).push(packet);
		}
	}
}
//****************************疑似故障确认***************************************
uint8_t  TROUBULE_L_SID[15]={'t','r','o','u','b','l','e','_','r','e','p','o','r','t','\0'}; //故障标志包
void FAULT_HANDING::push(int port, Packet *packet)
{
	//根据收到的get包进行查询
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_GET* pPKG;
	pPKG=(CoLoR_NEW_GET*)RecvBuf;
	char trouble_test_sid[15];//取出故障包中的sid
	memcpy(trouble_test_sid, pPKG->l_sid,14);
	trouble_test_sid[14]='\0';
	//匹配SID,说明为TAG维护模块的故障上报包
	if (!strcmp((char *)TROUBULE_L_SID,trouble_test_sid))
	{
		cout<<"This is a TROUTBLE_REPORT packet!"<<endl;
		//取出故障节点信息
		char trouble_node_nid[16];
		memcpy(trouble_node_nid,pPKG->n_sid,sizeof(pPKG->n_sid));
		//****************确认故障****************************
		char TROUBLE_YES[20]={'t','r','o','u','b','l','e','_','c','o','n','f','i','r','m','_','y','e','s','\0'}; //故障标志包
		memcpy(pPKG->l_sid,TROUBLE_YES,20);
		//cout<<pPKG->l_sid<<endl;
		cout<<trouble_node_nid<<" has acctualy already broken down ! --- confirmation packet sent "<<endl;
		/*
		//**************或者确认无故障*************************
		char TROUBLE_NO[19]={'t','r','o','u','b','l','e','_','c','o','n','f','i','r','m','_','n','o','\0'}; //无故障标志包
		memcpy(pPKG->l_sid,TROUBLE_NO,19);
		//cout<<pPKG->l_sid<<endl;
		cout<<trouble_node_nid<<" is working  properly ! --- confirmation packet sent "<<endl;
		*/
		//**************发送确认包-GET包************************
		//修改源和目的NID
		memcpy(pPKG->nid,"TAG1",4);         //目的NID
		memcpy(pPKG->nid+8,"RM1",3);     //源NID
		memcpy((char *)packet->data(),pPKG,packet->length());
		output(0).push(packet);	
	}
}

//为从eth0口进来的get包添加PID
int jj=0;
char UPDATE_SID_RM[10]={'p','i','d','u','p','d','a','t','e','\0'};     
void AddPID::push(int port, Packet *packet)
{
	//*******提取缓冲区数据+修改PID数量字段+修改minPID更新时间+添加PID**************************
	unsigned char pkg[bufsize];
	memset(pkg,0,sizeof(pkg));
	CoLoR_NEW_GET* pPKG;
	pPKG = (CoLoR_NEW_GET* )pkg;
	int RecvLength=0;
	RecvLength=packet->length();
	memcpy(pkg,packet->data(),RecvLength);
	//取出包中sid
	char test_sid[10];
	memcpy(test_sid, pPKG->l_sid,10);//get包里的PID更新标志
	//**************************为隧道更新包更新**************************
	if (!strcmp(UPDATE_SID_RM,test_sid)/*为pid更新包*/)
	{
		cout<<"RM add PIDs for PID_UPDATE_PACKET"<<endl;
		//在收到的get包后面添加上一条PID信息，并修改PID数量字段；
		pPKG->minpid=6;//修改PID的最小更新时间
		pPKG->pids_o=3;//00000011,表示PID的数量有1条，有offset标志位
		if (jj=0)//添加一样的PID
		{
			int iii;
			uint8_t PID[4]={'P','I','D','1'};
			for(iii=0;iii<4;iii++)
			pPKG->PID[iii]=PID[iii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj=0;
		}
		else //改变jj的值使每次添加不同的PID
		{
			int ii;
			uint8_t PID2[4]={'P','I','D','2'};
			for(ii=0;ii<4;ii++)
			pPKG->PID[ii]=PID2[ii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj=0;
		}	
	}
	//****************************发送数据包get包**********************************
	memcpy((unsigned char *)packet->data(),pPKG,126);
	output(0).push(packet);
	
}

//为从eth1口进来的包添加PID
int jj1=0;
void AddPID1::push(int port, Packet *packet)
{
	//******************提取缓冲区数据+修改PID数量字段+添加PID**************************
	unsigned char pkg[bufsize];
	memset(pkg,0,sizeof(pkg));
	CoLoR_NEW_GET* pPKG;
	pPKG = (CoLoR_NEW_GET* )pkg;
	int RecvLength=0;
	RecvLength=packet->length();
	memcpy(pkg,packet->data(),RecvLength);
	//取出包中sid
	char test_sid[10];
	memcpy(test_sid, pPKG->l_sid,10);//get包里的PID更新标志
	//**************************为隧道更新包更新**************************
	if (!strcmp(UPDATE_SID_RM,test_sid)/*为pid更新包*/)
	{
		cout<<"RM add PIDs for PID_UPDATE_PACKET"<<endl;
		//在收到的get包后面添加上PID信息，并修改PID数量字段；
		pPKG->minpid=6;//修改PID的最小更新时间
		pPKG->pids_o=3;//00000011,表示PID的数量有1条，有offset标志位
		if ( jj1==0)
		{
			int iii;
			uint8_t PID[4]={'P','I','D','1'};
			for(iii=0;iii<4;iii++)
			pPKG->PID[iii]=PID[iii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj1=0;
		}
		else //添加不同的PID 
		{
			int ii;
			uint8_t PID2[4]={'P','I','D','2'};
			for(ii=0;ii<4;ii++)
			pPKG->PID[ii]=PID2[ii];
			cout<< "add pid =  "<<pPKG->PID<<endl;
			jj=0;
		}
	}	
	//****************************发送数据包get包**********************************
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