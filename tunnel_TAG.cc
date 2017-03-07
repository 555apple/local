/*改动点：
1、增加了DSTIP_TBG的查表功能；
2、增加REQUST_TBG模块：收到IP包后若查表无匹配项RM发送请求；
3、增加RECV_TBG模块： 收到RM的返回结果并将其插入DSTIP_TBG表中;
4、增加了包缓存功能：在未匹配到TBG时，将包缓存下来，待rm返回结果后先发送缓存包
5、增加了包构造，分片的一、二分片在同一个模块完成，且脚本中只用接收一次。
6、将get包增加了一个4字节的reserved字段，用于存储隧道编号
*/
#include <click/config.h>
#include <iostream>
#include <elements/ip/lineariplookup.hh>//线性	IP查找模块
#include <elements/ip/radixiplookup.hh>//树型IP查找模块
#include "tunnel_TAG.hh"
using namespace std;
CLICK_DECLS

//data_handin
REQUST_TBG::REQUST_TBG(){}
REQUST_TBG::~REQUST_TBG(){}
Packet * REQUST_TBG::simple_action(Packet *p)
{
  return p;
}

RECV_TBG::RECV_TBG(){}
RECV_TBG::~RECV_TBG(){}
Packet * RECV_TBG::simple_action(Packet *p)
{
  return p;
}

CoLoR_Encapsulation::CoLoR_Encapsulation(){}
CoLoR_Encapsulation::~CoLoR_Encapsulation(){}
Packet *
CoLoR_Encapsulation::simple_action(Packet *p)
{
  return p;
}

CoLoR_Decapsulation::CoLoR_Decapsulation(){}
CoLoR_Decapsulation::~CoLoR_Decapsulation(){}
Packet *
CoLoR_Decapsulation::simple_action(Packet *p)
{
 return p;
}
//update
Updatepid_TAG_Creat::Updatepid_TAG_Creat(){}//构造函数和析构函数都是没有返回值的
Updatepid_TAG_Creat::~Updatepid_TAG_Creat(){}
Packet *Updatepid_TAG_Creat::simple_action(Packet *p)
{
  return p;
}

Updatepid_TAG_Send::Updatepid_TAG_Send(){}//构造函数和析构函数都是没有返回值的
Updatepid_TAG_Send::~Updatepid_TAG_Send(){}
Packet *Updatepid_TAG_Send::simple_action(Packet *p)
{
  return p;
}

Updatepid_TAG_Process::Updatepid_TAG_Process(){}//构造函数和析构函数都是没有返回值的
Updatepid_TAG_Process::~Updatepid_TAG_Process(){}
Packet *Updatepid_TAG_Process::simple_action(Packet *p)
{
  return p;
} 
//tunnel_maint//2016.12.05
//保活包定时发送
TAG_MAINT_SEND::TAG_MAINT_SEND(){}
TAG_MAINT_SEND::~TAG_MAINT_SEND(){}
Packet * TAG_MAINT_SEND::simple_action(Packet *p)
{
  return p;
}
//处理保活应答包
TAG_MAINT_RECV::TAG_MAINT_RECV(){}
TAG_MAINT_RECV::~TAG_MAINT_RECV(){}
Packet * TAG_MAINT_RECV::simple_action(Packet *p)
{
  return p;
}
//故障上报
TROUBLE_REPORT::TROUBLE_REPORT(){}
TROUBLE_REPORT::~TROUBLE_REPORT(){}
Packet * TROUBLE_REPORT::simple_action(Packet *p)
{
  return p;
}
//故障确认
TROUBLE_CONFIRM::TROUBLE_CONFIRM(){}
TROUBLE_CONFIRM::~TROUBLE_CONFIRM(){}
Packet * TROUBLE_CONFIRM::simple_action(Packet *p)
{
  return p;
}
//apple/t
//******************************维护模块各种定义*********************************
//隧道的开启和关闭关键字由数据模块控制
char MAINT_SID[13]={'t','u','n','n','e','l','_','m','a','i','n','t','\0'};//隧道维护包标志
char TROUBLE_SID[15]={'t','r','o','u','b','l','e','_','r','e','p','o','r','t','\0'};
char TUNNEL_DOWN[14]={'p','i','d','u','p','d','a','t','e','d','o','w','n','\0'}; //隧道关闭通告,由PID更新包通告，方便RM对更新包识别
int open =1 ; //值为1时，表明隧道打开
int off=0      ;//值为1时，表明隧道关闭 
uint16_t Keepalive_time = 10; //每隔10s进行一次互动
uint16_t Keepalive_state_time = 21; //判断故障的条件为（2×Keepalive_time+1×rtt）时间没有收到对方的保活应答包（只要收到应答就行，不在乎序号）
int Keepalive_launch = 1 ; //值为1表示keepalive计时器到时
int Keepalive_state_launch = 1; //值为1表示keepalive_state计时器到时
int KEEPALIVE_STATE = 1; //1表示在保活状态计时时间内收到了保活应答包
int ALARM_Breakdown =0; //警告标志，1表示隧道出现故障
int TROUBLE_CONFIRMATION = 0 ; //由RM确认。1表示有故障；0表示无故障，属于故障误报
//MAINT_end

//***************************PID更新模块各种定义***************************
//int NEW_TTL_TIMER1= 0;       //pidlist是否发生更新标志位，1表示发生，0表示没有发生，更新则上一个定时器到时动作无效
int PKG_RECV  = 0;               //是否有更新包到来的标志位，1表示有，0表示没有
int F_TIMER_launch =0;                   //发包频率定时器到时动作标志
int Frequence=4 ;	    	 //存储发包周期
int TTL_TIMER_launch=0;	      //TTL计时器到时动作标志位
int TTL=12;		      //存储PID的生存时间;
int INIT=1;		      //首次允许发包标志
int Emergency_GET =0;        //是否收到对方紧急请求更新包的标志位
int STATE = 0;		     //收发包状态位
int S_TIME = Frequence/2;		     //设定超时时间为2秒
int S_TIMER_launch = 0;		     //超时定时器到时动作标志位
int Get_NUM  =1;	     //发送的get包编号
int Data_NUM  =0;	    //接收的data包编号
int RECV_Emergency_Get_Num=0; //接收到的对方紧急更新请求包的数量
int  STARTOVER = 1 ;         //是否启动pid更新标志位，1代表启动，0代表不启动
char TUNNEL_SID[15]={'t','u','n','n','e','l','_','s','e','r','v','i','c','e','\0'};    //隧道包标志
char UPDATE_SID[10]={'p','i','d','u','p','d','a','t','e','\0'};    //更新包标志
//update

//***************************数据模块各种定义***************************
#define IN_DEVICE          "h2-eth0"  //程序的参数配置-自动配置时使用
#define OUT_DEVICE         "h2-eth1"
#define DEVICELENGTH        10
#define bufsize     1024 * 5
int add_TBG =0;
int tunnel_id_LMP ;
int RecvBufSize = bufsize;
unsigned char RecvBuf1[bufsize] = {0};
unsigned char RecvBuf2[bufsize] = {0};
uint8_t TUNNEL_SID_intag[14]={'t','u','n','n','e','l','_','s','e','r','v','i','c','e'}; //用于封装部分sid

//加了类型属性+隧道编号的计时器模块
//***************************定时器函数**********************************
#define MAX_TIMER_CNT 64
#define MUL_TIMER_RESET_SEC 1//初始定时时间
#define TIMER_UNIT 1//定时间隔
#define INVALID_TIMER_HANDLE (-1)
//计时器类型值
#define TTL_TIMER_TYPE 1            	//用于PID列表中PID生存时间计时
#define F_TIMER_TYPE  2              	 //用于周期发送PID更新包计时
#define PID_STATE_TIMER_TYPE 3	     //用于PID更新包状态计时
#define OFF_TIMER_TYPE 4	  	   //用于检测数据流控制隧道关闭计时
#define KEEPALIVE_TIMER_TYPE 5 	//用于周期发送隧道维护包计时
#define KEEPALIVE_STATE_TIMER_TYPE 6      //用于keepalive包状态计时
#define UT_TIMER_TYPE 7             	//用于隧道关系表的维护
int TIMER_ID = 0;
int launch_tunnel_id=0;

struct _timer_info
{
	int timer_id; /* 定时器编号 */
	int timer_type;//定时器类型
	int elapse; /* 相对前一个子定时器的剩余时间 */
	int tunnel_id;/* 隧道编号*/
	time_t time;/* 自系统时间来已经经过的时间(1970年00:00:00) */
	int (* timer_proc) (int num);//动作入口
	struct _timer_info* pre;
	struct _timer_info* next;
};

struct _timer_manage
{
	void (* old_sigfunc)(int);
	void (* new_sigfunc)(int);
	struct itimerval value, ovalue;
	struct _timer_info* timer_info;
};

typedef int timer_handle_t;
struct _timer_manage timer_manage;

//获取系统时间
void get_format_time(char *tstr)
{
	time_t t;
	
	t = time(NULL);
	strcpy(tstr, ctime(&t));
	tstr[strlen(tstr)-1] = '\0';
	
	return;
}

//主定时器到时动作函数：只对表头结点进行操作
void sig_func(int signo)
{
	int id;
	char tstr[200];
	int ret = 0;

	if(timer_manage.timer_info != NULL)
	{
		timer_manage.timer_info->elapse--; //只对表头结点时间减1
		//cout<<" int function sig_func() : timer_manage.timer_info->timer_type="<<timer_manage.timer_info->timer_type<<endl;
		while(timer_manage.timer_info->elapse <= 0)
		{
		   id = timer_manage.timer_info->timer_id;
		   ret = timer_manage.timer_info->timer_proc(id);

		  //定时器已空
		  if(ret)
		  {
			break;
		  }
		}
	}
}

//设定主定时器
/* success, return 0; failed, return -1 */
int init_mul_timer(struct _timer_manage *timer_manage)
{
	int ret;
	//设定定时器动作
	if( (timer_manage->old_sigfunc = signal(SIGALRM, sig_func)) == SIG_ERR)
	{
		return (-1);
	}
	timer_manage->new_sigfunc = sig_func;
	//设定定时周期
	timer_manage->value.it_value.tv_sec = MUL_TIMER_RESET_SEC;
	timer_manage->value.it_value.tv_usec = 0;
	timer_manage->value.it_interval.tv_sec = TIMER_UNIT;
	timer_manage->value.it_interval.tv_usec = 0;
	ret = setitimer(ITIMER_REAL, &timer_manage->value, &timer_manage->ovalue); 
	
	return (ret);
}

//清除主定时器
int destroy_mul_timer(struct _timer_manage *timer_manage)
{
	int ret;
	
	if( (signal(SIGALRM, timer_manage->old_sigfunc)) == SIG_ERR)
	{
		return (-1);
	}

	memset(timer_manage, 0, sizeof(struct _timer_manage));
	ret = setitimer(ITIMER_REAL, &timer_manage->ovalue, &timer_manage->value);
	if(ret < 0)
	{
		return (-1);
	} 
	
	printf("destroy multi timer\n");
	return ret;
}

//清除子定时器
struct _timer_info* del_a_timer(struct _timer_info* head, int value)
{
	struct _timer_info* p;

	p = head;

	if(head == NULL)
	{
		printf("no timer %d\n", value);
		return NULL;
	}

	//删除第一个元素
	if(p->timer_id == value)
	{
		if(p->next == NULL)
		{
			printf("delete the timer %d\n", value);
			//cout<<"TTL_TIMER_launch ="<<TTL_TIMER_launch<<endl;
			return NULL;
		}

		head = p->next;
		head->pre = NULL;
		free(p);
		printf("delete the timer %d\n", value);
		//cout<<"TTL_TIMER_launch ="<<TTL_TIMER_launch<<endl;
		return head;
	}

	//删除中间元素
	while(p->next != NULL)
	{
		if(p->timer_id == value)
		{
			p->next->pre = p->pre;
			p->pre->next = p->next;
			free(p);
			printf("delete the timer %d\n", value);
			//cout<<"TTL_TIMER_launch ="<<TTL_TIMER_launch<<endl;
			return head;
		}

		p = p->next;
	}

	//删除最后一个元素
	if(p->timer_id == value)
	{
		p->pre->next = NULL;
		free(p);
		printf("delete the timer %d\n", value);
		//cout<<"TTL_TIMER_launch ="<<TTL_TIMER_launch<<endl;
		return head;
	}

	printf("no timer %d\n", value);
	return head;
}

//升序插入子定时器
struct _timer_info* insert_a_timer(struct _timer_info* head, struct _timer_info* pnew)
{
	struct _timer_info* p;
	time_t t;

	t = time(NULL);
	//cout<<" time = "<<t<<endl;
	if(difftime(pnew->time, t) < 0) //difftime()功 能:返回两个time_t型变量之间的时间间隔，即 计算两个时刻之间的时间差。
	{
		printf("the time is past\n");
		return head;
	}

	p = head;

	//空链表直接插入
	if(head == NULL)
	{
		head = pnew;
		pnew->pre = NULL;
		pnew->next = NULL;
		pnew->elapse = difftime(pnew->time, t);
		return head;
	}

	//表头插入，作为第一个元素
	if(difftime(p->time, pnew->time) >= 0) //表头结点的时刻晚于要插入的新结点
	{
		/*cout<<"p->time = "<<p->time<<"      p->timer_id = "<<p->timer_id<< "      p->timer_type = "<<p->timer_type<<endl;
		cout<<"pnew->time = "<<pnew->time<<"      pnew->timer_id = "<<pnew->timer_id<< "        pnew->timer_type = "<<pnew->timer_type<<endl;
		cout<<" difftime(p->time, pnew->time) ="<<difftime(p->time, pnew->time) <<endl;
		*/head = pnew;
		pnew->pre = NULL;
		pnew->next = p;
		p->pre = pnew;
		pnew->elapse = difftime(pnew->time, t);
		p->elapse = p->elapse - pnew->elapse;
		return head;
	}

	while(p->next != NULL)
	{
		if(difftime(p->time, pnew->time) >= 0)
		{
			/*cout<<"p->time = "<<p->time<<"   p->timer_id = "<<p->timer_id<< "     p->timer_type = "<<p->timer_type<<endl;
			cout<<"pnew->time = "<<pnew->time<<"   pnew->timer_id = "<<pnew->timer_id<< "       pnew->timer_type = "<<pnew->timer_type<<endl;
			cout<<" difftime(p->time, pnew->time) ="<<difftime(p->time, pnew->time) <<endl;
			*/p->pre->next = pnew;
			pnew->next = p;
			pnew->pre = p->pre;
			p->pre = pnew;
			p->elapse = difftime(p->time, pnew->time);
			pnew->elapse = difftime(pnew->time, pnew->pre->time);
			return head;
		}

		p = p->next;
	}

	//最后一个元素:表尾插入，作为最后一个结点
	if(difftime(p->time, pnew->time) < 0)
	{
		/*cout<<"p->time = "<<p->time<<"      p->timer_id = "<<p->timer_id<< "     p->timer_type = "<<p->timer_type<<endl;
		cout<<"pnew->time = "<<pnew->time<<"      pnew->timer_id = "<<pnew->timer_id<< "     pnew->timer_type = "<<pnew->timer_type<<endl;
		cout<<" difftime(p->time, pnew->time) ="<<difftime(p->time, pnew->time) <<endl;
		*/p->next = pnew;
		pnew->pre = p;
		pnew->next = NULL;
		pnew->elapse = difftime(pnew->time, p->time);
		return head;
	}
	else//最后一个元素的前面插入
	{
		/*cout<<"p->time = "<<p->time<<"      p->timer_id = "<<p->timer_id<< "     p->timer_type = "<<p->timer_type<<endl;
		cout<<"pnew->time = "<<pnew->time<<"      pnew->timer_id = "<<pnew->timer_id<< "     pnew->timer_type = "<<pnew->timer_type<<endl;
		cout<<" difftime(p->time, pnew->time) ="<<difftime(p->time, pnew->time) <<endl;
		*/p->pre->next = pnew;
		pnew->next = p;
		pnew->pre = p->pre;
		p->pre = pnew;
		p->elapse = difftime(p->time, pnew->time);
		pnew->elapse = difftime(pnew->time, pnew->pre->time);
		return head;
	}
}

//打印管理定时器结构体
void print_timer_manage(struct _timer_info* head)
{
	struct _timer_info* p;

	p = head;
	
	printf("timer_id\ttype\telapse\ttunnel_id\n");

	while(p != NULL)
	{
		printf("%d\t\t%d\t%d\t%d\n", p->timer_id, p->timer_type, p->elapse,p->tunnel_id);
		p = p->next;
	}
}

