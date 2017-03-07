#include <click/config.h>
#include <iostream>
#include "tunnel_TBG.hh"
using namespace std;
CLICK_DECLS

TBG_CoLoR_Decapsulation::TBG_CoLoR_Decapsulation(){}
TBG_CoLoR_Decapsulation::~TBG_CoLoR_Decapsulation(){}
Packet *
TBG_CoLoR_Decapsulation::simple_action(Packet *p)
{
  return p;
}

TBG_CoLoR_Encapsulation::TBG_CoLoR_Encapsulation(){}
TBG_CoLoR_Encapsulation::~TBG_CoLoR_Encapsulation(){}
Packet *
TBG_CoLoR_Encapsulation::simple_action(Packet *p)
{
  return p;
}

Updatepid_TBG_Process::Updatepid_TBG_Process(){}//构造函数
Updatepid_TBG_Process::~Updatepid_TBG_Process(){}
Packet *Updatepid_TBG_Process::simple_action(Packet *p)
{
  return p;
}

Updatepid_TBG_Send::Updatepid_TBG_Send(){}//析构函数
Updatepid_TBG_Send::~Updatepid_TBG_Send(){}
Packet *Updatepid_TBG_Send::simple_action(Packet *p)
{
  return p;
}

TBG_MAINT::TBG_MAINT(){}
TBG_MAINT::~TBG_MAINT(){}
Packet * TBG_MAINT::simple_action(Packet *p)
{
  return p;
}
//统计发出的总字节数
int ftp_total_len_ip1=0;
int ftp_total_len_color1=0;

//***************************PID更新模块各种定义***************************
#define  INITIATIVE  0           //是否为主动方的标志位，1表示主动方，0表示被动
int TBG_Frequence=0;	    	        //存储TAG的更新包周期
int TBG_TTL_TIMER_launch=0;	       //TTL计时器到时动作标志位
int TBG_TTL=0;		                    //存储PID的生存时间;
int TBG_S_TIME = TBG_Frequence+1;             //根据TAG的更新频率设定更新状态周期，与TAG中不同，TBG中应该大于F
int TBG_S_TIMER_launch = 0;		     //超时定时器到时动作标志位


//update

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
int TBG_TIMER_ID = 0;
int tbg_launch_tunnel_id=0;

struct tbg_timer_info
{
	int TBG_TIMER_ID; /* 定时器编号 */
	int timer_type;//定时器类型
	int elapse; /* 相对前一个子定时器的剩余时间 */
	int tunnel_id;/* 隧道编号*/
	time_t time;/* 自系统时间来已经经过的时间(1970年00:00:00) */
	int (* timer_proc) (int num);//动作入口
	struct tbg_timer_info* pre;
	struct tbg_timer_info* next;
};

struct tbg_timer_manage
{
	void (* old_sigfunc)(int);
	void (* new_sigfunc)(int);
	struct itimerval value, ovalue;
	struct tbg_timer_info* tbg_timer_info;
};

typedef int timer_handle_t;
struct tbg_timer_manage tbg_timer_manage;

//获取系统时间
void tbg_get_format_time(char *tstr)
{
	time_t t;
	
	t = time(NULL);
	strcpy(tstr, ctime(&t));
	tstr[strlen(tstr)-1] = '\0';
	
	return;
}

//主定时器到时动作函数：只对表头结点进行操作
void tbg_sig_func(int signo)
{
	int id;
	char tstr[200];
	int ret = 0;

	if(tbg_timer_manage.tbg_timer_info != NULL)
	{
		tbg_timer_manage.tbg_timer_info->elapse--; //只对表头结点时间减1
		//cout<<" int function tbg_sig_func() : tbg_timer_manage.tbg_timer_info->timer_type="<<tbg_timer_manage.tbg_timer_info->timer_type<<endl;
		while(tbg_timer_manage.tbg_timer_info->elapse <= 0)
		{
		   id = tbg_timer_manage.tbg_timer_info->TBG_TIMER_ID;
		   ret = tbg_timer_manage.tbg_timer_info->timer_proc(id);

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
int tbg_init_mul_timer(struct tbg_timer_manage *tbg_timer_manage)
{
	int ret;
	//设定定时器动作
	if( (tbg_timer_manage->old_sigfunc = signal(SIGALRM, tbg_sig_func)) == SIG_ERR)
	{
		return (-1);
	}
	tbg_timer_manage->new_sigfunc = tbg_sig_func;
	//设定定时周期
	tbg_timer_manage->value.it_value.tv_sec = MUL_TIMER_RESET_SEC;
	tbg_timer_manage->value.it_value.tv_usec = 0;
	tbg_timer_manage->value.it_interval.tv_sec = TIMER_UNIT;
	tbg_timer_manage->value.it_interval.tv_usec = 0;
	ret = setitimer(ITIMER_REAL, &tbg_timer_manage->value, &tbg_timer_manage->ovalue); 
	
	return (ret);
}

//清除主定时器
int tbg_destroy_mul_timer(struct tbg_timer_manage *tbg_timer_manage)
{
	int ret;
	
	if( (signal(SIGALRM, tbg_timer_manage->old_sigfunc)) == SIG_ERR)
	{
		return (-1);
	}

	memset(tbg_timer_manage, 0, sizeof(struct tbg_timer_manage));
	ret = setitimer(ITIMER_REAL, &tbg_timer_manage->ovalue, &tbg_timer_manage->value);
	if(ret < 0)
	{
		return (-1);
	} 
	
	printf("destroy multi timer\n");
	return ret;
}

//清除子定时器
struct tbg_timer_info* tbg_del_a_timer(struct tbg_timer_info* head, int value)
{
	struct tbg_timer_info* p;

	p = head;

	if(head == NULL)
	{
		printf("no timer %d\n", value);
		return NULL;
	}

	//删除第一个元素
	if(p->TBG_TIMER_ID == value)
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
		if(p->TBG_TIMER_ID == value)
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
	if(p->TBG_TIMER_ID == value)
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
struct tbg_timer_info* tbg_insert_a_timer(struct tbg_timer_info* head, struct tbg_timer_info* pnew)
{
	struct tbg_timer_info* p;
	time_t t;

	t = time(NULL);

	if(difftime(pnew->time, t) < 0)
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

	//第一个元素
	if(difftime(p->time, pnew->time) >= 0)
	{
		head = pnew;
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
			p->pre->next = pnew;
			pnew->next = p;
			pnew->pre = p->pre;
			p->pre = pnew;
			p->elapse = difftime(p->time, pnew->time);
			pnew->elapse = difftime(pnew->time, pnew->pre->time);
			return head;
		}

		p = p->next;
	}

	//最后一个元素
	if(difftime(p->time, pnew->time) < 0)
	{
		p->next = pnew;
		pnew->pre = p;
		pnew->next = NULL;
		pnew->elapse = difftime(pnew->time, p->time);
		return head;
	}
	else
	{
		p->pre->next = pnew;
		pnew->next = p;
		pnew->pre = p->pre;
		p->pre = pnew;
		p->elapse = difftime(p->time, pnew->time);
		pnew->elapse = difftime(pnew->time, pnew->pre->time);
		return head;
	}
}

//打印管理定时器结构体
void tbg_print_timer_manage(struct tbg_timer_info* head)
{
	struct tbg_timer_info* p;

	p = head;
	
	printf("TBG_TIMER_ID\ttype\telapse\ttunnel_id\n");

	while(p != NULL)
	{
		printf("%d\t\t%d\t%d\t%d\n", p->TBG_TIMER_ID, p->timer_type, p->elapse,p->tunnel_id);
		p = p->next;
	}
}

//设定子定时器执行启动函数
int tbg_timer_proc_launch(int num)
{
	char tstr[200];
	tbg_get_format_time(tstr);
	printf("\n%s: timer_proc:%d is launched. timer_type: %d\n", tstr, num,tbg_timer_manage.tbg_timer_info->timer_type);
  	//提取到时定时器所属的隧道
  	tbg_launch_tunnel_id = tbg_timer_manage.tbg_timer_info->tunnel_id;
  	//根据type值来判断到时的计时器类型，执行该类型相应的动作
  	if(tbg_timer_manage.tbg_timer_info->timer_type == 1 )//表示TTL_TIMER定时器到时
	{
		TBG_TTL_TIMER_launch=1;
	}
	else if(tbg_timer_manage.tbg_timer_info->timer_type == 3 )//表示S_TIMER定时器到时
	{
		TBG_S_TIMER_launch=1;
	}
  
	//删除到时的子定时器条目
	tbg_timer_manage.tbg_timer_info = tbg_del_a_timer(tbg_timer_manage.tbg_timer_info, num);
	
	if(tbg_timer_manage.tbg_timer_info == NULL)
	{
		return 1;
	}

	return 0;
}
//插入子定时器
int tbg_set_child_timer(long delay, int id, int type, int tunnel_id){
	struct tbg_timer_info* tbg_timer_info;
	time_t t = time(NULL);
	tbg_timer_info = (struct tbg_timer_info*)malloc(sizeof(struct tbg_timer_info));
	tbg_timer_info->timer_type = type;//hu
	tbg_timer_info->TBG_TIMER_ID = id;
	tbg_timer_info->time = delay + t; //定时时长+此时时刻=到时时刻
	tbg_timer_info->tunnel_id = tunnel_id;
	tbg_timer_info->timer_proc = tbg_timer_proc_launch;	
	tbg_timer_manage.tbg_timer_info = tbg_insert_a_timer(tbg_timer_manage.tbg_timer_info, tbg_timer_info);
}
int tbg_timer_unlaunch = 1; //定时器初始化
//***************************定时器函数_end***********************

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
//**************************用于建立隧道列表的各类子函数*************************************
//************************ *隧道列表函数集*****************************
struct tbg_tunnel_list_info
{
	int tunnel_entry;//隧道条目编号
	int tunnel_id; //隧道编号
	IPAddress  sour_ip; //sour_ip
	uint8_t  TAG_NID[16];//TAG_NID
	int position;// 位置：1（同域）/0（异域）
	struct tbg_tunnel_list_info *next;	
};
struct tbg_tunnel_list_info *TBG_TUNNEL_LIST; //表头结点。此时的TUNNEL_LIST只是一个空指针，未指向任何地址
//打印当前所有的PID列表
void print_tbg_tunnel_list_all(struct tbg_tunnel_list_info* head)
{
	struct tbg_tunnel_list_info* p;
	p=head;
	int i=1;
	while(p!=NULL)
	{ 
		if (i ==1)
		{
			printf("--------------------------TBG TUNNEL TABLE---------------------------\n");
			printf("|tunnel_entry  | tunnel_id | sour_ip           | TAG_NID | position |\n" );
			printf("---------------------------------------------------------------------\n");
			i=0;
		}
		printf("|      %d       |    %d      | %s      |   %s  |    %d     |\n",p->tunnel_entry,p->tunnel_id,p->sour_ip.unparse().c_str(),p->TAG_NID,p->position);
		printf("---------------------------------------------------------------------\n");
		p=p->next;
	}
}
//打印指定结点的隧道关系表
void print_tbg_tunnel_list_single(struct tbg_tunnel_list_info* p)
{
	int i =1;
	if (i ==1)
	{
		printf("--------------------------TBG TUNNEL TABLE---------------------------\n");
		printf("|tunnel_entry  | tunnel_id | sour_ip           | TAG_NID | position |\n" );
		printf("---------------------------------------------------------------------\n");
		i=0;
	}
	printf("|      %d       |    %d      | %s      |   %s  |    %d     |\n",p->tunnel_entry,p->tunnel_id,p->sour_ip.unparse().c_str(),p->TAG_NID,p->position);
	printf("---------------------------------------------------------------------\n");
}
//查找匹配的"sour_ip“结点，查找成功返回找到的表结点指针，失败返回空指针
struct tbg_tunnel_list_info* match_sour_ip(struct tbg_tunnel_list_info *head, IPAddress sour_ip)
{
	printf("--->>> Into Function [match_sour_ip()]\n");
	struct tbg_tunnel_list_info *p;
	p=head;
	if (p ==NULL)
	{
		return NULL;
	}
	else
	{
		//匹配成功，说明条目已经存在，需要进一步匹配TAG_NID项
		while(p->next != NULL )//有多个结点时
		{
			//c++中string类的比较用compare。相等a.compare(b)=0;
			if (p->sour_ip.unparse().compare(sour_ip.unparse() )==0)//匹配表头或中间结点
			{
				printf("--->>> Result : sour_ip match successfully in [ tunnel %d ] !\n",p->tunnel_id);
				return p;
			}
			p=p->next;
		}
		if (p->sour_ip.unparse().compare(sour_ip.unparse()) ==0)//匹配表头或中间结点
		{
			printf("--->>> Result : sour_ip match successfully in [ tunnel %d ] !\n",p->tunnel_id);
			return p;
		}
		//匹配失败，说明没有该条目，则在表尾添加新的条目信息
		else
		{
			printf("--->>> Result : cannot match sour_ip!\n");
			return NULL;
		}
	}
}
//继sour_ip项匹配成功后，进一步匹配该条目的TAG_NID字段。成功表示不修改条目;失败表示要修改条目信息,返回需要修改的结点指针
struct tbg_tunnel_list_info* match_TAG_NID(struct tbg_tunnel_list_info *pmatched, uint8_t TAG_NID[16])
{
	printf("--->>> Into Function [match_TAG_NID()]\n");
	struct tbg_tunnel_list_info *p;
	p=pmatched; //已经成功匹配”sour_ip“项的条目指针
	if (strcmp((char*)p->TAG_NID, (char*)TAG_NID) ==0)//匹配该条目的TBG_NID字段
	{
		printf("--->>> Result : TAG_NID match successfully! [tunnel %d] don`t need change!\n",pmatched->tunnel_id);
		return NULL;
	}
	else
	{
		printf("--->>> Result : cannot match TAG_NID, [tunnel  %d] need change!\n", pmatched->tunnel_id);
		return p;
	}
}
//建立能够快速查找的哈希表     //哈希表的参数类型可以自己定义哦
HashMap<IPAddress,String >IP_NID;	//哈希表1 客户端IP对应隧道接入网关
HashMap<IPAddress,int >IP_Tunnel;      //哈希表2 客户端IP对应隧道编号
//插入新的隧道关系表结点
struct tbg_tunnel_list_info* insert_tbg_tunnel_list(struct tbg_tunnel_list_info *head, struct tbg_tunnel_list_info *pnew)
{
	struct tbg_tunnel_list_info *p;
	p=head;
	//表空，则直接在表头节点插入第一条隧道信息
	if (head == NULL)
	{
		printf("--->>> Into Function [insert_tbg_tunnel_list()]\n");
		printf("--->>> Empty table, insert the first tunnel_entry\n");
		pnew->tunnel_entry=1;//第一条隧道条目编号固定为1;
		head = pnew;
		pnew->next = NULL;
		//****添加哈希表1的条目 [sour_ip --- TAG_NID]*********
		char TAG_NID[16];
		memcpy(TAG_NID,pnew->TAG_NID,16);
		IP_NID.insert(pnew->sour_ip, TAG_NID);
		cout<<"--->>> HashMap IP_NID table_size = "<<IP_NID.size()<<endl;//打印出当前哈希表的大小		
		//****添加哈希表2的条目 [sour_ip --- tunnel_id]*********
		int tunnel_id = pnew->tunnel_id;
		IP_Tunnel.insert(pnew->sour_ip, tunnel_id);
		cout<<"--->>> HashMap IP_Tunnel table_size = "<<IP_NID.size()<<endl;//打印出当前哈希表的大小		
	}
	//表不空，则匹配“sour_ip”字段
	else
	{
		struct tbg_tunnel_list_info *pmatched;
		pmatched=match_sour_ip(TBG_TUNNEL_LIST, pnew->sour_ip);
		//匹配“sour_ip”字段失败，则在表尾添加新的隧道条目
		if (pmatched == NULL)
		{
			while(p->next != NULL)//找到最后一个表节点
			{
				p=p->next;
			}
			printf("--->>> Insert a new tunnel_entry\n");
			int tunnel_entry =p->tunnel_entry;
			pnew->tunnel_entry = ++tunnel_entry; //新增条目的编号为前一个结点+1
			p->next=pnew;	//将新的表结点插到表尾

			//****添加哈希表1的条目 [sour_ip --- TAG_NID]*********
			char TAG_NID[16];
			memcpy(TAG_NID,pnew->TAG_NID,16);
			IP_NID.insert(pnew->sour_ip, TAG_NID);
			cout<<"--->>> HashMap IP_NID table_size = "<<IP_NID.size()<<endl;//打印出当前哈希表的大小		
			//****添加哈希表2的条目 [sour_ip --- tunnel_id]*********
			int tunnel_id = pnew->tunnel_id;
			IP_Tunnel.insert(pnew->sour_ip, tunnel_id);
			cout<<"--->>> HashMap IP_Tunnel table_size = "<<IP_NID.size()<<endl;//打印出当前哈希表的大小			
		}
		//匹配“sour_ip”字段成功，则在进一步匹配该条目的TAG_NID字段
		else
		{	
			struct tbg_tunnel_list_info *pchanged;
			pchanged=match_TAG_NID(pmatched, pnew->TAG_NID);
			//该用户的接入网关信息已经发生改变，则修改TBG中的隧道列表信息
			if (pchanged != NULL)
			{
				printf("--->>> Change [tunnel_entry %d ] information\n", pchanged->tunnel_id);
				memcpy(pchanged->TAG_NID,pnew->TAG_NID,sizeof(pnew->TAG_NID));
				pchanged->tunnel_id = pnew->tunnel_id;
				pchanged->position = pnew->position;
				//更新了表中信息后，也要同步更新哈希表信息
				//****更新哈希表1 [sour_ip --- TAG_NID]*********
				char TAG_NID[16];
				memcpy(TAG_NID,pchanged->TAG_NID,16);
				IP_NID.insert(pchanged->sour_ip, TAG_NID);
				cout<<"--->>> HashMap IP_NID SIZE = "<<IP_NID.size()<<endl;//打印出当前哈希表的大小（应该是保持不变）	
				//****更新哈希表2项 [sour_ip --- tunnel_id]*********
				int tunnel_id = pnew->tunnel_id;
				IP_Tunnel.insert(pnew->sour_ip, tunnel_id);
				cout<<"--->>> HashMap IP_Tunnel table_size = "<<IP_NID.size()<<endl;//打印出当前哈希表的大小			
			}
		}
	}
	return head;
}

//设置新的tunnel_list列表参数并插入条目和打印
void set_tbg_tunnel_list(int tunnel_id, IPAddress sour_ip, uint8_t TAG_NID[16], int position)
{
	struct tbg_tunnel_list_info *tbg_tunnel_list; // 用来承载新条目信息
	tbg_tunnel_list=(struct tbg_tunnel_list_info *)malloc(sizeof(struct tbg_tunnel_list_info));//如果分配成功，返回指向被分配内存的指针
	tbg_tunnel_list->tunnel_id = tunnel_id;
	tbg_tunnel_list->sour_ip=sour_ip;
	memcpy( tbg_tunnel_list->TAG_NID,TAG_NID,16);
	tbg_tunnel_list->position = position;
	tbg_tunnel_list->next = NULL; //一定要把新节点的next赋值，不然会出错的
	TBG_TUNNEL_LIST=insert_tbg_tunnel_list(TBG_TUNNEL_LIST, tbg_tunnel_list);//返回表头指针
	//只有新增条目、更新条目时才打印隧道列表信息，不用每个数据包都打印
	//****打印隧道列表信息***************************
	char tstr[200];
	tbg_get_format_time(tstr);
	printf("[%s] Now all TBG_TUNNEL_LIST are as showed:\n", tstr);
	print_tbg_tunnel_list_all(TBG_TUNNEL_LIST);
}

//***************************数据模块各种定义**************************
/*#define IN_DEVICE          "h4-eth0" //程序的参数配置
#define DEVICELENGTH    10*/
//*************************各种定义*************************//
#define bufsize     1024 * 5
unsigned char RecvBuf[bufsize] = {0};
//用于数据包的分片整合
typedef struct _Gather Gather;
struct _Gather
{
	uint8_t flagRECV;
	uint8_t CachePKG[2][1514];
};
Gather List;
char TUNNEL_SID1[15]={'t','u','n','n','e','l','_','s','e','r','v','i','c','e','\0'};    //隧道包标志

//*******************************解封装处理 正向处理（合片）*******************************//
void TBG_CoLoR_Decapsulation::push(int port, Packet *packet)
{
	cout<<endl<<"--->>> void TBG_CoLoR_Decapsulation::push(int port, Packet *packet)"<<endl;
	unsigned char pkg[bufsize];
	memset(pkg,0,sizeof(pkg));
	CoLoR_NEW_DATA * pPKG;
	pPKG = (CoLoR_NEW_DATA *)pkg;
	int RecvLength=0;
	int pkglength=0;
	RecvLength=packet->length();
	memcpy(RecvBuf,packet->data(),packet->length());
	memcpy(pkg,RecvBuf,RecvLength);
	char test_sid[20]; //提取数据包的SID标识
	for (int i = 0; i < 20; ++i)
	test_sid[i]=pPKG->l_sid[i];
	//判断是更新包还是数据包
	if (!strcmp(TUNNEL_SID1,test_sid)) //判断为数据包，则合片
	{
		//第一分片
		if(pPKG->offset[0] == 1)
		{
			//*********建立或者更新隧道列表**************
			int tunnel_id = pPKG->reserved[1];
			IPAddress sour_ip;
			//提取IP数据包中的源IP地址 
			memcpy(&sour_ip,(unsigned char *)pPKG->data+14+12,4);//请求包里的源ip  //cout<<"sour_ip ="<<sour_ip.unparse().c_str()<<endl; 
			uint8_t TAG_NID[16];
			memcpy(TAG_NID,pPKG->nid+8,8); //cout<<"TAG_NID = "<<TAG_NID<<endl;
			int position = pPKG->reserved[0];
			set_tbg_tunnel_list(tunnel_id, sour_ip, TAG_NID, position);

			printf("--->>> [Divided = %d RECV Length] %d\n",pPKG->offset[0],RecvLength);
			if(List.flagRECV == 0)
			{
				List.flagRECV = 1;
				memcpy(List.CachePKG[0],pkg,RecvLength);
			}
			else
			{
				List.flagRECV = 0;
				memset(List.CachePKG,0,1514*2);
			}
		}
		else if(pPKG->offset[0] == 2) //第二片无法添加表项
		{
			printf("--->>> [Divided = %d RECV Length] %d\n",pPKG->offset[0],RecvLength);
			if(List.flagRECV == 1)
			{
				memcpy(List.CachePKG[1],pkg,RecvLength);

				pPKG = (CoLoR_NEW_DATA *)List.CachePKG[0];
				pkglength = pPKG->length[0]*256+pPKG->length[1];
				memcpy(pkg,pPKG->data,pkglength);
				
				pPKG = (CoLoR_NEW_DATA *)List.CachePKG[1];
				memcpy(pkg+pkglength,pPKG->data,pPKG->length[0]*256+pPKG->length[1]);
				pkglength+=pPKG->length[0]*256+pPKG->length[1];

				//发送转换好的数据包
				uint32_t addlength;//修改数据包长度
				addlength=pkglength-1114; //1114为整个data包长度
				packet->put(addlength);
				//printf("changedlength_IP=%d\n",packet->length());
				//发送数据包
				memcpy((char *)packet->data(),(const char *)pkg,pkglength);
				output(0).push(packet);
				//输出提示
				printf("--->>> TBG_CoLoR_Decapsulation IP packet sent out.(Gathered Pkg, length: %d)\n",pkglength);
			}
			List.flagRECV = 0;
			memset(List.CachePKG,0,1514*2);
		}
		else //将没有分片的包解封装
		{
			printf("--->>> [From TAG:RECV Length] %d\n",RecvLength);
			//*********建立或者更新隧道列表**************
			int tunnel_id = pPKG->reserved[1];
			IPAddress sour_ip;
			//提取IP数据包中的源IP地址 
			memcpy(&sour_ip,(unsigned char *)pPKG->data+14+12,4);//请求包里的源ip  //cout<<"sour_ip ="<<sour_ip.unparse().c_str()<<endl; 
			uint8_t TAG_NID[16];
			memcpy(TAG_NID,pPKG->nid+8,8); //cout<<"TAG_NID = "<<TAG_NID<<endl;
			int position = pPKG->reserved[0];
			set_tbg_tunnel_list(tunnel_id, sour_ip, TAG_NID, position);

			//***********解封装数据包********************
			pkglength = pPKG->length[0]*256+pPKG->length[1];
			memcpy(pkg,pPKG->data,pkglength);
			
			//***********发送转换好的数据包***************
			memcpy((char *)packet->data(),(const char *)pkg,pkglength);
			uint32_t takelength;
			takelength=1114-pkglength;
			packet->take(takelength);
			//printf("changedlength_IP=%d\n",packet->length());
			output(0).push(packet);
			//输出提示
			printf("--->>> TBG_CoLoR_Decapsulation IP packet sent out.(Non-Gathered Pkg, length: %d)\n",pkglength);
		}	
	}
	else //表明收到的是更新包或者是保活包
	{
		cout<<"--->>>This is a PID_updata or keepalive packet"<<endl;
		output(1).push(packet);//丢弃
	}	
}
//**************************封装 反向处理（分片）****************************************
//**********************程序的参数配置***********************
#define IN_DEVICE          "h4-eth1"
#define OUT_DEVICE         "h4-eth0"
#define DEVICELENGTH        10
/******************各种定义******************/
unsigned char RecvBuf_data1[bufsize] = {0};
unsigned char RecvBuf_data2[bufsize] = {0};
//获取本机网口的mac和ip
char in_device3[10];
char out_device3[10];
uint8_t in_mac3[7];
uint8_t out_mac3[7];
uint8_t in_ip3[5];
uint8_t out_ip3[5];
int GetLocalMac3 ( const char *device,uint8_t *mac,uint8_t *ip )
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
void TBG_CoLoR_Encapsulation::push(int port, Packet *packet)
{
	cout<<endl<<"--->>> void TBG_CoLoR_Encapsulation::push(int port, Packet *packet)"<<endl;
	//新的包格式下的包处理
	int Recvlength1=0;
	memcpy(RecvBuf_data1,packet->data(),packet->length());
	Recvlength1=packet->length();
	//cout<<"Recvlength ="<<Recvlength1<<"   ";
	CoLoR_NEW_DATA PKG;
	memset(&PKG,0,sizeof(CoLoR_NEW_DATA));//用来结构体清零

	//********************查找隧道列表****************************
	// 提取客户端的IP地址（目的IP），查找对应的隧道，并封装隧道头部信息
	IPAddress client_ip;
	memcpy(&client_ip,RecvBuf_data1+14+16,4);
	cout<<"--->>> client_ip ="<<client_ip.unparse().c_str()<<endl; 
	//查找哈希表1：查到TAG的NID
	String value;
 	value=IP_NID.find(client_ip);
 	char TAG_NID[16]; 
 	memcpy(TAG_NID,value.c_str(),16); 
 	cout<<"--->>> Find in hashmap1 TAG_NID ="<<TAG_NID<<endl;
 	//查找哈希表2：查到隧道编号
 	int tunnel_id;
 	tunnel_id=IP_Tunnel.find(client_ip);
 	cout<<"--->>> Find in hashmap2 tunnel_id ="<<tunnel_id<<endl;

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

	memset(PKG.reserved,0,sizeof(PKG.reserved));  //reserved[0]用于RM标识同域或者异域，reserved[1]用于存储隧道编号

	uint8_t n_sid[16]={'T','B','G','1',0,0,0,0,0,0,0,0,0,0,0,0};
	for(iii=0;iii<16;iii++)
	PKG.n_sid[iii]=n_sid[iii];

	uint8_t l_sid[20]={'t','u','n','n','e','l','_','s','e','r','v','i','c','e',0,0,0,0,0,0}; 
	for(iii=0;iii<20;iii++)
	PKG.l_sid[iii]=l_sid[iii];


	memcpy((char *)PKG.nid,TAG_NID,8); //目的NID，从隧道列表中获取
	memcpy((char *)(PKG.nid+8),"TBG1",4); //源NID

	//PID信息需要从PID列表中获取
	//memcpy(PKG.PID,pPKG_GET->PID,sizeof(pPKG_GET->PID));
	////t/cout<<PKG.PID<<endl;

	//**************填充数据字段和修改MAC地址***************************
	memcpy(out_device3,OUT_DEVICE,DEVICELENGTH);
	GetLocalMac3(out_device3, out_mac3, out_ip3);
    for(;;)
    {
	if(Recvlength1>1514)
		break;
	//如果转发包的长度大于1000（data字段的大小），即接近MTU值，为了方便封装隧道包头传输，需要切割单包
	else if(Recvlength1>1000) //根据data字段的大小来进行分片，只发送第一个分片
	{
		ftp_total_len_ip1+=Recvlength1;
		//因为CoLoR包封装的mac地址暂时没有作用，此处用于标识网口，故源和目的封装成为一样的mac对程序本身的功能是没有影响的。
		PKG.ether_dhost[0] = out_mac3[0];//目的h3-eth0
		PKG.ether_dhost[1] = out_mac3[1];
		PKG.ether_dhost[2] = out_mac3[2];
		PKG.ether_dhost[3] = out_mac3[3];
		PKG.ether_dhost[4] = out_mac3[4];
		PKG.ether_dhost[5] = out_mac3[5];
		PKG.ether_shost[0] = out_mac3[0];//源改自己
		PKG.ether_shost[1] = out_mac3[1];
		PKG.ether_shost[2] = out_mac3[2];
		PKG.ether_shost[3] = out_mac3[3];
		PKG.ether_shost[4] = out_mac3[4];
		PKG.ether_shost[5] = out_mac3[5];
		PKG.ether_type = 0x0008;
		PKG.offset[0] = 1;  //标志第一分片
		PKG.length[0] = Recvlength1/2/256;
		PKG.length[1] = Recvlength1/2%256;

		//**************发送第一个分片****************************
		memcpy(PKG.data,RecvBuf_data1,Recvlength1/2);//填充一半的IP包数据作为第一分片
		memcpy(RecvBuf_data1,&PKG,sizeof(CoLoR_NEW_DATA));//拷贝填充好包头的空data包
		memcpy((char *)packet->data(),(const char *)RecvBuf_data1,sizeof(CoLoR_NEW_DATA));
		//修改数据包的长度，不修改的话只能发出接收包长度的数据，数据不完全
		uint32_t takelength1;
		uint32_t addlength1;
		if(Recvlength1<1114)//1114为整个data包的长度
		{
			addlength1=1114-Recvlength1;
			packet->put(addlength1);
		}
		else
		{
			takelength1=Recvlength1-1114;
			packet->take(takelength1);
		}
		//printf("changedlength_CoLoR=%d\n",packet->length());
		output(0).push(packet);
		ftp_total_len_color1+=packet->length();
		//输出提示
		printf("--->>> TBG_CoLoR_Eecapsulation DATA packet sent out.(Divided Pkg, No.1, length: %d)\n",Recvlength1/2);
		
		//**************发送第二个分片****************************
		PKG.offset[0] = 2;   //标志位第二分片
		Packet *packet1;
		packet1 = Packet::make(sizeof(CoLoR_NEW_DATA)); //make 一个空包封装第二分片
		memcpy(PKG.data,RecvBuf_data1+Recvlength1/2,Recvlength1 - Recvlength1/2);//填充后一半的IP包数据作为第二分片
		memcpy(RecvBuf_data1,&PKG,sizeof(CoLoR_NEW_DATA));//拷贝填充好包头和数据的data包
		//printf("%d\n", sizeof(CoLoR_NEW_DATA));//应该是1114字节
		memcpy((char *)packet1->data(),(const char *)RecvBuf_data1,sizeof(CoLoR_NEW_DATA));
		//printf("changedlength_CoLoR=%d\t",packet1->length());
		output(0).push(packet1);
		ftp_total_len_color1+=packet1->length();
		//输出提示
		printf("--->>> TBG_CoLoR_Eecapsulation DATA packet sent out.(Divided Pkg, No.2, length: %d)\n",Recvlength1/2);
	}
	else//发送不分片的包
	{
		//ftp_total_len_ip1+=Recvlength1;//不分片包的总流量
		PKG.ether_dhost[0] = out_mac3[0];//目的h3-eth0
		PKG.ether_dhost[1] = out_mac3[1];
		PKG.ether_dhost[2] = out_mac3[2];
		PKG.ether_dhost[3] = out_mac3[3];
		PKG.ether_dhost[4] = out_mac3[4];
		PKG.ether_dhost[5] = out_mac3[5];
		PKG.ether_shost[0] = out_mac3[0];//源改自己
		PKG.ether_shost[1] = out_mac3[1];
		PKG.ether_shost[2] = out_mac3[2];
		PKG.ether_shost[3] = out_mac3[3];
		PKG.ether_shost[4] = out_mac3[4];
		PKG.ether_shost[5] = out_mac3[5];
		PKG.ether_type = 0x0008;
		PKG.offset[0] = 0;
		PKG.length[0] = Recvlength1/256; 
		PKG.length[1] = Recvlength1%256;
	
		//****************填充数据********************//
		memcpy(PKG.data,RecvBuf_data1,Recvlength1); //将整个IP包封装进data包的数据部分
		memcpy(RecvBuf_data1,&PKG,sizeof(CoLoR_NEW_DATA));
		memcpy((char *)packet->data(),(const char *)RecvBuf_data1,sizeof(CoLoR_NEW_DATA));

		//修改数据包的长度，不修改的话只能发出接收包长度的数据，数据不完全
		uint32_t addlength;
		addlength=1114-Recvlength1;
		packet->put(addlength);
		//printf("changedlength_CoLoR=%d\n",packet->length());
		output(0).push(packet);
		ftp_total_len_color1+=packet->length();
		//输出提示
		printf("--->>> TBG_CoLoR_Eecapsulation DATA packet sent out.(None Divided Pkg, length: %d)\n",Recvlength1);
	}
	/*
	//从服务器到TBG的入口流量统计
	cout<<"TBG : ftp_total_len_ip = "<<ftp_total_len_ip1<<endl;
	//从TBG发向TAG的总流量
	cout<<"TBG : ftp_total_len_color1 = "<<ftp_total_len_color1<<endl;
	*/
	break;
       }		
}

//************************PID更新模块************************************************
//****************用于建立隧道列表的各类子函数*****************************************
//新的PID列表，加上TBG标识
int TBG_TIMER_NUMBER=0; //PID列表条目编号
struct tbg_pidlist_info
{
	int pidlist_entry;//PID列表条目编号
	int tunnel_num; //隧道编号
	uint8_t  TAG_NID[16]; //TBG的NID
	int pid_num;//PID的数量
	uint8_t PID[4*5];// 到达该TAG的PID
	uint16_t TTL;
	uint16_t Frequence;	
	int tunnel_state;//隧道的开关状态	
	int PKG_RECV;//在TBG的PID列表中没有start_state标志位，而是换成了PKG_RECV标志位,插入S计时器时赋值为0，收到更新包时赋值为1;
	int update_state;//记录该条目更新状态，由S_timer控制
	struct tbg_pidlist_info *next;	
};
struct tbg_pidlist_info *TBG_PIDLIST; //此时的PIDLIST只是一个空指针，未指向任何地址
struct tbg_pidlist_info  tbg_pidlist_data;//用来存储包中的数据
//插入新的PID表结点
struct tbg_pidlist_info* tbg_insert_pidlist(struct tbg_pidlist_info *head, struct tbg_pidlist_info *pnew)
{
	struct tbg_pidlist_info *p;
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
void tbg_print_pidlist_manage(struct tbg_pidlist_info* head)
{
	struct tbg_pidlist_info* p;
	p=head;
	int i =1;
	while(p!=NULL)
	{
		if (i ==1)
		{
			printf("-------------------------------------------------PID TABLE---------------------------------------------------\n");
			printf("|pidlist_entry | tunnel_id | TAG_NID | pid_num |  PIDs  | TTL  |  F  | tunnel_state | PKG_RECV| update_state|\n" );
			printf("-------------------------------------------------------------------------------------------------------------\n");
			i=0;
		}
		printf("|       %d      |    %d      |   %s  |    %d    |  %s  | %d   |  %d  |      %d       |     %d   |      %d      |\n",p->pidlist_entry,p->tunnel_num,p->TAG_NID,p->pid_num,p->PID,p->TTL,p->Frequence,p->tunnel_state,p->PKG_RECV,p->update_state );
		p=p->next;
	}
printf("-------------------------------------------------------------------------------------------------------------\n");
}
//打印指定结点的PID列表
void tbg_print_pidlist_single(struct tbg_pidlist_info* p)
{
	int i =1;
	if (i ==1)
	{	
		printf("-------------------------------------------------PID TABLE---------------------------------------------------\n");
		printf("|pidlist_entry | tunnel_id | TAG_NID | pid_num |  PIDs  | TTL  |  F  | tunnel_state | PKG_RECV| update_state|\n" );
		printf("-------------------------------------------------------------------------------------------------------------\n");
		i=0;
	}
	printf("|       %d      |    %d      |   %s  |    %d    |  %s  | %d   |  %d  |      %d       |     %d   |      %d      |\n",p->pidlist_entry,p->tunnel_num,p->TAG_NID,p->pid_num,p->PID,p->TTL,p->Frequence,p->tunnel_state,p->PKG_RECV,p->update_state );
printf("-------------------------------------------------------------------------------------------------------------\n");
	
}
//设置新的PID列表参数并插入和打印
void tbg_set_pidlist(int tunnel_num , uint8_t nid[16], int pid_num, uint8_t PID[4*5], uint16_t TTL, uint16_t F)
{
	struct tbg_pidlist_info *pidlist;
	pidlist=(struct tbg_pidlist_info *)malloc(sizeof(struct tbg_pidlist_info));
	pidlist->pidlist_entry = ++TBG_TIMER_NUMBER;//每新添加一条条目，条目编号顺序加1
	printf("--->>> insert a new [ pidlist_entry %d ] \n",pidlist->pidlist_entry );
	pidlist->tunnel_num = tunnel_num;
	memcpy( pidlist->TAG_NID,nid,16);
	pidlist->pid_num = pid_num;
	memcpy( pidlist->PID,PID,4*5);
	pidlist->TTL = TTL;
	pidlist->Frequence = F;
	pidlist->tunnel_state =1;
	pidlist->PKG_RECV =0;//建表时会插入S计时器，此时将收包标志位置为0;
	pidlist->update_state=1;
	pidlist->next = NULL;
	TBG_PIDLIST=tbg_insert_pidlist(TBG_PIDLIST, pidlist);
	tbg_print_pidlist_manage(TBG_PIDLIST);
}	
/*可以能根据TBG来查找PID列表结点，虽然不同隧道的隧道可能适配到同一TBG，也就是说会出现
除了隧道标号和pidlist条目编号外其他内容完全相同的多个条目。PID的更新过程是基于端点的，因此
这些条目统一进行更新即可。
总结为：PID列表的建立依据于隧道列表，PID列表的更新依据于条目的TBG
*/
//查找对应TBG的PID列表结点，返回找到的表结点指针
struct tbg_pidlist_info* tbg_find_pidlist_nid(struct tbg_pidlist_info *head, uint8_t nid[16])
{
	printf("--->>> Into Function [tbg_find_pidlist_nid()]\n");
	printf("--->>> find_TBG_NID= %s\n", nid);
	struct tbg_pidlist_info *p;
	p=head;
	while(p->next != NULL )
	{
		if (strcmp((char*)p->TAG_NID, (char*)nid) ==0)//匹配表头和中间结点
		{
			printf("--->>> Find pidlist_entry!\n");
			tbg_print_pidlist_single(p);
			return p;
		}
		p=p->next;
	}
	if (strcmp((char*)p->TAG_NID, (char*)nid) ==0)//匹配最后一个结点
	{
		printf("--->>> Find pidlist_entry!\n");
		tbg_print_pidlist_single(p);
		return p;
	}
}
struct tbg_pidlist_info* tbg_find_pidlist_tunnelid(struct tbg_pidlist_info *head, int tunnel_num)
{
	printf("--->>> Into Function [tbg_find_pidlist_tunnelid()]\n");
	//printf("--->>> tbg_launch_tunnel_id = %d\n", tbg_launch_tunnel_id);
	struct tbg_pidlist_info *p;
	p=head;
	if (head == NULL ) // 表空时返回空指针
	{
		cout<<"--->>> NOW PIDLIST is NULL  !"<<endl;
		return NULL;
	}
	else
	{
		while(p->next != NULL )
		{
			if (p->tunnel_num == tunnel_num)//匹配表头和中间结点
			{
				printf("--->>> Find in [ pidlist_entry %d ] !\n",p->pidlist_entry);
				//tbg_print_pidlist_single(p);
				return p;
			}
			p=p->next;
		}
		if (p->tunnel_num == tunnel_num)//匹配最后一个结点
		{
			printf("--->>> Find in [ pidlist_entry %d ] !\n",p->pidlist_entry);
			//tbg_print_pidlist_single(p);
			return p;
		}
		else
		{
			cout<<"--->>> Cannot find pidlist_entry!"<<endl;
			return NULL;
		}
	}
}	

//***************************处理模块**********************************
int j=1;
char UPDATE_SID1[10]={'p','i','d','u','p','d','a','t','e','\0'};    //隧道包标志
char TUNNEL_DOWN1[14]={'p','i','d','u','p','d','a','t','e','d','o','w','n','\0'}; //隧道关闭通告包
void Updatepid_TBG_Process::push(int port, Packet *packet)
{
	cout<<endl<<"--->>> Module: void Updatepid_TBG_Process::push(int port, Packet *packet) "<<endl;
	//***********初始化定时器*************************
	if(tbg_timer_unlaunch){
		memset(&tbg_timer_manage, 0, sizeof(struct tbg_timer_manage));
		tbg_init_mul_timer(&tbg_timer_manage);//初始化定时器
		tbg_timer_unlaunch = 0;
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
	//*******接收到的数据包信息******************************
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_GET* pPKG_GET;//为什么要用结构体指针，利用结构体指针来取数据包的每一个字段
	pPKG_GET=(CoLoR_NEW_GET*)RecvBuf;
	
	//为了取出”pidupdate“
	char update_sid1[10];
	for (int i = 0; i < 9; ++i)
	update_sid1[i]=pPKG_GET->l_sid[i];update_sid1[9]='\0';
	//cout << "--->>> update_sid1 = "<< update_sid1<<endl;
	
	//为了取出“pidupdatedown”
	char tunnel_down1[14];
	for (int i = 0; i < 13; ++i)
	tunnel_down1[i]=pPKG_GET->l_sid[i];tunnel_down1[13]='\0';
	//cout << "--->>> tunnel_down1 = "<< tunnel_down1<<endl;
	
	//为pid更新包，在隧道没有关闭的情况下，进行PID列表的更新
	if (!strcmp(UPDATE_SID1,update_sid1))
	{
		cout<<"--->>> RECV get_update packet num = "<<j<<endl;;
		j++;
		if (strcmp(TUNNEL_DOWN1,tunnel_down1) == 0) //隧道暂时关闭
		{
			printf("--->>> [ tunnel %d ] has been turned off ! \n", pPKG_GET->reserved[1] );
			//更改PID列表中的隧道状态标志位
			int tunnel_num = pPKG_GET->reserved[1];
			struct tbg_pidlist_info *p;
			p=tbg_find_pidlist_tunnelid(TBG_PIDLIST,tunnel_num);
			p->tunnel_state = 0;
			//p->PKG_RECV =0; //隧道关闭时所有标志位都置否（不然收到包时应该置1）
			//打印该条目情况
			tbg_print_pidlist_single(p);
		}
		else //隧道开启
		{
			//依据隧道编号，查找对应的pidlist_entry条目
			struct tbg_pidlist_info *p_find;
			int tunnel_id = pPKG_GET->reserved[1];
			p_find = tbg_find_pidlist_tunnelid(TBG_PIDLIST,tunnel_id); // 根据隧道编号查找PID列表
			//修改条目的收包标志位
			if ( p_find != NULL )
			{
				p_find->PKG_RECV =1;
				tbg_print_pidlist_single(p_find);//打印修改标志位后的条目
				//cout<<"--->>> p_find->PKG_RECV ="<<p_find->PKG_RECV <<endl;
			}		
			//******************提取更新包（GET）信息*****************************
			int n;int c[8];
			n=pPKG_GET->pids_o;
			c[0]=n/128;n%=128; //GET包的pid_o字段前7个字节为pid数量，后一位为偏移字段
			c[1]=n/64;n%=64;
			c[2]=n/32;n%=32;
			c[3]=n/16;n%=16;
			c[4]=n/8;n%=8;
			c[5]=n/4;n%=4;
			c[6]=n/2;n%=2;
			c[7]=n;//for(iii=0;iii<=7;iii++) cout<<c[iii];
			//填充待插入的PID列表数据
			///*PID列表编号*/tbg_pidlist_data.pidlist_entry =++TBG_TIMER_NUMBER;
			/*隧道编号*/tbg_pidlist_data.tunnel_num=pPKG_GET->reserved[1];
			/*边界网关*/memcpy(tbg_pidlist_data.TAG_NID,pPKG_GET->nid+8,8);
			/*pid的数量*/tbg_pidlist_data.pid_num=c[0]*64+c[1]*32+c[2]*16+c[3]*8+c[4]*4+c[5]*2+c[6]*1; //GET包7位是PID数量，后一位是offset字段
			/*PIDs*/memcpy(tbg_pidlist_data.PID,pPKG_GET->PID,sizeof(pPKG_GET->PID));
			/*TTL*/tbg_pidlist_data.TTL=2*pPKG_GET->minpid;
			/*Frequence*/tbg_pidlist_data.Frequence=pPKG_GET->minpid-2;
			/*start_update参数： 在tbg_set_pidlsit()函数中统一设置为1,与TAG中不同，因为收到更新包就表明更新是开启的*/
			/*tunnel_state: 在tbg_set_pidlist()函数中统一设置为1*/
			/*update_state: 在tbg_set_pidlist()函数中统一设置为1*/

			//*******************建立、添加、更新、修改PID列表***********************************************
			//如果PID列表为空，则建立第一个结点（PID列表条目）
			if (TBG_PIDLIST == NULL)
			{
				//插入新的PID列表
				tbg_set_pidlist(tbg_pidlist_data.tunnel_num, tbg_pidlist_data.TAG_NID, tbg_pidlist_data.pid_num, tbg_pidlist_data.PID, tbg_pidlist_data.TTL, tbg_pidlist_data.Frequence);
				//计时器参数
				TBG_TTL=tbg_pidlist_data.TTL;
				//插入TTL+STATE 计时器
				char tstr[200];
				tbg_get_format_time(tstr);//获取当前时间
				printf("--->>> %s: For [ PIDLSIT %d ] insert a new TTL_TIMER and STATE_TIMER \n", tstr,tbg_pidlist_data.pidlist_entry);
				struct _timer_info* timer_info1;
				struct _timer_info* timer_info2;
				uint16_t time1 = TBG_TTL;//插入新的TTL定时器
				uint16_t time2 = tbg_pidlist_data.Frequence+1;//为了监测该条目的更新状态，同时插入更新状态计时器
				if (TBG_TIMER_ID == 100)TBG_TIMER_ID=0; //将定时器编号重置为0
				tbg_set_child_timer(time1, ++TBG_TIMER_ID, TTL_TIMER_TYPE, tunnel_id);//插入子定时器
				tbg_set_child_timer(time2, ++TBG_TIMER_ID, PID_STATE_TIMER_TYPE, tunnel_id);//插入子定时器
				tbg_print_timer_manage(tbg_timer_manage.tbg_timer_info);//打印定时时间表
				TBG_TTL_TIMER_launch=0;
				TBG_S_TIMER_launch=0;
			}
			//表不空，则建立新条目或者修改已有对应条目（根据TAG_NID查找是否有对应的PID列表条目）
			else
			{	
				if (p_find == NULL) //没有找到，需要新建条目
				{
					//插入新的PID列表
					tbg_set_pidlist(tbg_pidlist_data.tunnel_num,tbg_pidlist_data.TAG_NID,tbg_pidlist_data.pid_num,tbg_pidlist_data.PID,tbg_pidlist_data.TTL,tbg_pidlist_data.Frequence);
					//计时器参数
					TBG_TTL=tbg_pidlist_data.TTL;
					//插入TTL+STATE 计时器
					char tstr[200];
					tbg_get_format_time(tstr);//获取当前时间
					printf("--->>> %s: For [ PIDLSIT %d ] insert a new TTL_TIMER and STATE_TIMER \n", tstr,tbg_pidlist_data.pidlist_entry);
					struct _timer_info* timer_info1;
					struct _timer_info* timer_info2;
					uint16_t time1 = TBG_TTL;//插入新的TTL定时器
					uint16_t time2 = tbg_pidlist_data.Frequence+1;//为了监测该条目的更新状态，同时插入更新状态计时器
					if (TBG_TIMER_ID == 100)TBG_TIMER_ID=0; //将定时器编号重置为0
					tbg_set_child_timer(time1, ++TBG_TIMER_ID, TTL_TIMER_TYPE, tunnel_id);//插入子定时器
					tbg_set_child_timer(time2, ++TBG_TIMER_ID, PID_STATE_TIMER_TYPE, tunnel_id);//插入子定时器
					tbg_print_timer_manage(tbg_timer_manage.tbg_timer_info);//打印定时时间表
					TBG_TTL_TIMER_launch=0;
					TBG_S_TIMER_launch=0;
				}
				else //找到对应条目，则根据PID列表的更新规则进行更新
				{
					//更新条件：pid列表条目中任一项发生变化都需要进行更新
					if ((strcmp((char*)p_find->TAG_NID,(char*)tbg_pidlist_data.TAG_NID)!=0/*TBG_NID发生变化*/)||(strcmp((char*)p_find->PID,(char*)tbg_pidlist_data.PID)!=0/*PID发生变化*/)||(p_find->pid_num != tbg_pidlist_data.pid_num)/*PID数量发生表化*/||(p_find->Frequence != tbg_pidlist_data.Frequence)/*RM中PID最小更新周期*/)
					{
						printf("--->>> Update [pidlist_entry %d]\n", p_find->pidlist_entry );
						//更新方法：在原条目上进行修改，用新数据代替旧数据
						memcpy( p_find->TAG_NID,tbg_pidlist_data.TAG_NID,16);
						p_find->pid_num = tbg_pidlist_data.pid_num;
						memcpy( p_find->PID,tbg_pidlist_data.PID,4*5);
						p_find->TTL = tbg_pidlist_data.TTL;
						p_find->Frequence = tbg_pidlist_data.Frequence;
						//修改条目的收包标志位
						//p_find->PKG_RECV ==1; // 放在最初修改效果更好
					}
					else
						//修改条目的收包标志位
						//p_find->PKG_RECV ==1;
						printf("--->>> [pidlist_entry %d] keeps no change\n", p_find->pidlist_entry );
				}
			}
			//**************************回送data更新包******************************************
			int PKG_NUM=pPKG_GET->res;
			//******************修改data包长度***********************
			int pkglength;
			pkglength = packet->length();
			uint32_t addlength;
			addlength=134-pkglength;//Data长度14+80+40=134字节
			packet->put(addlength);
			CoLoR_NEW_DATA PKG;
			memset(&PKG,0,sizeof(CoLoR_NEW_DATA));

			//*******************封装data包: PID字段、data字段、minpid字段*************************
			//填充mac层地址
			PKG.ether_dhost[0] = 0xAA;//目的
			PKG.ether_dhost[1] = 0xAA;
			PKG.ether_dhost[2] = 0xAA;
			PKG.ether_dhost[3] = 0xAA;
			PKG.ether_dhost[4] = 0xAA;
			PKG.ether_dhost[5] = 0xAA;
			PKG.ether_shost[0] = 0xBB;//源
			PKG.ether_shost[1] = 0xBB;
			PKG.ether_shost[2] = 0xBB;
			PKG.ether_shost[3] = 0xBB;
			PKG.ether_shost[4] = 0xBB;
			PKG.ether_shost[5] = 0xBB;
			PKG.ether_type = 0x0008;	

			PKG.version_type=161;//10100001
			PKG.ttl=255;
			PKG.total_len=134;

			PKG.port_no1=1;
			PKG.port_no2=2;

			PKG.minpid=pPKG_GET->minpid; //最小的PID更新时间也需要同步

			//PID数量字段应该从PID列表中提取，DATA包中PID的数量占前5位
			int num;int f[8];
			num=tbg_pidlist_data.pid_num;
			f[0]=num/16;num%=16;
			f[1]=num/8;num%=8;
			f[2]=num/4;num%=4;
			f[3]=num/2;num%=2;
			f[4]=num;
			f[5]=1;f[6]=0;f[7]=0;
			//for(int iii=0;iii<=7;iii++) cout<<f[iii];
			//整合后3位
			num=f[0]*128+f[1]*64+f[2]*32+f[3]*16+f[4]*8+f[5]*4+f[6]*2+f[7]*1;
			//cout<<"num = "<<num<<endl;
			PKG.pids_o= num;//00001100,PID数量占5位，其他位o=1,c=0,n=0;
			//printf("--->>> (data_packet )PKG.pids_o = %d\n", PKG.pids_o);

			PKG.res=PKG_NUM;
			printf("--->>> PKG.res= PKG_NUM=%d\n", (int*)PKG.res);

			int iii;
			uint8_t offset[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.offset[iii]=offset[iii];

			uint8_t length[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.length[iii]=length[iii];

			PKG.next_header=0;
			PKG.checksum=123;

			//填充同异域、隧道编号字段
			memset(PKG.reserved,0,sizeof(PKG.reserved));
			PKG.reserved[0]=0;
			PKG.reserved[1]=pPKG_GET->reserved[1];

			//memcpy((char *)PKG.n_sid,p_find->TAG_NID,16);//直接从更新包中提取
			memcpy((char *)PKG.n_sid,pPKG_GET->nid+8,8);//直接从更新包中提取TAG的NID
			
			uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e','\0',0,0,0,0,0,0,0,0,0,0};
			for(iii=0;iii<20;iii++)
			PKG.l_sid[iii]=l_sid[iii];

			memcpy((char *)PKG.nid,pPKG_GET->nid+8,8);//直接从更新包中提取TAG的NID
			memcpy((char *)(PKG.nid+8),"TBG1",4); //源NID ，直接进行封装

			memcpy(PKG.PID,pPKG_GET->PID,sizeof(pPKG_GET->PID));
			memcpy(PKG.data,pPKG_GET->PID,sizeof(pPKG_GET->PID));
			//cout<<PKG.PID<<endl;
			//cout<<PKG.data<<endl;

			//****************************发送data包**********************************
			memcpy((unsigned char *)packet->data(),&PKG,sizeof(CoLoR_NEW_DATA));
			output(0).push(packet);
			printf("--->>> [ tunnel %d] Update data packet has sent out\n", tbg_pidlist_data.tunnel_num);
		}
	}
	else
	{
		output(1).push(packet);//将收到的数据包丢弃
	}

}
//*******************************发送模块****************************
void Updatepid_TBG_Send::push(int port, Packet *packet) //每秒检测计时器的到时状况
{	
	if (TBG_S_TIMER_launch == 1) //状态计时器到时，为了维护Update_state状态标志位
	{
		cout<<endl<<"--->>> Module: void Updatepid_TBG_Send::push(int port, Packet *packet)  ---- S_TIMER launched!"<<endl;
		//根据到时定时器的隧道编号，查找对应的PID列表条目
		struct tbg_pidlist_info *p;
		p=tbg_find_pidlist_tunnelid(TBG_PIDLIST,tbg_launch_tunnel_id);
		if (p->PKG_RECV ==1)
		{
			p->update_state = 1; //状态周期内收到了更新包，表明更新过程正常
		}
		else
		{
			p->update_state = 0; //状态周期内没有收到更新包，更新过程出现异常
		}
		printf("--->>> [ tunnel %d ] PKG_RECV = %d, Set update_state = %d \n",p->tunnel_num,p->PKG_RECV,p->update_state);
		//为该条目插入新的S计时器
		char tstr[200];
		tbg_get_format_time(tstr);//获取当前时间
		printf("\n--->>> %s: For [ PIDLSIT %d ] insert  a new STATE_TIMER \n", tstr,p->pidlist_entry);
		struct _timer_info* timer_info1;
		uint16_t time1 = tbg_pidlist_data.Frequence+1;//为了监测该条目的更新状态，同时插入更新状态计时器
		if (TBG_TIMER_ID == 100)TBG_TIMER_ID=0; //将定时器编号重置为0
		tbg_set_child_timer(time1, ++TBG_TIMER_ID, PID_STATE_TIMER_TYPE, p->tunnel_num);//插入子定时器
		tbg_print_timer_manage(tbg_timer_manage.tbg_timer_info);//打印定时时间表
		TBG_S_TIMER_launch=0;
		//修改该条目收包辅助标志位
		p->PKG_RECV=0;
	}
	//***********************发送紧急更新请求包********************
	if (TBG_TTL_TIMER_launch == 1)
	{
		cout<<endl<<"--->>> Module: void Updatepid_TBG_Send::push(int port, Packet *packet)  --- TTL_TMER launched!"<<endl;
		//根据到时定时器的隧道编号，查找对应的PID列表条目
		struct tbg_pidlist_info *p;
		p=tbg_find_pidlist_tunnelid(TBG_PIDLIST,tbg_launch_tunnel_id);
		if (p->update_state == 0)//更新异常，发送紧急更新包
		{
			printf("--->>> [ tunnel %d ] update_state = %d , unormal！\n",  p->tunnel_num, p->update_state);
			//******************修改包长度***********************
			int pkglength;                       
			pkglength = packet->length();
			uint32_t addlength;
			addlength=126-pkglength;
			packet->put(addlength);

			//********************封装get包**************************
			char RecvBuf_GET[1024];
			memcpy(RecvBuf_GET,packet->data(),packet->length());
			struct CoLoR_NEW_GET PKG;
			memset(&PKG,0,sizeof(CoLoR_NEW_GET));
			
			//封装mac层头部
			PKG.ether_dhost[0] = 0xAA;//目的地址
			PKG.ether_dhost[1] = 0xAA;
			PKG.ether_dhost[2] = 0xAA;
			PKG.ether_dhost[3] = 0xAA;
			PKG.ether_dhost[4] = 0xAA;
			PKG.ether_dhost[5] = 0xAA;
			PKG.ether_shost[0] = 0xBB;//源地址
			PKG.ether_shost[1] = 0xBB;
			PKG.ether_shost[2] = 0xBB;
			PKG.ether_shost[3] = 0xBB;
			PKG.ether_shost[4] = 0xBB;
			PKG.ether_shost[5] = 0xBB;
			PKG.ether_type = 0x0008;	

			PKG.version_type=160;//10100000
			PKG.ttl=255;
			PKG.total_len=126;
			//cout<<PKG.total_len<<endl;

			PKG.port_no1=1;
			PKG.port_no2=2;

			//PID列表条目无效，无法获取，设为0;
			PKG.minpid=0; 
			PKG.pids_o=0;//pid数量占7位，offset字段占一位
			PKG.res=0; //紧急更新包的编号设为0;

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

			memcpy((char *)PKG.n_sid,p->TAG_NID,16);//从PID列表中提取

			uint8_t l_sid[20]={'p','i','d','u','p','d','a','t','e',0,0,0,0,0,0,0,0,0,0,0};
			for(iii=0;iii<20;iii++)
			PKG.l_sid[iii]=l_sid[iii];

			memcpy((char *)PKG.nid,p->TAG_NID,8); //目的NID, 从PID列表中提取
			memcpy((char *)(PKG.nid+8),"TBG1",4); //源NID ，直接进行封装

			uint8_t content_Ch[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.content_Ch[iii]=content_Ch[iii];

			uint8_t publickey[4]={255,255,255,255};
			for(iii=0;iii<4;iii++)
			PKG.publickey[iii]=publickey[iii];

			memset(PKG.PID,0,sizeof(PKG.PID));

			//****************************发送Emergency_get包*********************************
			memcpy((unsigned char *)packet->data(),&PKG,126);
			output(0).push(packet);
			printf("--->>>Emergency : PID  is unavailable， [ pidlist_entry %d] PID update packet sent out!\n",p->pidlist_entry);		
		}
		else // 隧道更新情况正常，PID列表信息可用
		{
			char tstr[200];
			tbg_get_format_time(tstr);//获取当前时间
			printf("--->>> [ tunnel %d ] update_state = %d , PIDLIST is avaliable！\n",  p->tunnel_num, p->update_state);
			printf("\n--->>> %s: For [ PIDLSIT %d ] insert a new TTL_TIMER \n", tstr,tbg_pidlist_data.pidlist_entry);
			struct _timer_info* timer_info1;
			uint16_t time1 = p->TTL;//插入PID列表中的TTL定时器
			if (TBG_TIMER_ID == 100)TBG_TIMER_ID=0; //将定时器编号重置为0
			tbg_set_child_timer(time1, ++TBG_TIMER_ID, TTL_TIMER_TYPE, p->tunnel_num);//插入子定时器
			tbg_print_timer_manage(tbg_timer_manage.tbg_timer_info);//打印定时时间表
			TBG_TTL_TIMER_launch=0;
		}
	}
	else
	{
		output(1).push(packet);
	}      
}
//************************隧道维护模块************************************************
char MAINT_SID1[13]={'t','u','n','n','e','l','_','m','a','i','n','t','\0'};//隧道维护包标志
//收到对方的保活包，立即应答
void TBG_MAINT::push(int port, Packet *packet)
{
	//***********接收对方的保活包****************************
	char RecvBuf[1024*5];
	memset(RecvBuf, 0, 1024*5);
	memcpy(RecvBuf,packet->data(),packet->length());
	CoLoR_NEW_DATA* pPKG_DATA;//为什么要用结构体指针，利用结构体指针来取数据包的每一个字段
	pPKG_DATA=(CoLoR_NEW_DATA*)RecvBuf;
	//*********取出应答包中SID标识进行包服务判断**********
	char test_sid[13];//为了取出“tunnel_maint”
	for (int i = 0; i < 12; ++i) test_sid[i]=pPKG_DATA->l_sid[i]; test_sid[12]='\0';
	//对SID进行判断
	if (!strcmp(MAINT_SID1,test_sid)/*为隧道维护包*/)
	{
		//***********封装应答包*********************
		//填充mac层地址
		pPKG_DATA->ether_dhost[0] = 0xAA;//目的
		pPKG_DATA->ether_dhost[1] = 0xAA;
		pPKG_DATA->ether_dhost[2] = 0xAA;
		pPKG_DATA->ether_dhost[3] = 0xAA;
		pPKG_DATA->ether_dhost[4] = 0xAA;
		pPKG_DATA->ether_dhost[5] = 0xAA;
		pPKG_DATA->ether_shost[0] = 0xBB;//源
		pPKG_DATA->ether_shost[1] = 0xBB;
		pPKG_DATA->ether_shost[2] = 0xBB;
		pPKG_DATA->ether_shost[3] = 0xBB;
		pPKG_DATA->ether_shost[4] = 0xBB;
		pPKG_DATA->ether_shost[5] = 0xBB;
		pPKG_DATA->ether_type = 0x0008;

		//从PID列表中提取
		memcpy(pPKG_DATA->n_sid,tbg_pidlist_data.TAG_NID,16);

		//目的NID, 从PID列表中提取
		memcpy(pPKG_DATA->nid,tbg_pidlist_data.TAG_NID,8); 
		memcpy(pPKG_DATA->nid+8,"TBG1",4); //源NID ，直接进行封装

		//PID和数据字段由PID列表获取
		memcpy(pPKG_DATA->PID,tbg_pidlist_data.PID,sizeof(tbg_pidlist_data.PID));
		memcpy(pPKG_DATA->data,tbg_pidlist_data.PID,sizeof(tbg_pidlist_data.PID));

		//******************发送保活应答包**********************************
		memcpy((unsigned char *)packet->data(),pPKG_DATA,sizeof(CoLoR_NEW_DATA));
		output(0).push(packet);
		cout<<"keepalive-response sent!"<<endl;
	}
}

CLICK_ENDDECLS
EXPORT_ELEMENT(TBG_CoLoR_Decapsulation)
EXPORT_ELEMENT(TBG_CoLoR_Encapsulation)
EXPORT_ELEMENT(Updatepid_TBG_Process)
EXPORT_ELEMENT(Updatepid_TBG_Send)

ELEMENT_MT_SAFE(TBG_CoLoR_Decapsulation)
ELEMENT_MT_SAFE(TBG_CoLoR_Encapsulation)
ELEMENT_MT_SAFE(Updatepid_TBG_Process)
ELEMENT_MT_SAFE(Updatepid_TBG_Send)

EXPORT_ELEMENT(TBG_MAINT)
ELEMENT_MT_SAFE(TBG_MAINT)