//设定子定时器执行启动函数
int timer_proc_launch(int num)
{
	char tstr[200];
	get_format_time(tstr);
	//绝对是表头结点到时！
	printf("\n%s: timer_proc:%d is launched. timer_type: %d\n", tstr, num,timer_manage.timer_info->timer_type); 
  	//提取到时定时器所属的隧道
  	launch_tunnel_id = timer_manage.timer_info->tunnel_id;
  	//根据type值来判断到时的计时器类型，执行该类型相应的动作
  	if ( timer_manage.timer_info->timer_type == 1)
  	{
		//cout<<"1111timer_manage.timer_info->timer_type = "<<timer_manage.timer_info->timer_type<<endl;
		//cout<<"before : TTL_TIMER_launch =  " <<TTL_TIMER_launch<<endl;
		TTL_TIMER_launch=1;
		//cout<<"after : TTL_TIMER_launch =  " <<TTL_TIMER_launch<<endl;
  	}
	else if(timer_manage.timer_info->timer_type == 2 )//表示F_TIMER定时器到时
	{
		//cout<<" 2222timer_manage.timer_info->timer_type = "<<timer_manage.timer_info->timer_type<<endl;
		//cout<<"before : F_TIMER_launch =  " <<F_TIMER_launch<<endl;
		F_TIMER_launch=1;
		//cout<<"after : F_TIMER_launch =  " <<F_TIMER_launch<<endl;
	}
	else if(timer_manage.timer_info->timer_type == 3 )//表示S_TIMER定时器到时
	{
		//cout<<"333timer_manage.timer_info->timer_type = "<<timer_manage.timer_info->timer_type<<endl;
		//cout<<"before : S_TIMER_launch =  " <<S_TIMER_launch<<endl;
		S_TIMER_launch=1;
		//cout<<"after : S_TIMER_launch =  " <<S_TIMER_launch<<endl;
	}
  	else if(timer_manage.timer_info->timer_type == 5 )//表示keepalive定时器到时
	{
		Keepalive_launch=1;
	}
	else if(timer_manage.timer_info->timer_type == 6 )//表示keepalive_state计时器到时
	{
		Keepalive_state_launch=1;
		//在状态计时器结束时判断时间段内是否收到TBG的保活应答包
		if (KEEPALIVE_STATE ==1)
		{
			cout<<"**********************************"<<endl;
			cout<<"* Tunnel is working properly!    *"<<endl;
			cout<<"**********************************"<<endl;
			ALARM_Breakdown =0;//隧道无故障
		}
		else
		{
			cout<<"**********************************"<<endl;
			cout<<"* Tunnel has broken down!       *"<<endl;
			cout<<"**********************************"<<endl;
			ALARM_Breakdown =1;//警报！隧道出现故障
		}
	}
	//删除到时的子定时器条目
	timer_manage.timer_info = del_a_timer(timer_manage.timer_info, num);
	//print_timer_manage(timer_manage.timer_info);
	
	if(timer_manage.timer_info == NULL)
	{
		return 1;
	}

	return 0;
}
//插入子定时器
int set_child_timer(long delay, int id, int type, int tunnel_id){
	struct _timer_info* timer_info;
	time_t t = time(NULL);
	timer_info = (struct _timer_info*)malloc(sizeof(struct _timer_info));
	timer_info->timer_type = type;//hu
	timer_info->timer_id = id;
	timer_info->time = delay + t; //定时时长+此时时刻=到时时刻
	timer_info->tunnel_id = tunnel_id;
	timer_info->timer_proc = timer_proc_launch;	
	timer_manage.timer_info = insert_a_timer(timer_manage.timer_info, timer_info);
}
int timer_unlaunch = 1; //定时器初始化
//***************************定时器函数_end**********************************

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
	uint8_t reserved[4];//4字节的保留字段：reserved[0]用于RM标识同域或异域，reserved[1]用于存储隧道编号
	
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
//*****************获取本机网口的mac和ip函数***************************
char in_device1[10];
uint8_t in_mac1[7];
uint8_t in_ip1[5];
char out_device1[10];
uint8_t out_mac1[7];
uint8_t out_ip1[5];
int GetLocalMac1 ( const char *device,uint8_t *mac,uint8_t *ip )
{
	int sockfd;
	struct ifreq req;
	struct sockaddr_in * sin;
	
	if ( ( sockfd = socket ( PF_INET,SOCK_DGRAM,0 ) ) ==-1 )
	{
		fprintf ( stderr,"Sock Error:%s\n\a",strerror ( errno ) );
		close(sockfd);
		return ( -1 );
	}
	
	memset ( &req,0,sizeof ( req ) );
	strcpy ( req.ifr_name,device );

	if ( ioctl ( sockfd,SIOCGIFHWADDR, ( char * ) &req ) ==-1 )
	{
		fprintf ( stderr,"ioctl SIOCGIFHWADDR:%s\n\a",strerror ( errno ) );
		close ( sockfd );
		return ( -1 );
	}
	memcpy ( mac,req.ifr_hwaddr.sa_data,6 );
	
	req.ifr_addr.sa_family = PF_INET;
	if ( ioctl ( sockfd,SIOCGIFADDR, ( char * ) &req ) ==-1 )
	{
		fprintf ( stderr,"ioctl SIOCGIFADDR:%s\n\a",strerror ( errno ) );
		close ( sockfd );
		return ( -1 );
	}
	sin = ( struct sockaddr_in * ) &req.ifr_addr;
	memcpy ( ip, ( char * ) &sin->sin_addr,4 );
	close(sockfd);
	return 0;
}
//*************************各种表的定义*********************
HashMap<IPAddress,String >DstIP_TBG_tbl;
struct pac_cache
{
	uint32_t length;
	char *p; 
};
HashMap<int,struct pac_cache>packet_cache; //缓存数据包
/*************************隧道关系表规则*************************
1、仅根据RM回送的TBG_info应答包进行添加条目、修改条目动作;
2、当收到RM回送的TBG_info应答包时根据以下规则产生相应动作：
	（1）查表：若关系表为空，则添加为第一条隧道条目信息，并将其编号为“隧道1”，例如：
		tunnel_id           dst_net        TBG_NID    position        state
		      1            192.168.0.0        TBG1        1(同域)      1（开启）
	（2）查表：若关系表不为空，则匹配所有条目中的“目的网络”字段
		1）若无匹配项，则在表尾新增一条表项，并顺序赋值“隧道编号”，例如：
		tunnel_id           dst_net        TBG_NID      position        state
		      1            192.168.0.0        TBG1        1（同域）      1（开启）
		      2             172.16.0.0         TBG2        0（异域）      1（开启）
		2）若有匹配项，则判断该条目的“TBG_NID”项与RM回送的TBG是否相同
			（a）相同，则该条目信息保持不变（除“state”项）。例如：
			tunnel_id           dst_net        TBG_NID      position        state
			      1            192.168.0.0        TBG1        1（同域）      1（开启）
			（b）不同，则说明RM更改了到达对应net的隧道边界网关，此时修改对应条目信息，例如：
			tunnel_id           dst_net        TBG_NID      position        state
			      1            192.168.0.0        TBG3        0（异域）      1（开启）
备注：
1、出现（2）2）（a）情况的原因：隧道关系表维护模块进行隧道关系表维护期间，RM回应的关于到达网络192.168.0.0的TBG没有发生变化;
2、出现（2）2）（b）情况可能的原因有：
	原因1：故障检测模块作用。
	具体：隧道1的故障检测模块向RM上报了关于TBG1的“疑似故障”，经RM核实TBG1确实出现了故障。
因此对TAG重新发出的到达192.168.0.0网络的TBG_REQUST作出了应答，回应了新的隧道边界网关信息“TBG3”.
	原因2：隧道关系表维护模块作用。
	具体：在隧道关系表的更新维护过程中，（1）可能1：RM收到了来自新结点TBG3的注册信息，
为比TBG1更优的达到192.168.0.0网络最佳隧道边界网关。因此RM回送了最佳TBG信息，注意此时TBG1并没有故障。（2）可能2：RM在收到隧道1的“疑似故障”上报前就知道TBG1已经故障。
因此向TAG回应没有故障、能到达192.168.0.0网络的其它TBG信息。
附：
隧道关系表中“state”项值由隧道开/关模块决定		   
*/
//路由表参数（dst_net ----  tunnel_id）
RadixIPLookup rt;  //定义LinearIPLookup类的一个对象rt
IPRoute route; //作为add函数的参数
struct _tunnel_list_info
{
	int tunnel_id; //隧道编号
	IPAddress  dst_net; //dest_net
	uint8_t  TBG_NID[16];//TBG_NID
	int position;// 位置：1（同域）/0（异域）
	int state;//状态：1（开启）/0（关闭）
	struct _tunnel_list_info *next;	
};
struct _tunnel_list_info *TUNNEL_LIST; //表头结点。此时的TUNNEL_LIST只是一个空指针，未指向任何地址
//打印当前所有的PID列表
void print_tunnel_list_all(struct _tunnel_list_info* head)
{
	struct _tunnel_list_info* p;
	p=head;
	int i=1;
	while(p!=NULL)
	{

		if (i ==1)
		{
			printf("--------------------------------TUNNEL TABLE----------------------------------\n");
			printf("|tunnel_entry  | tunnel_id | dest_net          | TBG_NID | position | state  |\n" );
			printf("------------------------------------------------------------------------------\n");
			i=0;
		}
		printf("|      %d       |    %d      | %s      |   %s  |    %d     |   %d    |\n",p->tunnel_id,p->tunnel_id,p->dst_net.unparse().c_str(),p->TBG_NID,p->position,p->state );
		printf("------------------------------------------------------------------------------\n");
		p=p->next;
	}
}
//打印指定结点的隧道关系表
void print_tunnel_list_single(struct _tunnel_list_info* p)
{
	int i =1;
	if (i ==1)
	{
		printf("--------------------------------TUNNEL TABLE----------------------------------\n");
		printf("|tunnel_entry  | tunnel_id | dest_net          | TBG_NID | position | state  |\n" );
		printf("------------------------------------------------------------------------------\n");
		i=0;
	}
	printf("|      %d       |    %d      | %s      |   %s  |    %d     |   %d    |\n",p->tunnel_id,p->tunnel_id,p->dst_net.unparse().c_str(),p->TBG_NID,p->position,p->state );
	printf("------------------------------------------------------------------------------\n");
}
//查找匹配的"dst_net“结点，查找成功返回找到的表结点指针，失败返回空指针
struct _tunnel_list_info* match_dst_net(struct _tunnel_list_info *head, IPAddress dst_net)
{
	printf("--->>> Into Function [match_dst_net()]\n");
	struct _tunnel_list_info *p;
	p=head;
	if (p ==NULL)
	{
		return NULL;
	}
	else
	{
		//匹配成功，说明条目已经存在，需要进一步匹配TBG_NID项
		while(p->next != NULL )//有多个结点时
		{
			//c++中string类的比较用compare。相等a.compare(b)=0;
			if (p->dst_net.unparse().compare(dst_net.unparse() )==0)//匹配表头或中间结点
			{
				printf("--->>> Result : dst_net match successfully in tunnel_entry %d !\n",p->tunnel_id);
				return p;
			}
			p=p->next;
		}
		if (p->dst_net.unparse().compare(dst_net.unparse()) ==0)//匹配表头或中间结点
		{
			printf("--->>> Result : dst_net match successfully in tunnel_entry %d !\n",p->tunnel_id);
			return p;
		}
		//匹配失败，说明没有该条目，则在表尾添加新的条目信息
		else
		{
			printf("--->>> Result : cannot match dst_net!\n");
			return NULL;
		}
	}
	
}
//继dst_net项匹配成功后，进一步匹配该条目的TBG_NID字段。成功表示不修改条目;失败表示要修改条目信息,返回需要修改的结点指针
struct _tunnel_list_info* match_TBG_NID(struct _tunnel_list_info *pmatched, uint8_t TBG_NID[16])
{
	printf("--->>> Into Function [match_TBG_NID()]\n");
	struct _tunnel_list_info *p;
	p=pmatched; //已经成功匹配”dst_net“项的条目指针
	if (strcmp((char*)p->TBG_NID, (char*)TBG_NID) ==0)//匹配该条目的TBG_NID字段
	{
		printf("--->>> Result : TBG_NID match successfully! [tunnel_entry %d] don`t need change!\n",pmatched->tunnel_id);
		return NULL;
	}
	else
	{
		printf("--->>> Result : cannot match TBG_NID, [tunnel_entry %d] need change!\n", pmatched->tunnel_id);
		return p;
	}
}
//插入新的隧道关系表结点
struct _tunnel_list_info* insert_tunnel_list(struct _tunnel_list_info *head, struct _tunnel_list_info *pnew)
{
	struct _tunnel_list_info *p;
	p=head;
	//表空，则直接在表头节点插入第一条隧道信息
	if (head == NULL)
	{
		printf("--->>> Into Function [insert_tunnel_list()]\n");
		printf("--->>> Empty table, insert the first tunnel_entry\n");
		pnew->tunnel_id=1;//第一条隧道编号固定为1;
		head = pnew;
		pnew->next = NULL;
		//****添加一条路由*********
		memset(&route,0,sizeof(IPRoute));
		route.addr=pnew->dst_net;     //目的网络
		route.mask=0x00ffffff;        //掩码=255.255.255.0
		route.gw=0x0;                     //网关可以为TBG的IP，暂定位0
		route.port=pnew->tunnel_id;	          //出口=隧道编号
		ErrorHandler *errh;
		int f=rt.add_route(route,true, &route,errh); //返回值为0时，添加成功！
		if (f==0)
		{
		printf("-------------------------ROUTE TABLE----------------------------\n");
		printf("Dst_net		Mask		Gateway		TUNNEL_ID\n");
		cout<<route.addr.unparse().c_str()<<"\t";//点分十进制打印
		cout<<route.mask.unparse().c_str()<<"\t";//点分十进制打印
		cout<<route.gw.unparse().c_str()<<"\t\t";//点分十进制打印
		cout<<route.port<<endl<<endl;
		}	
		return head;
	}
	//表不空，则匹配“dst_net”字段
	else
	{
		struct _tunnel_list_info *pmatched;
		pmatched=match_dst_net(TUNNEL_LIST, pnew->dst_net);
		//匹配“dst_net”字段失败，则在表尾添加新的隧道条目
		if (pmatched == NULL)
		{
			while(p->next != NULL)//找到最后一个表节点
			{
				p=p->next;
			}
			printf("--->>> Insert a new tunnel_entry\n");
			int tunnel_id =p->tunnel_id;
			pnew->tunnel_id = ++tunnel_id; //新增条目的隧道编号为前一个结点+1
			p->next=pnew;	//将新的表结点插到表尾
			//****添加一条路由*********
			memset(&route,0,sizeof(IPRoute));
			route.addr=pnew->dst_net;     //目的网络
			route.mask=0x0000ffff;        //掩码=255.255.0.0
			route.gw=0x0;                     //网关可以为TBG的IP，暂定位0
			route.port=pnew->tunnel_id;	          //出口=隧道编号
			ErrorHandler *errh;
			int f=rt.add_route(route,true, &route,errh); //返回值为0时，添加成功！
			if (f==0)
			{
				cout<<"--->>> Add a new Route successfully !"<<endl;
				printf("\n-------------------------ROUTE TABLE----------------------------\n");
				printf("Dst_net		Mask		Gateway		TUNNEL_ID\n");
				cout<<route.addr.unparse().c_str()<<"\t";//点分十进制打印
				cout<<route.mask.unparse().c_str()<<"\t";//点分十进制打印
				cout<<route.gw.unparse().c_str()<<"\t\t";//点分十进制打印
				cout<<route.port<<endl<<endl;
			}
		}
		//匹配“dst_net”字段成功，则在进一步匹配该条目的TBG_NID字段
		else
		{	
			struct _tunnel_list_info *pchanged;
			pchanged=match_TBG_NID(pmatched, pnew->TBG_NID);
			//该隧道的TBG已经发生改变，修改该隧道关系条目的TBG信息
			if (pchanged != NULL)
			{
				printf("--->>> Change [tunnel_entry %d ] information\n", pchanged->tunnel_id);
				//注意隧道编号一定要保持不变！pchanged->tunnel_id不修改
				memcpy(pchanged->TBG_NID,pnew->TBG_NID,sizeof(pnew->TBG_NID));
				pchanged->position = pnew->position;
			}
		}
	}
	return head;
}

//设置新的tunnel_list列表参数并插入条目和打印
void set_tunnel_list(IPAddress dst_net, uint8_t TBG_NID[16], int position,int state)
{
	struct _tunnel_list_info *tunnel_list; // 用来承载新条目信息
	tunnel_list=(struct _tunnel_list_info *)malloc(sizeof(struct _tunnel_list_info));//如果分配成功，返回指向被分配内存的指针
	//memcpy( tunnel_list->dst_net,dst_net,sizeof(dst_net));
	tunnel_list->dst_net=dst_net;
	memcpy( tunnel_list->TBG_NID,TBG_NID,16);
	tunnel_list->position = position;
	tunnel_list->state = state;
	tunnel_list->next = NULL; //一定要把新节点的next赋值，不然会出错的
	TUNNEL_LIST=insert_tunnel_list(TUNNEL_LIST, tunnel_list);//返回表头指针
	char tstr[200];
	get_format_time(tstr);
	printf("[%s] Now all TUNNEL_LIST are as showed:\n", tstr);
	print_tunnel_list_all(TUNNEL_LIST);
}
//路由表最长前缀匹配
int LPM(IPAddress dst_ip)	
{
	//****查找路由************
	IPAddress gw;//用来存放查找得到的网关;
	printf("--->>> Into Function [LPM()]\n");
	cout<<"--->>> dst_ip= "<<dst_ip.unparse().c_str()<<"   Lookup route table ......"<<endl;
	int p;
	p=rt.lookup_route(dst_ip, gw);//最长前缀匹配失败返回-1
	if (p==-1)
	{ 
		cout<<"--->>> LPM fail !  Send REQUST_TBG packet to RM !"<<endl;
	}
	else
	{
		printf("--->>> LAM success ! Found Result : ");
		cout<<"tunnel_id="<<p<<endl;
		//cout<<"gw="<<gw.unparse().c_str()<<endl<<endl;
	}
	return p;
}
//为数据处理模块添加TBG_NID, 以LPM()函数返回值作为参数
struct _tunnel_list_info* find_tunnel_entry(struct _tunnel_list_info *head, int tunnel_id)
{
	if (tunnel_id == -1 )
	{
		cout<<"--->>> Please wait TBG_info from RM"<<endl;
	}
	else
	{
		struct _tunnel_list_info *p;
		p=head;
		while(p->next != NULL )//有多个结点时
		{
			if (p->tunnel_id == tunnel_id )//匹配表头或中间结点
			{
				printf("--->>> Result : Find TBG = [ %s ] in [ tunnel %d ] !\n",p->TBG_NID,p->tunnel_id);
				return p;
			}
			p=p->next;
		}
		if(p->tunnel_id == tunnel_id )//匹配表头或中间结点
		{
			printf("--->>> Result : Find TBG = [ %s ] in [ tunnel %d ] !\n",p->TBG_NID,p->tunnel_id);
			return p;
		}
	}
}
//*************************与RM请求交互*****************************
void REQUST_TBG::push(int port, Packet *packet)
{
	cout<<endl<<"--->>> Module: void REQUST_TBG::push(int port, Packet *packet)"<<endl;
	//判断客户端的请求类型，仅需首次进行判断
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	int Recvlength;
	Recvlength = packet->length();
	memcpy(RecvBuf,packet->data(),packet->length());
	//**********提取数据包的类型****************
	int v=0; int version=0;
	memcpy(&v , RecvBuf+14,1 );
	int c[8];
	c[0]=v/128;v%=128; //最高位
	c[1]=v/64;v%=64;
	c[2]=v/32;v%=32;
	c[3]=v/16;v%=16;
	c[4]=v/8;v%=8;
	c[5]=v/4;v%=4;
	c[6]=v/2;v%=2;
	c[7]=v;
	//for(int j=0;j<=7;j++) cout<<c[j];
	version=c[0]*8+c[1]*4+c[2]*2+c[3]*1; //前四位为version字段，IP=0100, CoLoR=1010;
	//cout<<version<<endl;
	if (version== 4)//为IP包，则根据目的IP查找TBG
	{
		IPAddress DST_IP1;
		memcpy(&DST_IP1,RecvBuf+14+16,4);//提取目的IP
		cout<<"--->>> Client type : [  IP ]     dst_IP="<<DST_IP1.unparse().c_str()<<endl;//点分十进制打印
		//最长前缀匹配
		tunnel_id_LMP = LPM(DST_IP1);//最长前缀匹配的结果，返回隧道编号
		if ( tunnel_id_LMP == -1) //匹配失败时，向RM请求TBG
		{
			//uint8_t DST_IP[4]; //用来存IP包的目的IP
			//memcpy(DST_IP,RecvBuf+14+16,4);//提取目的IP
			//printf("dst_ip:%d.%d.%d.%d\n",DST_IP[0],DST_IP[1],DST_IP[2],DST_IP[3] );
			struct CoLoR_NEW_GET PKG;
			memset(&PKG,0,sizeof(CoLoR_NEW_GET));	
			//封装mac层头部
			PKG.ether_dhost[0] = 0xCC;//目的地为RM
			PKG.ether_dhost[1] = 0xCC;
			PKG.ether_dhost[2] = 0xCC;
			PKG.ether_dhost[3] = 0xCC;
			PKG.ether_dhost[4] = 0xCC;
			PKG.ether_dhost[5] = 0xCC;
			PKG.ether_shost[0] = 0xAA;//源地址
			PKG.ether_shost[1] = 0xAA;
			PKG.ether_shost[2] = 0xAA;
			PKG.ether_shost[3] = 0xAA;
			PKG.ether_shost[4] = 0xAA;
			PKG.ether_shost[5] = 0xAA;
			PKG.ether_type = 0x0008;	

			PKG.version_type=160;//10100000
			PKG.ttl=255;
			PKG.total_len=126;

			PKG.port_no1=1;
			PKG.port_no2=2;

			PKG.minpid=0;
			PKG.pids_o=0;//00000000,pid数量占7位，offset字段占一位
			PKG.res=0;

			int iii;
			uint8_t offset[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.offset[iii]=offset[iii];

			uint8_t length[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.length[iii]=length[iii];

			PKG.content_len=1000;
			PKG.mtu=1500;

			PKG.publickey_len=256;
			PKG.checksum=123;

			memcpy((char *)PKG.n_sid,"TAG",3);
			memcpy(PKG.l_sid,TUNNEL_SID_intag,14); //封装隧道服务标识
			memcpy(PKG.l_sid+14,RecvBuf+14+16,4); //封装目的IP
			
			/*
			cout<<"11111"<<endl;
			printf("dst_ip:%d.%d.%d.%d\n",PKG.l_sid[14],PKG.l_sid[15],PKG.l_sid[16],PKG.l_sid[17]);
			char test_sid[15];
			memset(test_sid,0,sizeof(test_sid));
			for (int i = 0; i < 14; ++i)
			test_sid[i]=PKG.l_sid[i];
			test_sid[14]='\0';
			printf("%s\n",test_sid);
			cout<<"1111"<<endl;
			*/
			memcpy((char *)PKG.nid,"RM1",3); 
			memcpy((char *)(PKG.nid+8),"TAG",3); //源NID ，直接进行封装

			uint8_t content_Ch[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.content_Ch[iii]=content_Ch[iii];

			uint8_t publickey[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.publickey[iii]=publickey[iii];

			memset(PKG.PID,0,sizeof(PKG.PID));

			//****************向RM发送请求TBG的get包**************
			memcpy((unsigned char *)packet->data(),&PKG,126);
			//修改数据包的长度,只发出get包的大小,122为整个get包的长度
			uint32_t takelength;
			uint32_t addlength;
			if(Recvlength<126)//可能不会出现比122还小的ip包
			{
				addlength=126-Recvlength;
				packet->put(addlength);
			}
			else
			{
				takelength=Recvlength-126;
				packet->take(takelength);
			}
			output(0).push(packet);
			cout<<"--->>> REQUST_TBG packet sent "<<endl;
			add_TBG=0;
		}
		else//表示最长前缀匹配有结果，则可以添加TBG
		{
			cout<<"--->>> Add TBG_info into data packet "<<endl;
			add_TBG=1;
		}
	}
}
//建立TUNNEL_LIST和Dst_IP表
void RECV_TBG::push(int port, Packet *packet)
{
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_DATA* pPKG_DATA;//为什么要用结构体指针，利用结构体指针来取数据包的每一个字段
	pPKG_DATA=(CoLoR_NEW_DATA*)RecvBuf;
	char src_nid[3]; //为了取出”RM“
	memcpy(src_nid,(char *)pPKG_DATA->nid+8,2);src_nid[2]='\0';
	char test_sid[15];//为了取出“tunnel_service”
	for (int i = 0; i < 14; ++i) test_sid[i]=pPKG_DATA->l_sid[i]; test_sid[14]='\0';
	//*****************************为RM启动包更新**************************
	if (!strcmp((char *)TUNNEL_SID_intag,test_sid)/*为隧道包*/&&!strcmp(src_nid,"RM")/*为TBG回送包*/)
	{
		cout<<endl<<"--->>> Module: void RECV_TBG::push(int port, Packet *packet) "<<endl;
		cout<<"--->>> RECV_TBG info from RM "<<endl;
		IPAddress DST_NET;
		uint8_t TBG_NID[17];
		memcpy(&DST_NET, pPKG_DATA->l_sid+14,4);
		memcpy((char *)TBG_NID,(char *)pPKG_DATA->n_sid,16);TBG_NID[16]='\0';
		int position;
		int state = 1;
		position=(int)pPKG_DATA->reserved[0];
		cout<<"--->>> RECV_TBG info :  DST_NET= "<<DST_NET.unparse().c_str()<<endl;//点分十进制打印
		cout<<"--->>> RECV_TBG info :  TBG_NID = "<<TBG_NID<<endl;
		cout<<"--->>> RECV_TBG info :  position = "<<position<<endl;
		//******************建立或修改tunnel_list中的对应tunnel_entry***************************
		set_tunnel_list(DST_NET,TBG_NID,position,state);
		//*********************************************
	}
}
//封装与解封装模块移到最后了

//************************PID更新模块************************************************
//新的PID列表，加上TBG标识
int T_NUMBER=0; //PID列表条目编号
struct _pidlist_info
{
	int pidlist_entry;//PID列表条目编号
	int tunnel_num; //隧道编号
	uint8_t  TBG_NID[16]; //TBG的NID
	int pid_num;//PID的数量
	uint8_t PID[4*5];// 到达该TAG的PID
	uint16_t TTL;
	uint16_t Frequence;	
	int tunnel_state;//隧道的开关状态	
	int start_update;//标记是否开启了PID更新
	int update_state;//记录该条目更新状态，由S_timer控制，发出更新包时，置为0,收到更新包时置为1;
	struct _pidlist_info *next;	
};
struct _pidlist_info *PIDLIST; //此时的PIDLIST只是一个空指针，未指向任何地址
struct _pidlist_info  pidlist_data;//用来存储包中的数据
struct _pidlist_info  process_data;//处理模块中用来存储包中的数据

//插入新的PID表结点
struct _pidlist_info* insert_pidlist(struct _pidlist_info *head, struct _pidlist_info *pnew)
{
	struct _pidlist_info *p;
	p=head;
	if (head == NULL)
	{
		pnew->pidlist_entry =1;
		head = pnew;
		pnew->next = NULL;
		return head;
	}
	while(p->next != NULL)
	{
		p=p->next;
	}
	pnew->pidlist_entry = p->pidlist_entry+1;
	p->next=pnew;	
	return head;
}
//打印当前所有的PID列表
void print_pidlist_manage(struct _pidlist_info* head)
{
	struct _pidlist_info* p;
	p=head;
	int i =1;
	while(p!=NULL)
	{
		if (i ==1)
		{
			printf("-------------------------------------------------PID TABLE--------------------------------------------------------\n");
			printf("|pidlist_entry | tunnel_id | TBG_NID | pid_num |  PIDs  | TTL   |  F  | tunnel_state | start_update| update_state|\n" );
			printf("------------------------------------------------------------------------------------------------------------------\n");
			i=0;
		}
		printf("|       %d      |    %d      |   %s  |    %d    |  %s  |  %d   |  %d  |      %d       |      %d      |      %d      |\n",p->pidlist_entry,p->tunnel_num,p->TBG_NID,p->pid_num,p->PID,p->TTL,p->Frequence,p->tunnel_state,p->start_update,p->update_state );
		p=p->next;
	}
	printf("------------------------------------------------------------------------------------------------------------------\n");
}
//打印指定结点的PID列表
void print_pidlist_single(struct _pidlist_info* p)
{
	int i =1;
	if (i ==1)
	{
		printf("-------------------------------------------------PID TABLE--------------------------------------------------------\n");
		printf("|pidlist_entry | tunnel_id | TBG_NID | pid_num |  PIDs  | TTL   |  F  | tunnel_state | start_update| update_state|\n" );
		printf("------------------------------------------------------------------------------------------------------------------\n");
		i=0;
	}
	printf("|       %d      |    %d      |   %s  |    %d    |  %s  |  %d   |  %d  |      %d       |      %d      |      %d      |\n",p->pidlist_entry,p->tunnel_num,p->TBG_NID,p->pid_num,p->PID,p->TTL,p->Frequence,p->tunnel_state,p->start_update,p->update_state );
	printf("------------------------------------------------------------------------------------------------------------------\n");	
}
//设置新的PID列表参数并插入和打印
void set_pidlist(int tunnel_num , uint8_t nid[16], int pid_num, uint8_t PID[4*5], uint16_t TTL, uint16_t F, int tunnel_state )
{
	struct _pidlist_info *pidlist;
	pidlist=(struct _pidlist_info *)malloc(sizeof(struct _pidlist_info));
	pidlist->pidlist_entry = ++T_NUMBER; //没新建一条条目，则条目编号+1
	printf("--->>> insert a new [ pidlist_entry %d ] \n",pidlist->pidlist_entry  );
	pidlist->tunnel_num = tunnel_num;
	memcpy( pidlist->TBG_NID,nid,16);
	pidlist->pid_num = pid_num;
	memcpy( pidlist->PID,PID,4*5);
	pidlist->TTL = TTL;
	pidlist->Frequence = F;
	pidlist->start_update =0; //插入条目的时候，更新没有开启
	pidlist->tunnel_state =tunnel_state;
	pidlist->next = NULL;
	pidlist->update_state = 1;//插入PID列表时，设更新状态标志位为1;表明PID列表的可用状态
	PIDLIST=insert_pidlist(PIDLIST, pidlist);
	print_pidlist_manage(PIDLIST);
}	
/*可以能根据TBG来查找PID列表结点，虽然不同隧道的隧道可能适配到同一TBG，也就是说会出现
除了隧道标号和pidlist条目编号外其他内容完全相同的多个条目。PID的更新过程是基于端点的，因此
这些条目统一进行更新即可。
总结为：PID列表的建立依据于隧道列表，PID列表的更新依据于条目的TBG
*/
//查找对应TBG的PID列表结点，返回找到的表结点指针
struct _pidlist_info* find_pidlist_nid(struct _pidlist_info *head, uint8_t nid[16])
{
	printf("--->>> Into Function [find_pidlist_nid()]\n");
	printf("--->>> find_TBG_NID= %s\n", nid);
	struct _pidlist_info *p;
	p=head;
	while(p->next != NULL )
	{
		if (strcmp((char*)p->TBG_NID, (char*)nid) ==0)//匹配表头和中间结点
		{
			printf("--->>> Find pidlist_entry!\n");
			//print_pidlist_single(p);
			return p;
		}
		p=p->next;
	}
	if (strcmp((char*)p->TBG_NID, (char*)nid) ==0)//匹配最后一个结点
	{
		printf("--->>> Find pidlist_entry!\n");
		//print_pidlist_single(p);
		return p;
	}
}
struct _pidlist_info* find_pidlist_tunnel_id(struct _pidlist_info *head, int tunnel_num)
{
	printf("--->>> Into Function [find_pidlist_tunnel_id()]\n");
	//printf("--->>> launch_tunnel_id = %d\n", launch_tunnel_id);
	struct _pidlist_info *p;
	p=head;
	while(p->next != NULL )
	{
		if (p->tunnel_num == tunnel_num)//匹配表头和中间结点
		{
			printf("--->>> Find pidlist_entry!\n");
			//print_pidlist_single(p);
			return p;
		}
		p=p->next;
	}
	if (p->tunnel_num == tunnel_num)//匹配最后一个结点
	{
		printf("--->>> Find pidlist_entry!\n");
		//print_pidlist_single(p);
		return p;
	}
	else
	{
		cout<<"--->>> Cannot find pidlist_entry!"<<endl;
		return NULL;
	}
}	
//***************************负责处理RM的TBG_info包，建立PID列表************************************
void Updatepid_TAG_Creat::push(int port, Packet *packet)
{	
	//***********初始化定时器*************************
	if(timer_unlaunch){
		memset(&timer_manage, 0, sizeof(struct _timer_manage));
		init_mul_timer(&timer_manage);//初始化定时器
		timer_unlaunch = 0;
		//打印定时器类型值表：
		cout<<"***************TIMER-TYPE****************"<<endl;
		cout<<"* TYPE\t\t\t\tVALUE\t*"<<endl;
		cout<<"* TTL_TIMER_TYPE\t\t1\t*"<<endl;
		cout<<"* F_TIMER_TYPE\t\t\t2\t*"<<endl;
		cout<<"* PID_STATE_TIMER_TYPE\t\t3\t*"<<endl;
		cout<<"* OFF_TIMER_TYPE\t\t4\t*"<<endl;
		cout<<"* KEEPALIVE_TIMER_TYPE\t\t5\t*"<<endl;
		cout<<"* KEEPALIVE_STATE_TIMER_TYPE\t6\t*"<<endl;
		cout<<"* UT_TIMER_TYPE\t\t\t7\t*"<<endl;
		cout<<"*****************************************"<<endl;
	}

	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_DATA* pPKG_DATA;
	pPKG_DATA=(CoLoR_NEW_DATA*)RecvBuf;
	//因为data包的PID字段会被剥掉，所以收到的data包没有PID，PID信息存在data字段中
	memcpy(pPKG_DATA->PID,pPKG_DATA->data,sizeof(pPKG_DATA->PID));
	char src_nid[3];//为了取出”RM“
	memcpy(src_nid,(char *)pPKG_DATA->nid+8,2); src_nid[2]='\0';
	char test_sid[15];//为了取出“tunnel_service”
	for (int i = 0; i < 14; ++i) test_sid[i]=pPKG_DATA->l_sid[i]; test_sid[14]='\0';
	char test_sid1[10];//为了取出”pidupdate“
	for (int i = 0; i < 9; ++i) test_sid1[i]=pPKG_DATA->l_sid[i]; test_sid1[9]='\0';
	//******************为RM回送的TBG_info包更新(data)**************************
	if (!strcmp(TUNNEL_SID,test_sid)/*为隧道包*/&&!strcmp(src_nid,"RM")/*为TBG回送包*/)
	{	
		cout<<endl<<"--->>> Module: void Updatepid_TAG_Creat::push(int port, Packet *packet) "<<endl;
		//获取对应的tunnel_list信息
		IPAddress DST_NET;
		uint8_t TBG_NID[17];
		memcpy(&DST_NET, pPKG_DATA->l_sid+14,4);
		struct _tunnel_list_info *p_tunnel;
		int position,tunnel_id,state;
		while((p_tunnel=match_dst_net(TUNNEL_LIST,DST_NET) ) ==NULL)
		{
			 cout<<"--->>> Function [match_dst_net] return NUll. Tunnle_list is Null! Please wait "<<endl;
			sleep(5);//表空时暂停5s再查询
		}
		position = p_tunnel->position;
		tunnel_id = p_tunnel->tunnel_id;
		state=p_tunnel->state;
		cout<<"--->>> p_tunnel->tunnel_id ="<<p_tunnel->tunnel_id<<"     p_tunnel->position ="<<p_tunnel->position<<"     p_tunnel->state ="<<p_tunnel->state<<endl;
		//收到的包信息
		int n;int c[8];
		n=pPKG_DATA->pids_o;
		c[0]=n/128;n%=128;
		c[1]=n/64;n%=64;
		c[2]=n/32;n%=32;
		c[3]=n/16;n%=16;
		c[4]=n/8;n%=8;
		c[5]=n/4;n%=4;
		c[6]=n/2;n%=2;
		c[7]=n;//for(iii=0;iii<=7;iii++) cout<<c[iii];
		//填充待插入的PID列表数据
		///*PID列表编号*/pidlist_data.pidlist_entry =++T_NUMBER; 在set函数中添加
		/*隧道编号*/pidlist_data.tunnel_num=tunnel_id;
		/*边界网关*/memcpy(pidlist_data.TBG_NID,pPKG_DATA->n_sid,16);
		/*pid的数量*/pidlist_data.pid_num=c[0]*16+c[1]*8+c[2]*4+c[3]*2+c[4]*1; //data包的PID数量共有5位
		/*PIDs*/memcpy(pidlist_data.PID,pPKG_DATA->PID,sizeof(pPKG_DATA->PID));
		/*TTL*/pidlist_data.TTL=2*pPKG_DATA->minpid;
		/*Frequence*/pidlist_data.Frequence=pPKG_DATA->minpid-2;
		/*start_update参数： 在set_pidlsit()函数中统一设置为0*/
		/*tunnel_state*/pidlist_data.tunnel_state = state; //从tunnel_list中提取的隧道状态标志
		
		//打印收到的更新信息及PID列表的情况 //换成打印隧道列表该条目的信息更好
		cout<<"--->>> Recv TBG_info packet from RM, packet data :"<<endl;
		cout<<"     |  pPKG_DATA->pids_o = ";
		for(int iii=0;iii<=6;iii++) cout<<c[iii];
		cout<<endl<<"     |  pPKG_DATA->NID = "<<pPKG_DATA->n_sid<<endl;
		cout<<"     |  pPKG_DATA->PID = "<<pPKG_DATA->PID<<endl;
		cout<<"     |  pPKG_DATA->minpid = "<<pPKG_DATA->minpid<<endl;
		//判断PID列表是否为空，并且判断该条隧道的position是否为0;
		if (PIDLIST==NULL ) //PIDLIST表空且该隧道异域时，开启PID更新过程
		{
			if (position == 0) // TBG异域
			{
				//插入新的PID列表
				set_pidlist(pidlist_data.tunnel_num,pidlist_data.TBG_NID,pidlist_data.pid_num,pidlist_data.PID,pidlist_data.TTL,pidlist_data.Frequence,pidlist_data.tunnel_state);
				//计时器参数
				TTL=pidlist_data.TTL;
				//插入新的TTL计时器
				char tstr[200];
				get_format_time(tstr);//获取当前时间
				printf("\n--->>> %s: For [ PIDLSIT %d ] insert a new TTL_TIMER \n", tstr,T_NUMBER);
				struct _timer_info* timer_info1;
				uint16_t time1 = TTL;//插入新的TTL定时器
				if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
				set_child_timer(time1, ++TIMER_ID, TTL_TIMER_TYPE, tunnel_id);//插入子定时器
				print_timer_manage(timer_manage.timer_info);//打印定时时间表
				TTL_TIMER_launch=0;
			}	
		}
		//表不空，则建立新条目或者修改已有对应条目（根据TBG_NID查找是否有对应的PID列表条目）
		else
		{	
			//依据隧道编号，查找对应的pidlist_entry条目
			struct _pidlist_info *p_find;
			p_find = find_pidlist_tunnel_id(PIDLIST,p_tunnel->tunnel_id); // 根据隧道编号查找PID列表
			if (p_find == NULL) //没有找到，需要新建条目
			{
				//插入新的PID列表
				set_pidlist(pidlist_data.tunnel_num,pidlist_data.TBG_NID,pidlist_data.pid_num,pidlist_data.PID,pidlist_data.TTL,pidlist_data.Frequence,pidlist_data.tunnel_state);
				//计时器参数
				TTL=pidlist_data.TTL;
				//插入新的TTL计时器
				char tstr[200];
				get_format_time(tstr);//获取当前时间
				printf("\n--->>> %s: For [ PIDLSIT %d ] insert a new TTL_TIMER \n", tstr,T_NUMBER);
				struct _timer_info* timer_info1;
				uint16_t time1 = TTL;//插入新的TTL定时器
				if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
				set_child_timer(time1, ++TIMER_ID, TTL_TIMER_TYPE, p_tunnel->tunnel_id);//插入子定时器
				print_timer_manage(timer_manage.timer_info);//打印定时时间表
				TTL_TIMER_launch=0;
			}
			else //找到对应条目，则根据PID列表的更新规则进行更新
			{
				//更新条件：pid列表条目中任一项发生变化都需要进行更新
				if ((strcmp((char*)p_find->TBG_NID,(char*)pidlist_data.TBG_NID)!=0/*TBG_NID发生变化*/)||(strcmp((char*)p_find->PID,(char*)pidlist_data.PID)!=0/*PID发生变化*/)||(p_find->pid_num != pidlist_data.pid_num)/*PID数量发生表化*/||(p_find->Frequence != pidlist_data.Frequence)/*RM中PID最小更新周期*/)
				{
					printf("--->>> Update [pidlist_entry %d]\n", p_find->pidlist_entry );
					//更新方法：在原条目上进行修改，用新数据代替旧数据
					memcpy( p_find->TBG_NID,pidlist_data.TBG_NID,16);
					p_find->pid_num = pidlist_data.pid_num;
					memcpy( p_find->PID,pidlist_data.PID,4*5);
					p_find->TTL = pidlist_data.TTL;
					p_find->Frequence = pidlist_data.Frequence;
				}
				else
					printf("--->>> [pidlist_entry %d] keeps no change\n", p_find->pidlist_entry );
			}
		}
	}
	//**********************对方的紧急更新包*************************************
	else if (!strcmp(UPDATE_SID,test_sid1)) //为紧急更新包
	{
		struct _pidlist_info *p_find;
		int tunnel_num = pPKG_DATA->reserved[1];
		p_find=find_pidlist_tunnel_id(PIDLIST, tunnel_num);
		printf("--->>> Receive TBG Emergency Update get packet for [pidlist_entry %d---tunnel_id %d] \n", p_find->pidlist_entry ,p_find->tunnel_num);
		//立即发送更新包
		//********************封装get包**************************
		char RecvBuf_GET[1024];
		memcpy(RecvBuf_GET,packet->data(),packet->length());
		struct CoLoR_NEW_GET PKG;
		memset(&PKG,0,sizeof(CoLoR_NEW_GET));
		if (p_find->tunnel_state == 0)//隧道关闭
		{
			printf("--->>> [ tunnel %d ]  has been shut down \n",p_find ->tunnel_num);
			//用SID通知对方隧道已经关闭
			uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e','d','o','w','n',0,0,0,0,0,0,0};
			for(int iii=0;iii<20;iii++)
			PKG.l_sid[iii]=l_sid[iii];
		}
		else//隧道依然开启
		{
			//正常的SID	
			uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
			for(int iii=0;iii<20;iii++)
			PKG.l_sid[iii]=l_sid[iii];
		}
		//封装mac层头部
		PKG.ether_dhost[0] = 0xBB;//目的地址
		PKG.ether_dhost[1] = 0xBB;
		PKG.ether_dhost[2] = 0xBB;
		PKG.ether_dhost[3] = 0xBB;
		PKG.ether_dhost[4] = 0xBB;
		PKG.ether_dhost[5] = 0xBB;
		PKG.ether_shost[0] = 0xAA;//源地址
		PKG.ether_shost[1] = 0xAA;
		PKG.ether_shost[2] = 0xAA;
		PKG.ether_shost[3] = 0xAA;
		PKG.ether_shost[4] = 0xAA;
		PKG.ether_shost[5] = 0xAA;
		PKG.ether_type = 0x0008;

		PKG.version_type=160;//10100000
		PKG.ttl=255;
		PKG.total_len=126;
		//cout<<PKG.total_len<<endl;

		PKG.port_no1=1;
		PKG.port_no2=2;

		//由RM填充
		PKG.minpid=0;
		PKG.pids_o=0;//pid数量占7位，offset字段占一位
		PKG.res= Get_NUM++; //填充更新包编号
		printf("--->>> Send Get_NUM=%d  --  ", (int*)PKG.res);

		int iii;
		uint8_t offset[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.offset[iii]=offset[iii];

		uint8_t length[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.length[iii]=length[iii];

		PKG.content_len=1000;
		PKG.mtu=1500;

		PKG.publickey_len=256;
		PKG.checksum=123;

		//填写隧道编号
		memset(PKG.reserved,0,sizeof(PKG.reserved));
		PKG.reserved[1]=(uint8_t)p_find->tunnel_num;

		memcpy((char *)PKG.n_sid,p_find->TBG_NID,16);//从PID列表中提取

		memcpy((char *)PKG.nid,p_find->TBG_NID,8); //目的NID, 从PID列表中提取
		memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID ，直接进行封装

		uint8_t content_Ch[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.content_Ch[iii]=content_Ch[iii];

		uint8_t publickey[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.publickey[iii]=publickey[iii];

		memset(PKG.PID,0,sizeof(PKG.PID));

		//****************************发送get包*********************************
		memcpy((unsigned char *)packet->data(),&PKG,126);
		output(0).push(packet);
		printf("--->>> [ pidlist_entry %d] Emergency PID update packet sent out!\n",p_find->pidlist_entry);
	}	
	else
	{
		output(1).push(packet);//否则丢弃
	}	
}
//***************************发送模块************************************
//负责发送更新包
void Updatepid_TAG_Send::push(int port, Packet *packet)
{
	//***********初始化定时器*************************
	if(timer_unlaunch){
		memset(&timer_manage, 0, sizeof(struct _timer_manage));
		init_mul_timer(&timer_manage);//初始化定时器
		timer_unlaunch = 0;
		//打印定时器类型值表：
		cout<<"***************TIMER-TYPE****************"<<endl;
		cout<<"* TYPE\t\t\t\tVALUE\t*"<<endl;
		cout<<"* TTL_TIMER_TYPE\t\t1\t*"<<endl;
		cout<<"* F_TIMER_TYPE\t\t\t2\t*"<<endl;
		cout<<"* PID_STATE_TIMER_TYPE\t\t3\t*"<<endl;
		cout<<"* OFF_TIMER_TYPE\t\t4\t*"<<endl;
		cout<<"* KEEPALIVE_TIMER_TYPE\t\t5\t*"<<endl;
		cout<<"* KEEPALIVE_STATE_TIMER_TYPE\t6\t*"<<endl;
		cout<<"* UT_TIMER_TYPE\t\t\t7\t*"<<endl;
		cout<<"*****************************************"<<endl;
	}
	//******************修改包长度***********************
	int pkglength;                       
	pkglength = packet->length();
	uint32_t addlength;
	addlength=126-pkglength;
	packet->put(addlength);
	//************更新的首要条件：如果PID列表不为空****************
	if (PIDLIST  != NULL)
	{
		//PID列表中的所有隧道开启状态的条目开启PID更p-新过程
		struct _pidlist_info *p;
		p=PIDLIST;//从头结点开始遍历
		while(p->next != NULL )
		{
			//cout<<"while"<<endl;
			if (p->tunnel_state ==1)//隧道开启状态
			{
				if (p->start_update ==0)//更新尚未开启
				{
					printf("\n--->>> Module: void Updatepid_TAG_Send::push(int port, Packet *packet) : Start PID update process for [ pidlist_entry %d] !\n",p->pidlist_entry);
					//发送首个更新包，并且插入F、S定时器，开启更新过程
					//********************封装get包**************************
					char RecvBuf_GET[1024];
					memcpy(RecvBuf_GET,packet->data(),packet->length());
					struct CoLoR_NEW_GET PKG;
					memset(&PKG,0,sizeof(CoLoR_NEW_GET));
					//封装mac层头部
					PKG.ether_dhost[0] = 0xBB;//目的地址
					PKG.ether_dhost[1] = 0xBB;
					PKG.ether_dhost[2] = 0xBB;
					PKG.ether_dhost[3] = 0xBB;
					PKG.ether_dhost[4] = 0xBB;
					PKG.ether_dhost[5] = 0xBB;
					PKG.ether_shost[0] = 0xAA;//源地址
					PKG.ether_shost[1] = 0xAA;
					PKG.ether_shost[2] = 0xAA;
					PKG.ether_shost[3] = 0xAA;
					PKG.ether_shost[4] = 0xAA;
					PKG.ether_shost[5] = 0xAA;
					PKG.ether_type = 0x0008;

					PKG.version_type=160;//10100000
					PKG.ttl=255;
					PKG.total_len=126;

					PKG.port_no1=1;
					PKG.port_no2=2;

					PKG.minpid=0;//由RM填充
					PKG.pids_o=0;//由RM填充
					PKG.res= Get_NUM++; //填充更新包编号
					printf("--->>> Send Get_NUM=%d  --  ", (int*)PKG.res);

					int iii;
					uint8_t offset[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.offset[iii]=offset[iii];

					uint8_t length[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.length[iii]=length[iii];

					PKG.content_len=1000;
					PKG.mtu=1500;

					PKG.publickey_len=256;
					PKG.checksum=123;

					//填写隧道编号
					memset(PKG.reserved,0,sizeof(PKG.reserved));
					PKG.reserved[1]=(uint8_t)p->tunnel_num;

					memcpy((char *)PKG.n_sid,p->TBG_NID,16);//从PID列表中提取

					uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
					for(iii=0;iii<20;iii++)
					PKG.l_sid[iii]=l_sid[iii];

					memcpy((char *)PKG.nid,p->TBG_NID,8); //目的NID, 从PID列表中提取
					memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID ，直接进行封装

					uint8_t content_Ch[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.content_Ch[iii]=content_Ch[iii];

					uint8_t publickey[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.publickey[iii]=publickey[iii];

					memset(PKG.PID,0,sizeof(PKG.PID));

					//****************************发送get包*********************************
					memcpy((unsigned char *)packet->data(),&PKG,126);
					output(0).push(packet);
					p->update_state=0;
					cout<<"--->>> PIDUPDATE_STATE ="<<p->update_state<<endl;
					printf("--->>> [ pidlist_entry %d] PID update packet sent out!\n",p->pidlist_entry);
					p->start_update = 1;//标志该隧道的PID更新过程已经开启
					
					//插入F、S计时器
					char tstr[200];
					get_format_time(tstr);//获取当前时间
					printf("\n--->>> %s: insert F_TIMER timer and S_TIMER timer  \n", tstr);
					struct _timer_info* timer_info1;
					struct _timer_info* timer_info2;
					uint16_t time1 = Frequence;//发包频率为Frequence1=pidlist1.Frequence=pPKG_DATA->minpid-2;
					uint16_t time2 = S_TIME;
					if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
					set_child_timer(time1, ++TIMER_ID, F_TIMER_TYPE, p->tunnel_num);//插入子定时器
					set_child_timer(time2, ++TIMER_ID, PID_STATE_TIMER_TYPE,p->tunnel_num);//插入子定时器
					print_timer_manage(timer_manage.timer_info);//打印定时时间表
					F_TIMER_launch = 0;
					S_TIMER_launch = 0;
				}
			}
			p=p->next;
		}
		//开启更新：为表头或表尾结点
		if (p->tunnel_state ==1)//多个结点时，除最后一个结点都能遍历到
		{
			//**********************发包类型一：开启更新*************************
			if (p->start_update ==0)//PID更新尚未开启
			{
				printf("\n--->>> Module: void Updatepid_TAG_Send::push(int port, Packet *packet) : Start PID update process for [ pidlist_entry %d] !\n",p->pidlist_entry);
				//发送首个更新包，并且插入F、S定时器，开启更新过程
				//********************封装get包**************************
				char RecvBuf_GET[1024];
				memcpy(RecvBuf_GET,packet->data(),packet->length());
				struct CoLoR_NEW_GET PKG;
				memset(&PKG,0,sizeof(CoLoR_NEW_GET));
				//封装mac层头部
				PKG.ether_dhost[0] = 0xBB;//目的地址
				PKG.ether_dhost[1] = 0xBB;
				PKG.ether_dhost[2] = 0xBB;
				PKG.ether_dhost[3] = 0xBB;
				PKG.ether_dhost[4] = 0xBB;
				PKG.ether_dhost[5] = 0xBB;
				PKG.ether_shost[0] = 0xAA;//源地址
				PKG.ether_shost[1] = 0xAA;
				PKG.ether_shost[2] = 0xAA;
				PKG.ether_shost[3] = 0xAA;
				PKG.ether_shost[4] = 0xAA;
				PKG.ether_shost[5] = 0xAA;
				PKG.ether_type = 0x0008;

				PKG.version_type=160;//10100000
				PKG.ttl=255;
				PKG.total_len=126;

				PKG.port_no1=1;
				PKG.port_no2=2;

				PKG.minpid=0; //由RM填充PKG.minpid=0; //由RM填充
				PKG.pids_o=0;//由RM填充，pid数量占7位，offset字段占一位
				PKG.res= Get_NUM++; //填充更新包编号
				printf("--->>> Send Get_NUM=%d  --  ", (int*)PKG.res);

				int iii;
				uint8_t offset[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.offset[iii]=offset[iii];

				uint8_t length[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.length[iii]=length[iii];

				PKG.content_len=1000;
				PKG.mtu=1500;

				PKG.publickey_len=256;
				PKG.checksum=123;

				//填写隧道编号
				memset(PKG.reserved,0,sizeof(PKG.reserved));
				PKG.reserved[1]=(uint8_t)p->tunnel_num;

				memcpy((char *)PKG.n_sid,p->TBG_NID,16);//从PID列表中提取

				uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
				for(iii=0;iii<20;iii++)
				PKG.l_sid[iii]=l_sid[iii];

				memcpy((char *)PKG.nid,p->TBG_NID,8); //目的NID, 从PID列表中提取
				memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID ，直接进行封装

				uint8_t content_Ch[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.content_Ch[iii]=content_Ch[iii];

				uint8_t publickey[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.publickey[iii]=publickey[iii];

				memset(PKG.PID,0,sizeof(PKG.PID));

				//****************************发送get包*********************************
				memcpy((unsigned char *)packet->data(),&PKG,126);	
				output(0).push(packet);
				p->update_state =0;
				cout<<"--->>> PIDUPDATE_STATE ="<<p->update_state<<endl;
				printf("--->>> [ pidlist_entry %d] PID update packet sent out!\n",p->pidlist_entry);
				p->start_update = 1;//标志该隧道的PID更新过程已经开启

				//插入F、S计时器
				char tstr[200];
				get_format_time(tstr);//获取当前时间
				printf("\n--->>> %s: insert F_TIMER timer and S_TIMER timer  \n", tstr);
				struct _timer_info* timer_info1;
				struct _timer_info* timer_info2;
				uint16_t time1 = Frequence;//发包频率为Frequence1=pidlist1.Frequence=pPKG_DATA->minpid-2;
				uint16_t time2 = S_TIME;
				if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
				set_child_timer(time1, ++TIMER_ID, F_TIMER_TYPE, p->tunnel_num);//插入子定时器
				set_child_timer(time2, ++TIMER_ID, PID_STATE_TIMER_TYPE,p->tunnel_num);//插入子定时器
				print_timer_manage(timer_manage.timer_info);//打印定时时间表
				S_TIMER_launch =0;
				F_TIMER_launch = 0;
			}
		}
		//******************发包类型二：周期更新*******************
		if( F_TIMER_launch == 1 )//发送周期更新包
		{
			//找到计时器到时的隧道编号！！！！！
			printf("\n--->>> Module: void Updatepid_TAG_Send::push(int port, Packet *packet) : F_TIMER launched for [ tunnel %d ]! \n", launch_tunnel_id );
			//根据隧道编号查找PID列表
			struct _pidlist_info*  p_pidlist;
			p_pidlist = find_pidlist_tunnel_id(PIDLIST, launch_tunnel_id);
			if (p_pidlist->tunnel_state == 0)//隧道关闭
			{
				printf("--->>> [ tunnel %d ]  has been shut down \n",p_pidlist ->tunnel_num);
			}
			else //隧道依然开启
			{
				//********************封装get包**************************
				char RecvBuf_GET[1024];
				memcpy(RecvBuf_GET,packet->data(),packet->length());
				struct CoLoR_NEW_GET PKG;
				memset(&PKG,0,sizeof(CoLoR_NEW_GET));
				
				//封装mac层头部
				PKG.ether_dhost[0] = 0xBB;//目的地址
				PKG.ether_dhost[1] = 0xBB;
				PKG.ether_dhost[2] = 0xBB;
				PKG.ether_dhost[3] = 0xBB;
				PKG.ether_dhost[4] = 0xBB;
				PKG.ether_dhost[5] = 0xBB;
				PKG.ether_shost[0] = 0xAA;//源地址
				PKG.ether_shost[1] = 0xAA;
				PKG.ether_shost[2] = 0xAA;
				PKG.ether_shost[3] = 0xAA;
				PKG.ether_shost[4] = 0xAA;
				PKG.ether_shost[5] = 0xAA;
				PKG.ether_type = 0x0008;	

				PKG.version_type=160;//10100000
				PKG.ttl=255;
				PKG.total_len=126;
				//cout<<PKG.total_len<<endl;

				PKG.port_no1=1;
				PKG.port_no2=2;

				//GET包中由RM填充
				PKG.minpid=0; //由RM填充
				PKG.pids_o=0;//pid数量占7位，offset字段占一位
				PKG.res= Get_NUM++; //填充更新包编号
				printf("--->>> Send Get_NUM=%d  --  ", (int*)PKG.res);

				int iii;
				uint8_t offset[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.offset[iii]=offset[iii];

				uint8_t length[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.length[iii]=length[iii];

				PKG.content_len=1000;
				PKG.mtu=1500;

				PKG.publickey_len=256;
				PKG.checksum=123;

				//填写隧道编号
				memset(PKG.reserved,0,sizeof(PKG.reserved));
				PKG.reserved[1]=(uint8_t)launch_tunnel_id;

				//填充PID列表中的NID
				memcpy((char *)PKG.n_sid,p_pidlist->TBG_NID,16);//从PID列表中提取
				memcpy((char *)PKG.nid,p_pidlist->TBG_NID,8); //目的NID, 从PID列表中提取
				
				uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
				for(iii=0;iii<20;iii++)
				PKG.l_sid[iii]=l_sid[iii];

				memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID ，直接进行封装

				uint8_t content_Ch[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.content_Ch[iii]=content_Ch[iii];

				uint8_t publickey[4]={255,255,255,255};
				for(iii=0;iii<4;iii++)
				PKG.publickey[iii]=publickey[iii];

				memset(PKG.PID,0,sizeof(PKG.PID));

				//****************************发送get包*********************************
				memcpy((unsigned char *)packet->data(),&PKG,sizeof(CoLoR_NEW_GET));	
				output(0).push(packet);
				p_pidlist->update_state=0;
				cout<<"--->>> PIDUPDATE_STATE ="<<p_pidlist->update_state<<endl;			
				printf("--->>> [ pidlist_entry %d] PID update packet sent out!\n",p_pidlist->pidlist_entry);
			}
				
		}
		//******************发包类型三：状态周期发包(GET)*******************
		else if (S_TIMER_launch == 1 )//状态周期计时器到时，F计时器没有到时，并且该隧道的更新状态位为0;
		{
			printf("\n--->>> Module: void Updatepid_TAG_Send::push(int port, Packet *packet) : S_TIMER launched for [ tunnel %d ]! \n", launch_tunnel_id );
			if (F_TIMER_launch == 1)
			{
				printf("--->>> F_TIMER launched at the same time !   -- Normal update send out！\n");
			}
			else
			{
				//找到计时器到时的隧道编号！！！！
				//根据隧道编号查找PID列表
				struct _pidlist_info*  p_pidlist;
				p_pidlist = find_pidlist_tunnel_id(PIDLIST, launch_tunnel_id);
				if (p_pidlist->tunnel_state == 0)//隧道关闭
				{
					printf("--->>> [ tunnel %d ]  has been shut down \n",p_pidlist ->tunnel_num);
				}
				else if(p_pidlist->update_state == 0)//隧道依然开启并且状态标志位为0
				{
					printf("--->>> Find out [ tunnel %d ]  update_state = %d , need re_send update packet\n",p_pidlist ->tunnel_num,p_pidlist->update_state);
					//********************封装get包**************************
					char RecvBuf_GET[1024];
					memcpy(RecvBuf_GET,packet->data(),packet->length());
					struct CoLoR_NEW_GET PKG;
					memset(&PKG,0,sizeof(CoLoR_NEW_GET));
					
					//封装mac层头部
					PKG.ether_dhost[0] = 0xBB;//目的地址
					PKG.ether_dhost[1] = 0xBB;
					PKG.ether_dhost[2] = 0xBB;
					PKG.ether_dhost[3] = 0xBB;
					PKG.ether_dhost[4] = 0xBB;
					PKG.ether_dhost[5] = 0xBB;
					PKG.ether_shost[0] = 0xAA;//源地址
					PKG.ether_shost[1] = 0xAA;
					PKG.ether_shost[2] = 0xAA;
					PKG.ether_shost[3] = 0xAA;
					PKG.ether_shost[4] = 0xAA;
					PKG.ether_shost[5] = 0xAA;
					PKG.ether_type = 0x0008;	

					PKG.version_type=160;//10100000
					PKG.ttl=255;
					PKG.total_len=126;
					//cout<<PKG.total_len<<endl;

					PKG.port_no1=1;
					PKG.port_no2=2;

					//由RM填充
					PKG.minpid=0;
					PKG.pids_o=0;//pid数量占7位，offset字段占一位
					PKG.res= Get_NUM++; //填充更新包编号
					printf("--->>> Send Get_NUM=%d  --  ", (int*)PKG.res);

					int iii;
					uint8_t offset[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.offset[iii]=offset[iii];

					uint8_t length[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.length[iii]=length[iii];

					PKG.content_len=1000;
					PKG.mtu=1500;

					PKG.publickey_len=256;
					PKG.checksum=123;

					//填写隧道编号
					memset(PKG.reserved,0,sizeof(PKG.reserved));
					PKG.reserved[1]=(uint8_t)launch_tunnel_id;

					//填充PID列表中的NID
					memcpy((char *)PKG.n_sid,p_pidlist->TBG_NID,16);//从PID列表中提取
					memcpy((char *)PKG.nid,p_pidlist->TBG_NID,8); //目的NID, 从PID列表中提取
					
					uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
					for(iii=0;iii<20;iii++)
					PKG.l_sid[iii]=l_sid[iii];

					memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID ，直接进行封装

					uint8_t content_Ch[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.content_Ch[iii]=content_Ch[iii];

					uint8_t publickey[4]={255,255,255,255};
					for(iii=0;iii<4;iii++)
					PKG.publickey[iii]=publickey[iii];

					memset(PKG.PID,0,sizeof(PKG.PID));

					//****************************发送get包*********************************
					memcpy((unsigned char *)packet->data(),&PKG,sizeof(CoLoR_NEW_GET));
					output(0).push(packet);
					p_pidlist->update_state=0; //修改状态标志位
					cout<<"--->>> PIDUPDATE_STATE ="<<p_pidlist->update_state<<endl;			
					printf("--->>> [ pidlist_entry %d] PID update packet sent out!\n",p_pidlist->pidlist_entry);
				}
				else
					printf("--->>> Find out [ tunnel %d ]  update_state = %d , Don`t need re_send update packet\n",p_pidlist ->tunnel_num,p_pidlist->update_state);	
			}
		}
		//*******************发包类型四：紧急更新********************
		//条件： TTL计时器到时但状态值S=1,表明PID列表已经更新过，且此时发包频率计时器没有到时、状态定时器没有到时
		else if(TTL_TIMER_launch == 1) // 需要用特殊的方式标志下紧急包，用于插入TLL计时器
		{
			printf("\n--->>> Module: void Updatepid_TAG_Send::push(int port, Packet *packet) : TTL_TIMER launched for [ tunnel %d ]! \n", launch_tunnel_id );
			if ( F_TIMER_launch == 1 )
			{
				printf("--->>> Emergency : PIDLIST Unormal -- F_TIMER launched! Normal update send out！\n");
			}
			else
			{
				//找到计时器到时的隧道编号！！！！！
				//根据隧道编号查找PID列表
				struct _pidlist_info*  p_pidlist;
				p_pidlist = find_pidlist_tunnel_id(PIDLIST, launch_tunnel_id);
				if (p_pidlist->tunnel_state == 0)//隧道关闭
				{
					printf("--->>> [ tunnel %d ]  has been shut down \n",p_pidlist ->tunnel_num);
				}
				//隧道开启状态
				else if(p_pidlist->update_state == 0 )//该隧道没有正常更新，PID列表可能已经不可用,紧急更新
				{
					printf("--->>> [ tunnel %d ] update_state = %d , Unormal！\n",  p_pidlist->tunnel_num, p_pidlist->update_state);
					//进入状态包发送
					if (S_TIMER_launch ==1) 
						printf("--->>> Emergency : PIDLIST Unormal -- S_TIMER launched! STATE update send out！\n");
					//TTL计时器到时，更新状态位为0,在没有状态包或者更新包发出时，才发送紧急更新包
					else 
					{
						//********************封装get包**************************
						char RecvBuf_GET[1024];
						memcpy(RecvBuf_GET,packet->data(),packet->length());
						struct CoLoR_NEW_GET PKG;
						memset(&PKG,0,sizeof(CoLoR_NEW_GET));
						
						//封装mac层头部
						PKG.ether_dhost[0] = 0xBB;//目的地址
						PKG.ether_dhost[1] = 0xBB;
						PKG.ether_dhost[2] = 0xBB;
						PKG.ether_dhost[3] = 0xBB;
						PKG.ether_dhost[4] = 0xBB;
						PKG.ether_dhost[5] = 0xBB;
						PKG.ether_shost[0] = 0xAA;//源地址
						PKG.ether_shost[1] = 0xAA;
						PKG.ether_shost[2] = 0xAA;
						PKG.ether_shost[3] = 0xAA;
						PKG.ether_shost[4] = 0xAA;
						PKG.ether_shost[5] = 0xAA;
						PKG.ether_type = 0x0008;

						PKG.version_type=160;//10100000
						PKG.ttl=255;
						PKG.total_len=126;
						//cout<<PKG.total_len<<endl;

						PKG.port_no1=1;
						PKG.port_no2=2;

						//由RM填充
						PKG.minpid=0;
						PKG.pids_o=0;//pid数量占7位，offset字段占一位
						PKG.res= 0;//紧急更新包的编号设为0
						printf("--->>> Send Get_NUM=%d  --  ", (int*)PKG.res);
						
						int iii;
						uint8_t offset[4]={255,255,255,255};
						for(iii=0;iii<4;iii++)
						PKG.offset[iii]=offset[iii];

						uint8_t length[4]={255,255,255,255};
						for(iii=0;iii<4;iii++)
						PKG.length[iii]=length[iii];

						PKG.content_len=1000;
						PKG.mtu=1500;

						PKG.publickey_len=256;
						PKG.checksum=123;

						//填写隧道编号
						memset(PKG.reserved,0,sizeof(PKG.reserved));
						PKG.reserved[1]=(uint8_t)launch_tunnel_id;

						memcpy((char *)PKG.n_sid,p_pidlist->TBG_NID,16);//从PID列表中提取

						uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
						for(iii=0;iii<20;iii++)
						PKG.l_sid[iii]=l_sid[iii];

						memcpy((char *)PKG.nid,p_pidlist->TBG_NID,8); //目的NID, 从PID列表中提取
						memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID ，直接进行封装

						uint8_t content_Ch[4]={255,255,255,255};
						for(iii=0;iii<4;iii++)
						PKG.content_Ch[iii]=content_Ch[iii];

						uint8_t publickey[4]={255,255,255,255};
						for(iii=0;iii<4;iii++)
						PKG.publickey[iii]=publickey[iii];

						memset(PKG.PID,0,sizeof(PKG.PID));

						//****************************发送get包*********************************
						memcpy((unsigned char *)packet->data(),&PKG,sizeof(CoLoR_NEW_GET));
						output(0).push(packet);
						printf("--->>> Emergency : PID  is unavailable packet sent out!\n",p_pidlist->pidlist_entry);
					}
				}
			}
		}
		//*****************发包类型五：对方紧急更新发包*******************
		//由Updatepid_TAG_Creat模块处理发送，由此可以对TBG的紧急情况立刻作出响应		
		//*************（采用 if 平行检测）进行计时器的检测和插入工作*******************************
		if ( F_TIMER_launch == 1) //F计时器到时要同时插入S计时器
		{
			printf("--->>>  [ tunnel %d ] F_TIMER launched !\n", launch_tunnel_id);
			struct _pidlist_info*  p_pidlist;
			p_pidlist = find_pidlist_tunnel_id(PIDLIST, launch_tunnel_id);
			if (p_pidlist->tunnel_state == 1)//隧道开启
			{
				//插入F、S计时器，S计时器用来追踪F周期发送的包的状态
				char tstr[200];
				get_format_time(tstr);//获取当前时间
				printf("\n--->>> %s: insert F_TIMER timer and S_TIMER timer  \n", tstr);
				struct _timer_info* timer_info1;
				struct _timer_info* timer_info2;
				uint16_t time1 = Frequence;//发包频率为Frequence1=pidlist1.Frequence=pPKG_DATA->minpid-2;
				uint16_t time2 = S_TIME;
				if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
				set_child_timer(time1, ++TIMER_ID, F_TIMER_TYPE, p_pidlist->tunnel_num);//插入子定时器
				set_child_timer(time2, ++TIMER_ID, PID_STATE_TIMER_TYPE,p_pidlist->tunnel_num);//插入子定时器
				print_timer_manage(timer_manage.timer_info);//打印定时时间表
				S_TIMER_launch =0;
				F_TIMER_launch = 0;
			}
		}
		else if (S_TIMER_launch == 1)
		{
			printf("--->>>  [ tunnel %d ] S_TIMER launched !\n", launch_tunnel_id);
			struct _pidlist_info*  p_pidlist;
			p_pidlist = find_pidlist_tunnel_id(PIDLIST, launch_tunnel_id);
			if (p_pidlist->tunnel_state == 1)//隧道开启
			{
				//到时，立刻插入S_TIMER定时器
				char tstr[200];
				get_format_time(tstr);//获取当前时间
				printf("\n--->>> %s: insert S_TIMER timer \n", tstr);
				struct _timer_info* timer_info2;
				uint16_t time2 = S_TIME;
				if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
				set_child_timer(time2, ++TIMER_ID, PID_STATE_TIMER_TYPE,launch_tunnel_id);//插入子定时器
				print_timer_manage(timer_manage.timer_info);//打印定时时间表
				S_TIMER_launch =0;
			}

		}
		if (TTL_TIMER_launch == 1)
		{
			printf("--->>>  [ tunnel %d ] TTL_TIMER launched !\n", launch_tunnel_id);
			struct _pidlist_info*  p_pidlist;
			p_pidlist = find_pidlist_tunnel_id(PIDLIST, launch_tunnel_id);
			if (p_pidlist->tunnel_state == 1)//隧道开启
			{
				//插入条件：隧道开启并且隧道的更新状态正常
				if (p_pidlist->update_state == 1)
				{
					char tstr[200];
					get_format_time(tstr);//获取当前时间
					printf("--->>> [ tunnel %d ] update_state = %d , PIDLSIT is avaliable！\n", p_pidlist->tunnel_num, p_pidlist->update_state);
					printf("\n--->>> %s: For [ PIDLSIT %d ] insert a new TTL_TIMER \n", tstr,p_pidlist->pidlist_entry);
					struct _timer_info* timer_info1;
					uint16_t time1 = p_pidlist->TTL;//插入PID列表中的TTL值定时器
					if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
					set_child_timer(time1, ++TIMER_ID, TTL_TIMER_TYPE, p_pidlist->tunnel_num);//插入子定时器
					print_timer_manage(timer_manage.timer_info);//打印定时时间表
					TTL_TIMER_launch=0;
				}
			}
		}
	}
	else //PID列表为空时，丢弃数据包
	{
		output(1).push(packet);
	}
}
//************************接收处理模块*********************************************
//负责更新PID列表
int packet_num=1;
void Updatepid_TAG_Process::push(int port, Packet *packet)
{
	//***********初始化定时器*************************
	if(timer_unlaunch){
		memset(&timer_manage, 0, sizeof(struct _timer_manage));
		init_mul_timer(&timer_manage);//初始化定时器
		timer_unlaunch = 0;
		//打印定时器类型值表：
		cout<<"***************TIMER-TYPE****************"<<endl;
		cout<<"* TYPE\t\t\t\tVALUE\t*"<<endl;
		cout<<"* TTL_TIMER_TYPE\t\t1\t*"<<endl;
		cout<<"* F_TIMER_TYPE\t\t\t2\t*"<<endl;
		cout<<"* PID_STATE_TIMER_TYPE\t\t3\t*"<<endl;
		cout<<"* OFF_TIMER_TYPE\t\t4\t*"<<endl;
		cout<<"* KEEPALIVE_TIMER_TYPE\t\t5\t*"<<endl;
		cout<<"* KEEPALIVE_STATE_TIMER_TYPE\t6\t*"<<endl;
		cout<<"* UT_TIMER_TYPE\t\t\t7\t*"<<endl;
		cout<<"*****************************************"<<endl;
	}
	//**************************处理接收包（DATA）******************************************
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_DATA* pPKG_DATA;//为什么要用结构体指针，利用结构体指针来取数据包的每一个字段
	pPKG_DATA=(CoLoR_NEW_DATA*)RecvBuf;
	char test_sid1[10];//为了取出”pidupdate“
	for (int i = 0; i < 9; ++i) 
	test_sid1[i]=pPKG_DATA->l_sid[i]; 
	test_sid1[9]='\0';
	//**************************更新对应的隧道列表条目**************************
	if (!strcmp(UPDATE_SID,test_sid1)/*为pid更新包*/)
	{		
		cout<<endl<<"--->>> Module: void Updatepid_TAG_Process::push(int port, Packet *packet) "<<endl;
		cout<<"--->>> RECV_data_packet_num = "<<packet_num<<"   ";
		packet_num++;
		Data_NUM=pPKG_DATA->res;
		cout<<"---   RECV_Data_NUM ="<<Data_NUM<<" ---   SEND_GET_NUM = "<<Get_NUM<<endl;
		//查找PID列表
		//依据隧道编号，查找对应的pidlist_entry条目
		struct _pidlist_info *p_pidlist;
		int tunnel_id = pPKG_DATA->reserved[1];
		//cout<<"tunnel_id = "<<tunnel_id<<endl;
		p_pidlist = find_pidlist_tunnel_id(PIDLIST,tunnel_id); // 根据隧道编号查找PID列表
		//查找TUNNEL列表,查找对应的tunnel_list条目,为了提取隧道列表中的隧道开启状态
		struct _tunnel_list_info *p_tunnel;
		p_tunnel = find_tunnel_entry(TUNNEL_LIST,tunnel_id);
		//找到对应的PID列表条目，并根据相应的更新规则更新
		if ( p_pidlist != NULL)
		{

			//修改条目的更新状态标志位
			if (Data_NUM == (Get_NUM-1))
			{
				p_pidlist->update_state =1;
				//print_pidlist_single(p_pidlist);//打印修改标志位后的条目
				//cout<<"--->>> p_pidlist->update_state ="<<p_pidlist->update_state <<endl;
			}
			else
				p_pidlist->update_state =0;
			cout<<"--->>> PIDUPDAT_STATE = "<<p_pidlist->update_state<<endl;
			//******************提取更新包（DATA）信息*****************************
			//收到的包信息
			int n;int c[8];
			n=pPKG_DATA->pids_o;
			c[0]=n/128;n%=128;
			c[1]=n/64;n%=64;
			c[2]=n/32;n%=32;
			c[3]=n/16;n%=16;
			c[4]=n/8;n%=8;
			c[5]=n/4;n%=4;
			c[6]=n/2;n%=2;
			c[7]=n;//for(iii=0;iii<=7;iii++) cout<<c[iii];
			//填充待插入的PID列表数据
			///*PID列表编号*/process_data.pidlist_entry =++T_NUMBER; 只有新建时才用
			/*隧道编号*/process_data.tunnel_num=tunnel_id;
			/*边界网关*/memcpy(process_data.TBG_NID,pPKG_DATA->n_sid,16);
			/*pid的数量*/process_data.pid_num=c[0]*16+c[1]*8+c[2]*4+c[3]*2+c[4]*1; //DATA包的PID数量有5位
			/*PIDs*/memcpy(process_data.PID,pPKG_DATA->PID,sizeof(pPKG_DATA->PID));
			/*TTL*/process_data.TTL=2*pPKG_DATA->minpid;
			/*Frequence*/process_data.Frequence=pPKG_DATA->minpid-2;
			/*start_update参数： 在set_pidlsit()函数中统一设置为0*/
			/*tunnel_state*/process_data.tunnel_state = p_tunnel->state; //从隧道列表提取的隧道开启状态
			
			//更新条件：pid列表条目中任一项发生变化都需要进行更新
			if ((strcmp((char*)p_pidlist->TBG_NID,(char*)process_data.TBG_NID)!=0/*TBG_NID发生变化*/)||(strcmp((char*)p_pidlist->PID,(char*)process_data.PID)!=0/*PID发生变化*/)||(p_pidlist->pid_num != process_data.pid_num)/*PID数量发生表化*/||(p_pidlist->Frequence != process_data.Frequence)/*RM中PID最小更新周期*/)
			{
				printf("--->>> Update [pidlist_entry %d]\n", p_pidlist->pidlist_entry );
				//更新方法：在原条目上进行修改，用新数据代替旧数据
				memcpy( p_pidlist->TBG_NID,process_data.TBG_NID,16);
				p_pidlist->pid_num = process_data.pid_num;
				memcpy( p_pidlist->PID,process_data.PID,4*5);
				p_pidlist->TTL = process_data.TTL;
				p_pidlist->Frequence = process_data.Frequence;
			}
			else
				printf("--->>> [pidlist_entry %d] keeps no change\n", p_pidlist->pidlist_entry );
		}
		else //不能找到更新包对应的PID列表条目
		{
			printf("--->>> ERROR!! Cannot find !!");
			output(0).push(packet);//将收到的数据包丢弃
		}		
	}
}
/*******************************************************************************
*******************************隧道维护模块*************************************/
void TAG_MAINT_SEND::push(int port, Packet *packet1)
{
	cout<<endl<<"--->>> Module: void TAG_MAINT_SEND::push(int port, Packet *packet) "<<endl;
	//***********初始化定时器*************************
	if(timer_unlaunch){
		memset(&timer_manage, 0, sizeof(struct _timer_manage));
		init_mul_timer(&timer_manage);//初始化定时器
		timer_unlaunch = 0;
		//打印定时器类型值表：
		cout<<"***************TIMER-TYPE****************"<<endl;
		cout<<"* TYPE\t\t\t\tVALUE\t*"<<endl;
		cout<<"* TTL_TIMER_TYPE\t\t1\t*"<<endl;
		cout<<"* F_TIMER_TYPE\t\t\t2\t*"<<endl;
		cout<<"* PID_STATE_TIMER_TYPE\t\t3\t*"<<endl;
		cout<<"* OFF_TIMER_TYPE\t\t4\t*"<<endl;
		cout<<"* KEEPALIVE_TIMER_TYPE\t\t5\t*"<<endl;
		cout<<"* KEEPALIVE_STATE_TIMER_TYPE\t6\t*"<<endl;
		cout<<"* UT_TIMER_TYPE\t\t\t7\t*"<<endl;
		cout<<"*****************************************"<<endl;
	}
	//在隧道开启时，进行隧道关系的维护：周期性的发送keepalive报文
	if (open == 1 && Keepalive_launch == 1)//隧道关系期间并且计时器到时
	{
	 	//动作一：插入新的Keepalive计时器 
		char tstr[200];
		get_format_time(tstr);//获取当前时间
		printf("%s: insert Keepalive timer \n", tstr);
		struct _timer_info* timer_info1;
		uint16_t time1 = Keepalive_time;//互动时间为10s
		//int id = 0;//改为全局变量TIMER_ID
		if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
		set_child_timer(time1, ++TIMER_ID, KEEPALIVE_TIMER_TYPE, launch_tunnel_id);//插入子定时器
		print_timer_manage(timer_manage.timer_info);//打印定时时间表
		Keepalive_launch=0;
		//插入保活包的状态计时器
		if (Keepalive_state_launch == 1)
		{
			get_format_time(tstr);//获取当前时间
			printf("%s: insert Keepalive_state timer \n", tstr);
			struct _timer_info* timer_info2;
			uint16_t time2 = Keepalive_state_time;//互动时间为21s
			if (TIMER_ID == 100)TIMER_ID=0; //将定时器编号重置为0
			set_child_timer(time2, ++TIMER_ID, KEEPALIVE_STATE_TIMER_TYPE,launch_tunnel_id);//插入子定时器
			print_timer_manage(timer_manage.timer_info);//打印定时时间表
			Keepalive_state_launch=0;
			KEEPALIVE_STATE = 0;//在插入保活状态计时器的时刻对应答状态进行复位
			cout<<"KEEPALIVE_STATE = "<<KEEPALIVE_STATE<<endl;
		}

		//动作二：发送互动包
		Packet *packet;
		CoLoR_NEW_DATA PKG;
		memset(&PKG,0,sizeof(CoLoR_NEW_DATA));//用来结构体清零
		//***封装data包: PID\data\minpid字段从PID列表中获取*************************
		//封装mac层头部
		PKG.ether_dhost[0] = 0xBB;//目的地址
		PKG.ether_dhost[1] = 0xBB;
		PKG.ether_dhost[2] = 0xBB;
		PKG.ether_dhost[3] = 0xBB;
		PKG.ether_dhost[4] = 0xBB;
		PKG.ether_dhost[5] = 0xBB;
		PKG.ether_shost[0] = 0xAA;//源地址
		PKG.ether_shost[1] = 0xAA;
		PKG.ether_shost[2] = 0xAA;
		PKG.ether_shost[3] = 0xAA;
		PKG.ether_shost[4] = 0xAA;
		PKG.ether_shost[5] = 0xAA;
		PKG.ether_type = 0x0008;		

		PKG.version_type=161;//10100001
		PKG.ttl=255;
		PKG.total_len=134;

		PKG.port_no1=1;
		PKG.port_no2=2;

		//PKG.minpid=pPKG_GET->minpid; //最小的PID更新时间也需要同步
		PKG.pids_o=12;//00001100,PID数量占5位，其他位o=1,c=0,n=0;
		PKG.res=0;//这个字段只在更新包中使用

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
		
		//从PID列表中提取
		memcpy((char *)PKG.n_sid,PIDLIST->TBG_NID,16);

		uint8_t maint_l_sid[20]={'t','u','n','n','e','l','_','m','a','i','n','t','\0',0,0,0,0,0,0,0};
		for(iii=0;iii<20;iii++)
		PKG.l_sid[iii]=maint_l_sid[iii];

		//目的NID, 从PID列表中提取
		memcpy((char *)PKG.nid,PIDLIST->TBG_NID,8); 
		memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID ，直接进行封装

		//PID和数据字段由PID列表获取
		memcpy(PKG.PID,PIDLIST->PID,sizeof(PIDLIST->PID));
		memcpy(PKG.data,PIDLIST->PID,sizeof(PIDLIST->PID));
		//cout<<PKG.PID<<endl;
		//cout<<PKG.data<<endl;

		//******************构造数据包并发送data包**********************************
		packet = Packet::make(sizeof(CoLoR_NEW_DATA));
		memcpy((unsigned char *)packet->data(),&PKG,sizeof(CoLoR_NEW_DATA));
		output(0).push(packet);
		cout<<"PACKET-SEND : keepalive packet sent!"<<endl;
	}
}

void TAG_MAINT_RECV::push(int port, Packet *packet)
{
	//***********接收TBG的保活应答包****************************
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_DATA* pPKG_DATA;//为什么要用结构体指针，利用结构体指针来取数据包的每一个字段
	pPKG_DATA=(CoLoR_NEW_DATA*)RecvBuf;
	//*********取出应答包中SID标识进行包服务判断**********
	char test_sid[13];//为了取出“tunnel_maint”
	for (int i = 0; i < 12; ++i) test_sid[i]=pPKG_DATA->l_sid[i]; test_sid[12]='\0';
	//对SID进行判断
	if (!strcmp(MAINT_SID,test_sid)/*为隧道维护包*/)
	{
		//判断隧道编号
		cout<<"PACKET-RECV : this is a keepalive response packet!"<<endl;
		KEEPALIVE_STATE = 1;//在插入保活状态计时器的时刻对应答状态进行复位
		cout<<"KEEPALIVE_STATE = "<<KEEPALIVE_STATE<<endl;
	}
}
//出现故障时向RM报告
void TROUBLE_REPORT::push(int port, Packet *packet)
{
	if (ALARM_Breakdown == 1)
	{	
		int Recvlength=0;
		char RecvBuf[1024*5];
		memcpy(RecvBuf,packet->data(),packet->length());
		Recvlength=packet->length();
		struct CoLoR_NEW_GET PKG;
		memset(&PKG,0,sizeof(CoLoR_NEW_GET));	
		//封装mac层头部
		PKG.ether_dhost[0] = 0xCC;//目的地为RM
		PKG.ether_dhost[1] = 0xCC;
		PKG.ether_dhost[2] = 0xCC;
		PKG.ether_dhost[3] = 0xCC;
		PKG.ether_dhost[4] = 0xCC;
		PKG.ether_dhost[5] = 0xCC;
		PKG.ether_shost[0] = 0xAA;//源地址
		PKG.ether_shost[1] = 0xAA;
		PKG.ether_shost[2] = 0xAA;
		PKG.ether_shost[3] = 0xAA;
		PKG.ether_shost[4] = 0xAA;
		PKG.ether_shost[5] = 0xAA;
		PKG.ether_type = 0x0008;	

		PKG.version_type=160;//10100000
		PKG.ttl=255;
		PKG.total_len=126;

		PKG.port_no1=1;
		PKG.port_no2=2;

		PKG.minpid=0;
		PKG.pids_o=0;//00000000,pid数量占7位，offset字段占一位
		PKG.res=0;

		int iii;
		uint8_t offset[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.offset[iii]=offset[iii];

		uint8_t length[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.length[iii]=length[iii];

		PKG.content_len=1000;
		PKG.mtu=1500;

		PKG.publickey_len=256;
		PKG.checksum=123;

		memcpy((char *)PKG.n_sid,"TBG1",4);//出现故障的TBG的NID
		uint8_t  trouble_l_sid[20]={'t','r','o','u','b','l','e','_','r','e','p','o','r','t','\0',0,0,0,0,0};
		for(iii=0;iii<20;iii++)
		PKG.l_sid[iii]=trouble_l_sid[iii];
		
		memcpy((char *)PKG.nid,"RM1",3);         // 本域的RM
		memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID,直接进行封装

		uint8_t content_Ch[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.content_Ch[iii]=content_Ch[iii];

		uint8_t publickey[4]={255,255,255,255};
		for(iii=0;iii<4;iii++)
		PKG.publickey[iii]=publickey[iii];

		memset(PKG.PID,0,sizeof(PKG.PID));

		//****************向RM发送疑似故障节点信息**************
		memcpy((unsigned char *)packet->data(),&PKG,126);
		//修改数据包的长度,只发出get包的大小,122为整个get包的长度
		uint32_t addlength;
		if(Recvlength<126)//可能不会出现比122还小的ip包
		{
			addlength=126-Recvlength;
			packet->put(addlength);
		}
		output(0).push(packet);
		cout<<"**TROUBLE REPORT** : packet has sent to RM !!"<<endl;
		ALARM_Breakdown=0;//故障报告后，置为0,即一次故障上报一次
	}
}

//接收来自RM的故障确认结果
char TROUBLE_YES[20]={'t','r','o','u','b','l','e','_','c','o','n','f','i','r','m','_','y','e','s','\0'}; //故障标志包
char TROUBLE_NO[19]={'t','r','o','u','b','l','e','_','c','o','n','f','i','r','m','_','n','o','\0'}; //无故障标志包
void TROUBLE_CONFIRM::push(int port, Packet *packet)
{
	//根据收到的get包进行查询
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_GET* pPKG;
	pPKG=(CoLoR_NEW_GET*)RecvBuf;
	char trouble_confirm_sid[20];//取出故障包中的sid
	memcpy(trouble_confirm_sid, pPKG->l_sid,20);
	//匹配SID，确认是否故障
	if (!strcmp(TROUBLE_YES,trouble_confirm_sid))
	{ 
		TROUBLE_CONFIRMATION = 1;//确认发生故障
		cout<<"\nRECV trouble confirmation from RM: ";
		cout<<"TROUBLE_CONFIRMATION = "<<TROUBLE_CONFIRMATION<<endl;
		cout<<pPKG->n_sid<<" has acctualy already broken down ! "<<endl;
	}
	else if (!strcmp(TROUBLE_NO,trouble_confirm_sid))
	{
		TROUBLE_CONFIRMATION = 0; //确认没有故障，误报了
		cout<<"\nRECV trouble confirmation from RM: ";
		cout<<"TROUBLE_CONFIRMATION = "<<TROUBLE_CONFIRMATION<<endl;
		cout<<pPKG->n_sid<<" is working properly ! mis_trouble_report"<<endl;
	}
}


//apple
//****************************正向封装模块**************************
void CoLoR_Encapsulation::push(int port, Packet *packet)//用于发送不需分片的包或者是第一分片
{
	cout<<endl<<"--->>> void CoLoR_Encapsulation::push(int port, Packet *packet)"<<endl;
	//新的包格式下的包处理
	int Recvlength=0;
	memcpy(RecvBuf1,packet->data(),packet->length());
	Recvlength=packet->length();
	//hu/cout<<"From Client:Recvlength ="<<Recvlength<<"\t";
	CoLoR_NEW_DATA PKG;
	memset(&PKG,0,sizeof(CoLoR_NEW_DATA));//用来结构体清零
   for(;;)
   {
	if(Recvlength>1514)
		break;
	else
	{
		//如果没有匹配到TBG，将数据包存在缓存里
		if(add_TBG == 0)//add_TBG的值由REQUST_TBG模块中的LPM（）函数控制
		{
			struct pac_cache cache;
			cache.length=packet->length();
			cache.p=(char*)packet->data();
			packet_cache.insert(packet_cache.size()+1,cache);
			cout<<"--->>> LPM failed, No macth TBG ! Packet has been cached!"<<"\n";
			printf("--->>> packet_cache_size=%d\n",packet_cache.size());
			break;
		}
		  else //匹配到TBG了！
	  	{
			//********************填充CoLoR-data包文**********************
			PKG.version_type=161;//10100001
			PKG.ttl=255;
			PKG.total_len=134;

			PKG.port_no1=1;
			PKG.port_no2=2;

			PKG.minpid=0; //最小的PID更新时间也需要同步
			PKG.pids_o=4;//00000100,PID数量占5位，其他位o=1,c=0,n=0;
			PKG.res=0;

			int iii;
			uint8_t offset[4]={0,0,0,0};
			for(iii=0;iii<4;iii++)
			PKG.offset[iii]=offset[iii];

			uint8_t length[4]={0,0,0,0};
			for(iii=0;iii<4;iii++)
			PKG.length[iii]=length[iii];

			PKG.next_header=0;
			PKG.checksum=123;
			
			//查找隧道信息
			struct _tunnel_list_info *find_p;
			find_p=find_tunnel_entry(TUNNEL_LIST,tunnel_id_LMP);//根据LPM结果查找
			//reserved[0]用于标志同域或者异域，reserved[1]用于存储隧道编号
			memset(PKG.reserved,0,sizeof(PKG.reserved));
			PKG.reserved[0]=find_p->position;
			PKG.reserved[1]=find_p->tunnel_id;
			//printf("reserved[0]=find_p->position=%d\n", PKG.reserved[0]);
			//printf("reserved[1]=find_p->tunnel_id=%d\n", PKG.reserved[1]);

			uint8_t n_sid[16]={'T','A','G',0,0,0,0,0,0,0,0,0,0,0,0,0};
			for(iii=0;iii<16;iii++)
			PKG.n_sid[iii]=n_sid[iii];

			uint8_t l_sid[20]={'t','u','n','n','e','l','_','s','e','r','v','i','c','e',0,0,0,0,0,0}; 
			for(iii=0;iii<20;iii++)
			PKG.l_sid[iii]=l_sid[iii];

			//将匹配到的TBG封装
			memcpy((char *)PKG.nid,find_p->TBG_NID,8); //目的NID
			memcpy((char *)(PKG.nid+8),"TAG1",4); //源NID

			//PID信息需要从PID列表中获取
			//memcpy(PKG.PID,pPKG_GET->PID,sizeof(pPKG_GET->PID));
			////hu/cout<<PKG.PID<<endl;

			//**************填充数据字段和修改MAC地址***************************
			memcpy(out_device1,OUT_DEVICE,DEVICELENGTH);
			GetLocalMac1(out_device1, out_mac1, out_ip1);

			//*************先将缓存里的包依次发出**********
			int packet_num=1;
			Packet *p;
			struct pac_cache cache;
			if(packet_cache.size()!=0)
			printf("--->>> Firstly send cached packets  :  packet_cache_size=%d\n",packet_cache.size());
			while(packet_cache.size()!=0)
			{	
				cache=packet_cache.find(packet_num);
				//hu/printf("cache_packet_length=%d\n",cache.length);
				p = Packet::make(cache.length);
				memcpy((unsigned char *)p->data(),cache.p,cache.length);
				int Recvlength1=p->length();
				//如果转发包的长度大于1000（data字段的大小），即接近MTU值，为了方便封装隧道包头传输，需要切割单包
				if(Recvlength1>1000) //根据data字段的大小来进行分片，只发送第一个分片
				{
					//因为CoLoR包封装的mac地址暂时没有作用，此处用于标识网口，故源和目的封装成为一样的mac对程序本身的功能是没有影响的。
					PKG.ether_dhost[0] = out_mac1[0];//目的h3-eth0
					PKG.ether_dhost[1] = out_mac1[1];
					PKG.ether_dhost[2] = out_mac1[2];
					PKG.ether_dhost[3] = out_mac1[3];
					PKG.ether_dhost[4] = out_mac1[4];
					PKG.ether_dhost[5] = out_mac1[5];
					PKG.ether_shost[0] = out_mac1[0];//源改自己
					PKG.ether_shost[1] = out_mac1[1];
					PKG.ether_shost[2] = out_mac1[2];
					PKG.ether_shost[3] = out_mac1[3];
					PKG.ether_shost[4] = out_mac1[4];
					PKG.ether_shost[5] = out_mac1[5];
					PKG.ether_type = 0x0008;
					PKG.offset[0] = 1;
					PKG.length[0] = Recvlength1/2/256;
					PKG.length[1] = Recvlength1/2%256;

					//******************发送第一个分片**************//
					memcpy(PKG.data,RecvBuf1,Recvlength1/2);//填充一半的IP包数据作为第一分片
					memcpy(RecvBuf1,&PKG,sizeof(CoLoR_NEW_DATA));//拷贝填充好包头和数据的data包
					//printf("%d\n", sizeof(CoLoR_NEW_DATA));//应该是1114字节
					memcpy((char *)p->data(),(const char *)RecvBuf1,sizeof(CoLoR_NEW_DATA));

					//修改数据包的长度，不修改的话只能发出接收包长度的数据，数据不完全
					uint32_t takelength1;
					uint32_t addlength1;
					if(Recvlength1<1114)//1114为整个data包的长度
					{
						addlength1=1114-Recvlength1;
						p->put(addlength1);
					}
					else
					{
						takelength1=Recvlength1-1114;
						p->take(takelength1);
					}

					////hu/printf("changedlength_CoLoR=%d\t",packet->length());
					output(0).push(p);
					//输出提示
					printf("--->>> CachePKG_CoLoR sent out.(Divided Pkg, No.1, length: %d)\n",Recvlength1/2);
					
					//t/
					//**************发送第二个分片****************************
					PKG.offset[0] = 2;   //标志位第二分片
					Packet *packet1;
					packet1 = Packet::make(sizeof(CoLoR_NEW_DATA)); //make 一个空包封装第二分片
					memcpy(PKG.data,RecvBuf2+Recvlength1/2,Recvlength1 - Recvlength1/2);//填充后一半的IP包数据作为第二分片
					memcpy(RecvBuf2,&PKG,sizeof(CoLoR_NEW_DATA));//拷贝填充好包头和数据的data包
					//printf("%d\n", sizeof(CoLoR_NEW_DATA));//应该是1114字节
					memcpy((char *)packet1->data(),(const char *)RecvBuf2,sizeof(CoLoR_NEW_DATA));
					//printf("changedlength_CoLoR=%d\t",packet1->length());
					output(0).push(packet1);
					//输出提示
					printf("--->>> CachePKG_CoLoR sent out.(Divided Pkg, No.2, length: %d)\n",Recvlength1/2);
				}
				else//发送不分片的包
				{
					PKG.ether_dhost[0] = out_mac1[0];//目的h3-eth0
					PKG.ether_dhost[1] = out_mac1[1];
					PKG.ether_dhost[2] = out_mac1[2];
					PKG.ether_dhost[3] = out_mac1[3];
					PKG.ether_dhost[4] = out_mac1[4];
					PKG.ether_dhost[5] = out_mac1[5];
					PKG.ether_shost[0] = out_mac1[0];//源改自己
					PKG.ether_shost[1] = out_mac1[1];
					PKG.ether_shost[2] = out_mac1[2];
					PKG.ether_shost[3] = out_mac1[3];
					PKG.ether_shost[4] = out_mac1[4];
					PKG.ether_shost[5] = out_mac1[5];
					PKG.ether_type = 0x0008;
					PKG.offset[0] = 0;
					PKG.length[0] = Recvlength1/256;
					PKG.length[1] = Recvlength1%256;
				
					//****************填充数据********************//
					memcpy(PKG.data,RecvBuf1,Recvlength1); //将整个IP包封装进data包的数据部分
					memcpy(RecvBuf1,&PKG,sizeof(CoLoR_NEW_DATA));
					memcpy((char *)p->data(),(const char *)RecvBuf1,sizeof(CoLoR_NEW_DATA));

					//修改数据包的长度，不修改的话只能发出接收包长度的数据，数据不完全
					uint32_t addlength;
					addlength=1114-Recvlength1;
					p->put(addlength);
					//hu/printf("changedlength_CoLoR=%d\t",p->length());
					output(0).push(p);
					//输出提示
					printf("--->>> CachePKG_CoLoR sent out.(None Divided Pkg, length: %d)\n",Recvlength1);
				}
				packet_cache.remove(packet_num); 
				packet_num++;
			}//while()
			//***********再发送此次收到的包*************
			 if(Recvlength>1000) //根据data字段的大小来进行分片，只发送第一个分片
			{
				//因为CoLoR包封装的mac地址暂时没有作用，此处用于标识网口，故源和目的封装成为一样的mac对程序本身的功能是没有影响的。
				PKG.ether_dhost[0] = out_mac1[0];//目的h3-eth0
				PKG.ether_dhost[1] = out_mac1[1];
				PKG.ether_dhost[2] = out_mac1[2];
				PKG.ether_dhost[3] = out_mac1[3];
				PKG.ether_dhost[4] = out_mac1[4];
				PKG.ether_dhost[5] = out_mac1[5];
				PKG.ether_shost[0] = out_mac1[0];//源改自己
				PKG.ether_shost[1] = out_mac1[1];
				PKG.ether_shost[2] = out_mac1[2];
				PKG.ether_shost[3] = out_mac1[3];
				PKG.ether_shost[4] = out_mac1[4];
				PKG.ether_shost[5] = out_mac1[5];
				PKG.ether_type = 0x0008;
				PKG.offset[0] = 1;
				PKG.length[0] = Recvlength/2/256;
				PKG.length[1] = Recvlength/2%256;

				//******************发送第一个分片**************//
				memcpy(PKG.data,RecvBuf1,Recvlength/2);//填充一半的IP包数据作为第一分片
				memcpy(RecvBuf1,&PKG,sizeof(CoLoR_NEW_DATA));//拷贝填充好包头和数据的data包
				//printf("%d\n", sizeof(CoLoR_NEW_DATA));//应该是1114字节
				memcpy((char *)packet->data(),(const char *)RecvBuf1,sizeof(CoLoR_NEW_DATA));

				//修改数据包的长度，不修改的话只能发出接收包长度的数据，数据不完全
				uint32_t takelength1;
				uint32_t addlength1;
				if(Recvlength<1114)//1114为整个data包的长度
				{
					addlength1=1114-Recvlength;
					packet->put(addlength1);
				}
				else
				{
					takelength1=Recvlength-1114;
					packet->take(takelength1);
				}

				//printf("changedlength_CoLoR=%d\t",packet->length());
				output(0).push(packet);
				//输出提示
				printf("--->>> CoLoR_Eecapsulation DATA packet sent out.(Divided Pkg, No.1, length: %d)\n",Recvlength/2);
				//t/
				//**************发送第二个分片****************************
				PKG.offset[0] = 2;   //标志位第二分片
				Packet *packet1;
				packet1 = Packet::make(sizeof(CoLoR_NEW_DATA)); //make 一个空包封装第二分片
				memcpy(PKG.data,RecvBuf2+Recvlength/2,Recvlength - Recvlength/2);//填充后一半的IP包数据作为第二分片
				memcpy(RecvBuf2,&PKG,sizeof(CoLoR_NEW_DATA));//拷贝填充好包头和数据的data包
				//printf("%d\n", sizeof(CoLoR_NEW_DATA));//应该是1114字节
				memcpy((char *)packet1->data(),(const char *)RecvBuf2,sizeof(CoLoR_NEW_DATA));
				//printf("changedlength=%d\t",packet1->length());
				output(0).push(packet1);
				//输出提示
				printf("--->>> CoLoR_Eecapsulation DATA packet sent out.(Divided Pkg, No.2, length: %d)\n",Recvlength/2);

			}
			else//发送不分片的包
			{
				PKG.ether_dhost[0] = out_mac1[0];//目的h3-eth0
				PKG.ether_dhost[1] = out_mac1[1];
				PKG.ether_dhost[2] = out_mac1[2];
				PKG.ether_dhost[3] = out_mac1[3];
				PKG.ether_dhost[4] = out_mac1[4];
				PKG.ether_dhost[5] = out_mac1[5];
				PKG.ether_shost[0] = out_mac1[0];//源改自己
				PKG.ether_shost[1] = out_mac1[1];
				PKG.ether_shost[2] = out_mac1[2];
				PKG.ether_shost[3] = out_mac1[3];
				PKG.ether_shost[4] = out_mac1[4];
				PKG.ether_shost[5] = out_mac1[5];
				PKG.ether_type = 0x0008;
				PKG.offset[0] = 0;
				PKG.length[0] = Recvlength/256;
				PKG.length[1] = Recvlength%256;
				//****************填充数据********************//
				memcpy(PKG.data,RecvBuf1,Recvlength); //将整个IP包封装进data包的数据部分
				memcpy(RecvBuf1,&PKG,sizeof(CoLoR_NEW_DATA));
				memcpy((char *)packet->data(),(const char *)RecvBuf1,sizeof(CoLoR_NEW_DATA));
				//修改数据包的长度，不修改的话只能发出接收包长度的数据，数据不完全
				uint32_t addlength;
				addlength=1114-Recvlength;
				packet->put(addlength);
				//hu/printf("changedlength_CoLoR=%d\t",packet->length());
				output(0).push(packet);
				//输出提示
				printf("--->>> CoLoR_Eecapsulation DATA packet sent out.(None Divided Pkg, length: %d)\n",Recvlength);
			}//else		
		}//else
	}//else
	break;
     }//for
}
//************************反向解封装处理模块函数定义**********************************************
unsigned char RecvBuf_data[bufsize] = {0};
//用于数据包的分片整合
typedef struct _Gather Gather;
struct _Gather
{
	uint8_t flagRECV;
	uint8_t CachePKG[2][1514];
};
Gather List_data;

//mania//函数解释见/code/port_tbl.cc文件
void InsertMap(HashMap<String,String> &tbl){
	ifstream tbl_file("/home/apple/click-2.0.1/conf/port_tbl");
	bool loop = true;
	bool choose_buf;
	char mac[32] = {'\0'};
	char port[8] = {'\0'};
	char c = '\0';
	int i = 0;
	while(loop){
		for(i = 0, choose_buf = true; (c = tbl_file.get()) != '\n'; ++i){
			if(c == EOF){
				loop = false;
				break;
			}
			else if(c == ' '){
				mac[i] = '\0';
				i = -1;
				choose_buf = false;
			}
			else{
				if(choose_buf){
					mac[i] = c;
				}
				else{
					port[i] = c;
				}
			}
		}
		port[i] = '\0';
		tbl.insert(mac, port);
	}
	tbl_file.close();
}
char PIDUPDATE_SID[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
int ftp_total_len_color=0;
//************************反向处理模块**********************************************
void CoLoR_Decapsulation::push(int port, Packet *packet)
{
	char RecvBuf[bufsize];
	memset(RecvBuf, 0, bufsize);
	memcpy(RecvBuf,packet->data(),packet->length());
	/*
	//统计TBG到TAG的入口流量
	ftp_total_len_color+=packet->length();
	cout<<"TAG : ftp_total_len_color = "<<ftp_total_len_color<<endl;
	*/
	CoLoR_NEW_DATA* pPKG_RECV;//为什么要用结构体指针，利用结构体指针来取数据包的每一个字段
	pPKG_RECV=(CoLoR_NEW_DATA*)RecvBuf;
	char test_sid[20];
	for (int i = 0; i < 20; ++i)
	test_sid[i]=pPKG_RECV->l_sid[i];
	//判断是否为数据包
	if (!strcmp(TUNNEL_SID,test_sid))
	{
		cout<<endl<<"--->>> void CoLoR_Decapsulation::push(int port, Packet *packet)"<<endl;
		//建立mac与网口的对应关系表
	    	HashMap<String,String>tbl;
	    	InsertMap(tbl);
	    	//用来验证建表与读取表的正确性，配置好port—tbl表后，先用这段程序验证下！！！
	    	/*char dest_mac111[18] = {'\0'};
	    	int output_port111;
	    	String _val ;
		sprintf(dest_mac111, "52:85:87:8b:3f:d4");
		//hu/cout<<dest_mac111<<endl;
		_val = tbl.find(dest_mac111);
		output_port111 = atoi(_val.c_str());
		//hu/cout<<output_port111<<endl;
		sprintf(dest_mac111, "e6:40:41:91:c8:c6");
		//hu/cout<<dest_mac111<<endl;
		_val = tbl.find(dest_mac111);
		output_port111 = atoi(_val.c_str());
		//hu/cout<<output_port111<<endl;
		sprintf(dest_mac111, "b2:34:a7:02:f5:17");
		//hu/cout<<dest_mac111<<endl;
		_val = tbl.find(dest_mac111);
		output_port111 = atoi(_val.c_str());
		//hu/cout<<output_port111<<endl;
		sprintf(dest_mac111, "fe:80:79:f8:c3:df");
		//hu/cout<<dest_mac111<<endl;
		_val = tbl.find(dest_mac111);
		output_port111 = atoi(_val.c_str());
		//hu/cout<<output_port111<<endl;
		sprintf(dest_mac111, "06:b0:11:79:04:bf");
		//hu/cout<<dest_mac111<<endl;
		_val = tbl.find(dest_mac111);
		output_port111 = atoi(_val.c_str());
		//hu/cout<<output_port111<<endl;
	    	*/
	    	//tend//
		unsigned char pkg[bufsize];
		memset(pkg,0,sizeof(pkg));
		CoLoR_NEW_DATA * pPKG;
		pPKG = (CoLoR_NEW_DATA *)pkg;
		int RecvLength=0;
		int pkglength=0;
		RecvLength=packet->length();
		memcpy(RecvBuf_data,packet->data(),packet->length());
		memcpy(pkg,RecvBuf_data,RecvLength);//pkg里面存的是CoLoR包
		if(pPKG->offset[0] == 1)
		{
			//hu/printf("[From TBG:Divided = %d RECV Length] %d\n",pPKG->offset[0],RecvLength);
			if(List_data.flagRECV == 0)
			{
				List_data.flagRECV = 1;
				memcpy(List_data.CachePKG[0],pkg,RecvLength);
			}
			else
			{
				List_data.flagRECV = 0;
				memset(List_data.CachePKG,0,1514*2);
			}
		}
		else if(pPKG->offset[0] == 2)
		{
			printf("--->>> [From TBG:Divided = %d RECV Length] %d\n",pPKG->offset[0],RecvLength);
			if(List_data.flagRECV == 1)
			{
				memcpy(List_data.CachePKG[1],pkg,RecvLength);
				//合并第一片的数据
				pPKG = (CoLoR_NEW_DATA *)List_data.CachePKG[0];
				pkglength = pPKG->length[0]*256+pPKG->length[1];
				memcpy(pkg,pPKG->data,pkglength);
				//合并第二片的数据
				pPKG = (CoLoR_NEW_DATA *)List_data.CachePKG[1];
				memcpy(pkg+pkglength,pPKG->data,pPKG->length[0]*256+pPKG->length[1]);
				pkglength+=pPKG->length[0]*256+pPKG->length[1];

				//*******************修改数据包长度*********************
				uint32_t addlength;
				addlength=pkglength-1114;
				packet->put(addlength);
				//hu/printf("changedlength_IP=%d\n",packet->length());
				//********************添加数据***************************
				memcpy((char *)packet->data(),(const char *)pkg,pkglength);//pkg现在里面存的是IP包
				//*****************查表找出对应网口**********************
				//hu/cout<<"divided"<<'\t';
				char dest_mac[18] = {'\0'};
				sprintf(dest_mac, "%02x:%02x:%02x:%02x:%02x:%02x", pkg[0], pkg[1], pkg[2], pkg[3], pkg[4], pkg[5]);
				//hu/cout<<dest_mac<<'\t';
				String _val = tbl.find(dest_mac);
				int output_port = atoi(_val.c_str());
				//hu/cout<<output_port<<endl;
				if(output_port == 1)
				{ 
					output(0).push(packet);
				}
				else if(output_port == 2) 
				{
					output(1).push(packet);
				}
				else if(output_port == 3)   
				{
					output(2).push(packet);
				}
				else if(output_port == 4)   
				{
					output(3).push(packet);
				}
				else if(output_port == 5)      
				{
					output(4).push(packet);
				}
				else
				{
					//hu/printf("can not find!!\n");
				}
				printf("--->>> CoLoR_Decapsulation IP packet sent out.(Gathered Pkg, length: %d)\n",pkglength);
				/*
				//TAG到用户的出口流量统计——分片总流量1
				ftp_total_len_ip+=pkglength;
				cout<<"TAG : ftp_total_len_ip = "<<ftp_total_len_ip<<endl;
				*/
			}

			List_data.flagRECV = 0;
			memset(List_data.CachePKG,0,1514*2);
		}
		else
		{
			pkglength = pPKG->length[0]*256+pPKG->length[1];
			memcpy(pkg,pPKG->data,pkglength);
			printf("--->>> [From TBG:RECV Length] %d\n",RecvLength);
				
			//*****************修改数据包长度*********************
			uint32_t takelength;
			takelength=1114-pkglength;
			packet->take(takelength);
			//hu/printf("changedlength_IP=%d\n",packet->length());

			//******************添加数据**************************
			memcpy((char *)packet->data(),(const char *)pkg,pkglength);

			//******************查表找出对应网口*******************
			//hu/cout<<"non-divided"<<'\t';
			char dest_mac1[18] = {'\0'};
			sprintf(dest_mac1, "%02x:%02x:%02x:%02x:%02x:%02x", pkg[0], pkg[1], pkg[2], pkg[3], pkg[4], pkg[5]);
			//hu/cout<<dest_mac1<<'\t';
			String _val1 = tbl.find(dest_mac1);
			int output_port1 = atoi(_val1.c_str());
			//hu/cout<<output_port1<<endl;
			if(output_port1 == 1)
			{ 
				output(0).push(packet);
			}
			else if(output_port1 == 2)
			{
				output(1).push(packet);
			}
			else if(output_port1 == 3)   
			{
				output(2).push(packet);
			}
			else if(output_port1 == 4)   
			{
				output(3).push(packet);
			}
			else if(output_port1 == 5)      
			{
				output(4).push(packet);
			}
			else
			{
				//hu/printf("can not find!!\n");
			}
			printf("--->>> CoLoR_Decapsulation IP packet sent out.(Non-Gathered Pkg, length: %d)\n",pkglength);
			//***********************************************************
			//TAG到用户的出口流量统计——不分片总流量2
			//ftp_total_len_ip+=pkglength;
			//cout<<"TAG : ftp_total_len_ip = "<<ftp_total_len_ip<<endl;
		}	
	}
	else//表明收到的是更新包或者是保活包
	{
		//cout<<"This is a updata or keepalive packet"<<endl;
		output(5).push(packet);//丢弃
	}	
}

//apple
CLICK_ENDDECLS
EXPORT_ELEMENT(REQUST_TBG)
EXPORT_ELEMENT(RECV_TBG)
EXPORT_ELEMENT(CoLoR_Encapsulation)
EXPORT_ELEMENT(CoLoR_Decapsulation)
EXPORT_ELEMENT(Updatepid_TAG_Send)
EXPORT_ELEMENT(Updatepid_TAG_Process)
EXPORT_ELEMENT(Updatepid_TAG_Creat)

ELEMENT_MT_SAFE(REQUST_TBG)
ELEMENT_MT_SAFE(RECV_TBG)
ELEMENT_MT_SAFE(CoLoR_Encapsulation)
ELEMENT_MT_SAFE(CoLoR_Decapsulation)
ELEMENT_MT_SAFE(Updatepid_TAG_Send)
ELEMENT_MT_SAFE(Updatepid_TAG_Process)
ELEMENT_MT_SAFE(Updatepid_TAG_Creat)

//maint
EXPORT_ELEMENT(TAG_MAINT_SEND)
ELEMENT_MT_SAFE(TAG_MAINT_SEND)
EXPORT_ELEMENT(TAG_MAINT_RECV)
ELEMENT_MT_SAFE(TAG_MAINT_RECV)
EXPORT_ELEMENT(TROUBLE_REPORT)
ELEMENT_MT_SAFE(TROUBLE_REPORT)
EXPORT_ELEMENT(TROUBLE_CONFIRM)
ELEMENT_MT_SAFE(TROUBLE_CONFIRM)

