#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <memory.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h>
#include <sys/stat.h>
#define SIZE sizeof(cliMesg)  //结构体cliMesg的大小

typedef struct client_message
{
    char id[20];         //每个账号唯一id
	char passwd[20];	 //账号密码
	char name[50];		 //账号昵称
	char hy[100][20];	 //好友列表，最大100个
	int hys;             //好友数量
	int online;          //0不在线，1在线
	int fd;              //存放客户端成功连接后accept产生的新的套接字，不在线为-5
	int chatroom;		 //存放是否打开了双人聊天室，打开为1，没打开为0
	struct client_message *next;	 //下一条链表的首地址	
}cliMesg;

typedef struct friend_message{

    char rid[20];               //收信息的人
	char sid[20];               //发信息的人
	int type;                   //信息类型，好友请求1，私聊信息2,好友请求回复信息3
	char mesg[1024];            //信息
	struct friend_message * next;//下一条信息的首地址	
}friMesg;

cliMesg *head=NULL;
friMesg *head1=NULL; 
int count =0;   //账号数量
int count1=0;   //未发送的信息
//创建账号信息的头指针
cliMesg *create_count()
{
      cliMesg *p1;
      p1=(cliMesg *)malloc(SIZE);
       if(p1==NULL)
    {
       printf("create error\n");
	 return NULL;
	}
	p1->next=NULL;
       FILE *fp;
	fp=fopen("countmsg","a+");       //打开文件
	if(fp==NULL)
	{
        printf("open error\n");
        return NULL;
	}
	//文件为空
      if(fgetc(fp)==EOF)
      	{
       fclose(fp);
       return p1;
	}
	 //文件有内容
      rewind(fp);          //文件指针重返文件头
	int n;
	//获取账号数量
	fread(&n,sizeof(int),1,fp);		
	printf("n=%d\n",n);
	count=n;                         
	cliMesg t;
	cliMesg *tmp,*p;
	int i;
	for(i=0;i<n;i++)
	{
       fread(&t,sizeof(cliMesg),1,fp);
	   p=p1;
	   while(p->next)
	  {
			p=p->next;
	  }
	  tmp=(cliMesg *)malloc(sizeof(cliMesg));
	  tmp->next=NULL;
	  strcpy(tmp->id,t.id);
	  strcpy(tmp->name,t.name);
	  strcpy(tmp->passwd,t.passwd);
	  tmp->hys=t.hys;
	  int j;
	  //有好友将数据存入好友链表
	  for(j=0;j<tmp->hys;j++)
	  {
           strcpy(tmp->hy[j],t.hy[j]);
	  }
	  tmp->fd= -5;
	  tmp->chatroom=0;
	  tmp->online=0;
	  p->next=tmp;
	  
	}
	fclose(fp);
       return p1;

}

//创建未发送信息的头指针
friMesg *create_buffmsg()
{
	friMesg *x;
	x=(friMesg *)malloc(sizeof(friMesg));
	if(x==NULL)
	{
		printf("create error\n");
		return NULL;
	}
	x->next=NULL;
	FILE *fp;
	fp=fopen("buffmsg","a+");
	if(fp==NULL)
	{
		printf("open error\n");
		return NULL;
	}
	//如果为空文件关闭文件直接返回头指针
    if(fgetc(fp)==EOF)
	{
		fclose(fp);
		return x;
	}
	rewind(fp);
	int n;
	fread(&n,sizeof(int),1,fp);
	printf("n=%d\n",n);
	count1=n;
	
	friMesg t;
	friMesg *p,*p1;
	int i;
	for(i=0;i<n;i++)
	{
		fread(&t,sizeof(friMesg),1,fp);
		p1=x;
		while(p1->next)
		{
			p1=p1->next;
		}
		p=(friMesg *)malloc(sizeof(friMesg));
		p->next=NULL;
		strcpy(p->rid,t.rid);
		strcpy(p->sid,t.sid);
		p->type=t.type;
		strcpy(p->mesg,t.mesg);
		p1->next=p;
	}
	fclose(fp);
	return x;
}
//保存账户信息
void saveclient()
{
    FILE *fp;
	fp=fopen("countmsg","w");       //打开文件
	if(fp==NULL)
	{
             printf("open error\n");
		return;
	}
      printf("count=%d",count);
	fwrite(&count,sizeof(int),1,fp);		//先保存账号个数

    cliMesg* p;
	p=head;
	if(p->next==NULL) 		//如果账号列表为空，关闭文件并退出函数
	{		
		fclose(fp);
		return ;
	}	
	p=p->next;
	while(p)                //按结构体大小保存账号信息
	{
		fwrite(p,sizeof(cliMesg),1,fp);
		p=p->next;		
	}	
	printf("账号信息保存成功\n");
	fclose(fp);	

}
//保存未发出去的消息
void savefile()
{
	FILE  *fp;
	fp=fopen("buffmsg","w");
	if(fp==NULL)
	{
		printf("open error\n");
		return;
	}
	printf("count1=%d\n",count1);
	fwrite(&count1,sizeof(int),1,fp);
	friMesg *p;
	p=head1;
	if(p->next==NULL)
	{
		fclose(fp);
		return;
	}
	p=p->next;
	while(p)
	{
		fwrite(p,sizeof(cliMesg),1,fp);
		p=p->next;
	}
	printf("信息保存成功");
	fclose(fp);
	
}
//服务器上当前客户端id.txt保存聊天记录
void savefile2(char str[1024],char id[20])
{
	FILE *fp;
	char ch[1024];
	sprintf(ch,"%s.txt",id);
	fp=fopen(ch,"a+");        
	if(fp==NULL)			//打开错误关闭程序
		return ;
	memset(ch,0,1024);
	strcat(str,"\n");      //txt文档每行句末尾加特殊换行符
	strcpy(ch,str);
	fwrite(ch,strlen(ch),1,fp);			
	printf("聊天记录保存成功\n");
	fclose(fp);
}
int isnum1(char s[20])
{
    int i=0;
	while(s[i])
	{
	   if(!isdigit(s[i]))
	    {
              return 0;
	    }
        i++;
	}
    if(i<11)
	{
		return 0;
	}
    return 1;
}
void add(int fd)
{
    cliMesg *p1,*p,*p2;
     
    int leap=0;                                    //标识符，账号是否能正确注册
    p=(cliMesg *)malloc(SIZE);			
	if(p==NULL)
		return;
	char str[256];
	char str1[256];
	memset(str,0,sizeof(str));
	memset(str1,0,sizeof(str1));
	strcpy(str,"请输入您要注册的手机号");
	send(fd,str,strlen(str),0);
	memset(str,0,sizeof(str));
	recv(fd,str,sizeof(str),0);
	strcpy(str1,str);
	if(!isnum1(str))                 //判断是否纯数字账号
	{
		memset(str,0,sizeof(str));
		strcpy(str,"请输入正确的手机号\n");
		send(fd,str,strlen(str),0);
		return;
	}
	p1=head;
	//判断注册账户是否存在
	while(p1->next)
	{
		if(strcmp(p1->next->id,str)==0)
		{	
			leap=1;
			break;
		}
		p1=p1->next;
	}
	if(leap==1)
	{
		memset(str,0,sizeof(str));
		strcpy(str,"账号重复\n");
		send(fd,str,strlen(str),0);		
		return;
	}
	//正常注册
	strcpy(p->id,str1);
	memset(str,0,sizeof(str));
	strcpy(str,"请输入密码");
	send(fd,str,strlen(str),0);			
	
	memset(str,0,sizeof(str));
	recv(fd,str,sizeof(str),0);
	strcpy(p->passwd,str);
 
	memset(str,0,sizeof(str));
	strcpy(str,"请输入昵称");
	send(fd,str,strlen(str),0);	
 
	memset(str,0,sizeof(str));
	recv(fd,str,sizeof(str),0);
	strcpy(p->name,str);
	p1=head;
	while(p1->next)
	{
		p1=p1->next;
	}
	p1->next=p;
	p->hys=0;
	p->online=0;
	p->fd=-5;
	p->next=NULL;		
	memset(str,0,sizeof(str));
	strcpy(str,"注册成功,您可以登录了\n");
	send(fd,str,strlen(str),0);		
	count++;                                 //全局变量账号数量+1
	//保存账户信息
	saveclient();

}
int check_iscount(char id[20])
{
   cliMesg *p;
   if(head->next==NULL)
   {
     return 0;
   }
   p=head->next;
   while(p)
  {
	if(strcmp(id,p->id)==0)
	{
	return 1;
	}
	 p=p->next;
  }
   return 0;
}

int check_coutpasswd(char id[20],char passwd[20])
{
    cliMesg *p;
    if(head->next==NULL)
   {
     return 0;
   }
   p=head->next;
  
   while(p)
   {
            
		if(strcmp(id,p->id)==0 && strcmp(passwd,p->passwd)==0)
		{
			return 1;
		}
		p=p->next;
   }
   return 0;
}
int countOnline(char id[20])
{
     cliMesg *p;
	 p=head;
	 while(p)
	 {
            if((strcmp(p->id,id)==0) && p->online==1)
            	{
               return 1;
	            }
		p=p->next;
	 }
       return 0;
}
//查找已加好友
void find_friends(int fd,char id[20])
{
	cliMesg *p,*p1;
	p=head;
	char sendbuf[1024]={0};
    char recvbuf[1024]={0};
	char find_id[1024]={0};
	
	strcpy(sendbuf,"请输入您要查找的手机号");
	send(fd,sendbuf,strlen(sendbuf),0);
	recv(fd,find_id,sizeof(find_id),0);
	//查找手机号是否存在
	if(check_iscount(find_id)==0)
	{
		memset(sendbuf,0,sizeof(sendbuf));
		strcpy(sendbuf,"此号码不存在");
		send(fd,sendbuf,strlen(sendbuf),0);
		return;
	}
    //查找自己是否有该好友
	p=p->next;
	while(p)
	{
		if(strcmp(p->id,id)==0)
		{
			break;
		}
		p=p->next;
	}
	if(p->hys==0)
	{
		memset(sendbuf,0,sizeof(sendbuf));
		strcpy(sendbuf,"没有此好友");
		send(fd,sendbuf,strlen(sendbuf),0);
	}
	int i;
	int num=p->hys;
	for(i=0;i<num;i++)
	{
		//如果找到
		if(strcmp(p->hy[i],find_id)==0)
		{
			//将其昵称取出
			p1=head->next;
			char str[20]={0};
			while(p1)
			{
			  	if(strcmp(p1->id,find_id)==0)
				{
					strcpy(str,p1->name);
					break;			
				}
				p1=p1->next;
			}	
            memset(sendbuf,0,sizeof(sendbuf));
            sprintf(sendbuf,"该好友账户为%s,昵称为%s",find_id,str);	
            send(fd,sendbuf,strlen(sendbuf),0);			
		}
		
	}
}
//删除好友
void del_friends(int fd,char id[20])
{
	cliMesg *p,*p1;
	int i,leap=0;
	char sendbuf[1024]={0};
	char recvbuf[1024]={0};
	
	p=head->next;
	while(p)
	{
		if(strcmp(p->id,id)==0)
		{
			break;
		}
		p=p->next;
	}
	if(p->hys==0)
    {
		strcpy(sendbuf,"您的好友列表为空");
		send(fd,sendbuf,strlen(sendbuf),0);
		return;
	}
	//好友列表含有信息
	memset(sendbuf,0,sizeof(1024));
	strcpy(sendbuf,"输入你想删除的好友账户");
	send(fd,sendbuf,strlen(sendbuf),0);
	recv(fd,recvbuf,sizeof(recvbuf),0);
	for(i=0;i<(p->hys);i++)
    {
		if(strcmp(p->hy[i],recvbuf)==0)
		{
			//删除中间节点
			while(i<(p->hys-1))
			{
				strcpy(p->hy[i],p->hy[i+1]);
				i++;
			}
			memset(p->hy[i],0,sizeof(p->hy[i]));
			p->hys--;
			memset(sendbuf,0,sizeof(sendbuf));
			sprintf(sendbuf,"删除成功");
			send(fd,sendbuf,strlen(sendbuf),0);
			leap=1;
			saveclient();
			break;
		}	
	}
	//本地删除了，对方列表也要删除
	if(leap==1)
	{
		p1=head->next;
		while(p1)
		{
			if(strcmp(p1->id,recvbuf)==0)
			{
				break;
			}
			p1=p1->next;		
		}
		for(i=0;i<(p1->hys);i++)
		{
			if(strcmp(p1->hy[i],p->id)==0)
			{
				while(i<(p1->hys-1))
			    {
				strcpy(p1->hy[i],p1->hy[i+1]);
			   	 i++;
			    } 
				memset(p1->hy[i],0,sizeof(p1->hy[i]));
				p1->hys--;
				memset(sendbuf,0,sizeof(sendbuf));
				sprintf(sendbuf,"对方那也删除");
				send(fd,sendbuf,strlen(sendbuf),0);
				saveclient();
				break;
			}
			
		}
	}
	else
	{
		memset(sendbuf,0,sizeof(sendbuf));
		sprintf(sendbuf,"删除失败，您没有此好友");
		send(fd,sendbuf,strlen(sendbuf),0);		
	}
}
//添加好友 
void add_friends(int fd,char id[20])
{
	cliMesg *p,*p1;
	int i;
	char sendbuf[1024]={0};
	char recvbuf[1024]={0};
	//找到自己的账户节点
	p=head->next;
	while(p)
	{
		if(strcmp(p->id,id)==0)
		{
			break;
		}
		p=p->next;
	}
	
	strcpy(sendbuf,"输入添加的好友手机号");
	send(fd,sendbuf,strlen(sendbuf),0);
	recv(fd,recvbuf,sizeof(recvbuf),0);
	memset(sendbuf,0,sizeof(sendbuf));
	strcpy(sendbuf,"消息已发送");
	send(fd,sendbuf,strlen(sendbuf),0);
	if(check_iscount(recvbuf)==0)
	{
	memset(sendbuf,0,sizeof(sendbuf));
	strcpy(sendbuf,"没有此账号");
	send(fd,sendbuf,strlen(sendbuf),0);
	return;
	}
	if(strcmp(recvbuf,p->id)==0)
	{
	memset(sendbuf,0,sizeof(sendbuf));
	strcpy(sendbuf,"不能添加自己为好友");
	send(fd,sendbuf,strlen(sendbuf),0);
	return ;
	}
	//查看自己好友列表里是否有该好友
	for(i=0;i<p->hys;i++)
	{
		if(strcmp(recvbuf,p->hy[i])==0)
		{
		 memset(sendbuf,0,sizeof(sendbuf));
	     strcpy(sendbuf,"此好友已添加");
	     send(fd,sendbuf,strlen(sendbuf),0);
         return;		 
		}
	}
	//如果没有该好友,的向对方发送请求
	p1=head->next;
	time_t timep;
	while(p1)
	{
		if(strcmp(p1->id,recvbuf)==0)
		{
			friMesg *m,*m1;
			m=(friMesg *)malloc(sizeof(friMesg));
			m->type=1;
			strcpy(m->rid,recvbuf);   //收消息的人
			strcpy(m->sid,p->id);     //发消息的人
          	memset(sendbuf,0,sizeof(sendbuf));
            time(&timep);
            sprintf(sendbuf,"%s\n%s想添加您为好友,y同意,n不同意",ctime(&timep),p->id);
			strcpy(m->mesg,sendbuf);
			m->next=NULL;
			m1=head1;
			while(m1->next)
			{
				m1=m1->next;
			}
			m1->next=m;
			count1++;
			//如果对方在线
			if(countOnline(p1->id)==1)
			{
				memset(sendbuf,0,sizeof(sendbuf));
	            strcpy(sendbuf,"您有新消息，请输入open查看");
	            send(p1->fd,sendbuf,strlen(sendbuf),0);
			}
			savefile();  //保存未发送的信息
			
		}
		p1=p1->next;
	}
}

char a[100][20];   //存放聊天室人的id
int len=0;         //聊天室人数
//多人聊天
void multi_chat(int fd,char id[20])
{
	char sendbuf[1024]={0};
	char recvbuf[1024]={0};
	strcpy(sendbuf,"您已进入聊天室,输入quit退出,输入look查看当前人");
	send(fd,sendbuf,sizeof(sendbuf),0);
	strcpy(a[len],id);
	len++;
	int i;
	cliMesg *p;
	time_t timep;
	time(&timep);
	while(1)
	{
		memset(recvbuf,0,sizeof(recvbuf));
		recv(fd,recvbuf,sizeof(recvbuf),0);
		if(strcmp(recvbuf,"quit")==0)
		{
		   for(i=0;i<len;i++)
		   {
			   if(strcmp(a[i],id)==0)
			   {
				   while(i<len-1)
				   {
					   strcpy(a[i],a[i+1]);
					   i++;
				   }
				   
			   }		   
		   }
		   len--;
		   for(i=0;i<len;i++)
		   {
			   p=head->next;
			   while(p)
			   {
				   if(strcmp(p->id,a[i])==0)
				   {
					   //向还在群里的人发送通知
					   memset(sendbuf,0,sizeof(sendbuf));
					   sprintf(sendbuf,"%s退出聊天室",id);
					   send(p->fd,sendbuf,strlen(sendbuf),0);
					   break;
				   }
				   p=p->next;
			   }
			   		   
		   }
		   return ;
		}
		//查看当前群里有多少人
		if(strcmp(recvbuf,"look")==0)
	    {
			memset(sendbuf,0,sizeof(sendbuf));
			sprintf(sendbuf,"当前聊天室有%d人,他们是：",len);
			send(fd,sendbuf,strlen(sendbuf),0);
			for(i=0;i<len;i++)
			{
				p=head->next;
				while(p)
				{
					if(strcmp(p->id,a[i])==0)
					{
						memset(sendbuf,0,sizeof(sendbuf));
			            sprintf(sendbuf,"昵称是%s 账户是%s\n",p->name,p->id);
			            send(fd,sendbuf,strlen(sendbuf),0);
						break;
					}
					p=p->next;
				}		
			}
			continue;
		}
        //群发消息
        for(i=0;i<len;i++)
		{
			p=head->next;
			while(p)
			{
				if(strcmp(p->id,a[i])==0 &&strcmp(p->id,id)!=0)
				{
					memset(sendbuf,0,sizeof(sendbuf));
			        sprintf(sendbuf,"%s%s say: %s",ctime(&timep),id,recvbuf);
			        send(p->fd,sendbuf,strlen(sendbuf),0);
				    break;
				}
				p=p->next;
			}
			
		}			
	}
	
}
//修改个人信息
void modify(int fd,char id[20])
{
	cliMesg *p,*p1;
	int i;
	char sendbuf[1024]={0};
	char recvbuf[1024]={0};
	p=head->next;
	while(1)
	{
		if(strcmp(p->id,id)==0)
		{
			break;
		}
		p=p->next;
	}
	sprintf(sendbuf,"我的昵称：%s\n我的账户：%s\n我的密码：%s",p->name,p->id,p->passwd);
	send(fd,sendbuf,strlen(sendbuf),0);
	memset(sendbuf,0,sizeof(sendbuf));
	strcpy(sendbuf,"\t1.修改昵称\n\t2.修改密码\n\t3.退出");
	send(fd,sendbuf,strlen(sendbuf),0);
	recv(fd,recvbuf,sizeof(recvbuf),0);
	if(strcmp(recvbuf,"1")==0)
	{
		memset(sendbuf,0,sizeof(sendbuf));
		strcpy(sendbuf,"请输入新的昵称");
		send(fd,sendbuf,strlen(sendbuf),0);
		memset(recvbuf,0,sizeof(recvbuf));
		recv(fd,recvbuf,sizeof(recvbuf),0);
		strcpy(p->name,recvbuf);
		saveclient();
		return;
	}else if(strcmp(recvbuf,"2")==0)		//输入2修改密码
	{
		memset(sendbuf,0,sizeof(sendbuf));
		strcpy(sendbuf,"请输入新的密码");
		send(fd,sendbuf,strlen(sendbuf),0);
		memset(recvbuf,0,sizeof(recvbuf));
		recv(fd,recvbuf,sizeof(recvbuf),0);
		strcpy(p->passwd,recvbuf);
		saveclient();
		return ;
	}
	else if(strcmp(recvbuf,"3")==0)
	{
		return;
	}	
	else
	{
		memset(sendbuf,0,sizeof(sendbuf));
		strcpy(sendbuf,"输入非法");
		send(fd,sendbuf,strlen(sendbuf),0);
	}
}
//双人聊天室
void double_chat(int fd,char id[20])
{
	int flags=1;
	cliMesg *p;
	
	p=head->next;
	time_t timep;	
	time(&timep);
	char recvbuf[1024]={0};
	char sendbuf[1024]={0};
	char str[1024]={0};
	int ret=0;
	strcpy(sendbuf,"输入你想聊天的好友账户");
	send(fd,sendbuf,strlen(sendbuf),0);
	
	recv(fd,recvbuf,sizeof(recvbuf),0);
	strcpy(str,recvbuf);
	cliMesg *p1;
	p1=head->next;
	while(p1)
	{
		if(strcmp(p1->id,str)==0)
		{
	     break;
		}
		p1=p1->next;
	}
	/*
    memset(sendbuf,0,1024);
	sprintf(sendbuf,"flags=%d",p1->chatroom);
    send(fd,sendbuf,strlen(sendbuf),0);
	*/
  if(p1->chatroom==0)
  {
	if(strcmp(recvbuf,id)==0)		//不能与自己聊天
	{
		memset(sendbuf,0,1024);
		strcpy(sendbuf,"不能与自己聊天");
		send(fd,sendbuf,strlen(sendbuf),0);
		return;		
	}	
	while(p)
	{
		if(strcmp(p->id,id)==0)
		{
			break;
		}
		p=p->next;
	}
	cliMesg *p1;
	p1=head->next;
	while(p1)
	{
		if(strcmp(p1->id,recvbuf)==0)
		{
	     break;
		}
		p1=p1->next;
	}
	if(p->hys==0)
	{
		memset(sendbuf,0,sizeof(sendbuf));
		strcpy(sendbuf,"没有此好友");
		send(fd,sendbuf,strlen(sendbuf),0);
		return;
	}
	int i;
	int num=p->hys;
	
	if(p->fd==-5)			//判断好友在不在线和有没有打开聊天窗口
	{
		memset(sendbuf,0,1024);
		strcpy(sendbuf,"好友不在线");
		send(fd,sendbuf,1024,0);
		return;
	}
	for(i=0;i<num;i++)
	{
		if(strcmp(p->hy[i],str)==0)
		{
			flags=0;
			memset(sendbuf,0,1024);
			//p->chatroom=1;
			p1->chatroom=1;
	        strcpy(sendbuf,"连接成功");
	        send(fd,sendbuf,strlen(sendbuf),0);
			
		}
		
	}
    if(flags==1)
	{
		memset(sendbuf,0,1024);
	    strcpy(sendbuf,"请先添加该好友");
	    send(fd,sendbuf,strlen(sendbuf),0);
		return;
	}
	//进入聊天室
	 while(1)					//进入聊天，输入quit退出
	 {
		memset(recvbuf,0,1024);
		memset(sendbuf,0,1024);	
		recv(fd,recvbuf,1024,0);
		if(strcmp(recvbuf,"quit")==0)
		{
			p->chatroom=0;
			memset(sendbuf,0,1024);		
			sprintf(sendbuf,"%s 已退出,请您输入quit退出",id);
		    send(p1->fd,sendbuf,1024,0);
			break;
		}
		memset(sendbuf,0,1024);		
		sprintf(sendbuf,"%s %s say:%s",ctime(&timep),id,recvbuf);
		savefile2(sendbuf,id);//保存聊天记录
		savefile2(sendbuf,p1->id);
		send(p1->fd,sendbuf,1024,0);
		memset(recvbuf,0,1024);
		memset(sendbuf,0,1024);	
		
		recv(p1->fd,recvbuf,1024,0);
		sprintf(sendbuf,"%s %s say:%s",ctime(&timep),p1->id,recvbuf);
		savefile2(sendbuf,id);//保存聊天记录
		savefile2(sendbuf,p1->id);
		send(fd,sendbuf,1024,0);	
	}	
   }else
   {
	   memset(sendbuf,0,sizeof(sendbuf));
	   strcpy(sendbuf,"该好友正在聊天");
	   send(fd,sendbuf,strlen(sendbuf),0);
   }
}
void manage_friends(int fd,char id[20])
{
	char sendbuf[1024]={0};
	char recvbuf[1024]={0};
	strcpy(sendbuf,"\n\t1.查找好友\n\t2.删除好友\n\t3.添加好友\n\t4.退出\n");
	send(fd,sendbuf,strlen(sendbuf),0);
	recv(fd,recvbuf,sizeof(recvbuf),0);
	if(strcmp(recvbuf,"1")==0)
	{
		find_friends(fd,id);
	}
	/*
	if(strcmp(recvbuf,"2")==0)
	{
		display_friends(fd,id);
	}
	*/
	if(strcmp(recvbuf,"2")==0)
	{
		del_friends(fd,id);
	}
	if(strcmp(recvbuf,"3")==0)
	{ 
		//添加朋友消息
		add_friends(fd,id);
	}
	if(strcmp(recvbuf,"4")==0)
	{ 
		return;
	}
}

//在线从服务器查看聊天记录
void jilu(int fd,char id[20])
{
	int fp;      //文件标识符
	char str[1024];
	sprintf(str,"%s.txt",id); //打开相应人的聊天记录文本文件
	fp=open(str,O_RDONLY);    //只读方式打开
	if(fp==-1)
	{
		memset(str,0,1024);
		strcpy(str,"您没有聊天记录");
		send(fd,str,strlen(str),0);
		return;
	}
	int ret;
	memset(str,0,1024);
	while(ret=read(fp,str,1024))	//ret读的长度，为0退出while
	{
		write(fd,str,strlen(str));	//读出来发送给客户端
		memset(str,0,1024);		
	}	
	close(fp);
}
void load(int fd,char id[20])
{
	int fp;
	char sendbuf[1024];
	char recvbuf[1024]={0};
	sprintf(sendbuf,"%s.txt",id);
	fp=open(sendbuf,O_RDONLY);
	if(fp==-1)							//先判断有没有聊天记录文件
	{
		memset(sendbuf,0,1024);
		strcpy(sendbuf,"您没有聊天记录");
		send(fd,sendbuf,strlen(sendbuf),0);
		return;
	}
	
	int ret;
	memset(sendbuf,0,1024);
	strcpy(sendbuf,"xiazai");
    send(fd,sendbuf,strlen(sendbuf),0);
	memset(sendbuf,0,1024);	
	while(ret=read(fp,sendbuf,1024))
	{
		send(fd,&ret,sizeof(int),0);		//先发送内容字节数，再发送内容
		send(fd,sendbuf,ret,0);
		memset(sendbuf,0,1024);
	}
	memset(sendbuf,0,1024);
	strcpy(sendbuf,"endend");	
	ret = strlen(sendbuf);						//结束后发送endend告诉客户端发送完毕
	send(fd,&ret,sizeof(int),0);
	send(fd,sendbuf,strlen(sendbuf),0);
	close(fp);
}
//处理客户端的信息
void *handlerClient(void *arg)
{
	 int fd=*(int *)arg;
     char recvbuf[1024]={0};
	 char recvbuf1[1024]={0};
     char  sendbuf[1024]={0};
	 int ret;
	
      //接受数据
	 if((ret=recv(fd,recvbuf,sizeof(recvbuf),0))==-1)
	{
         perror("recv");
	     return NULL;
	}

	printf("client say:%s\n",recvbuf);
    while(1)
    {
          memset(sendbuf,0,sizeof(sendbuf));	
          //登录界面
		  strcpy(sendbuf,"欢迎使用快播，可使用功能如下\n\t1.登录帐号\n\t2.注册帐号\n\t3.退出\n");
	      send(fd,sendbuf,strlen(sendbuf),0);
	      memset(recvbuf,0,sizeof(recvbuf));
	     //接受客户端的数据
	     if(recv(fd,recvbuf,sizeof(recvbuf),0)==-1)
	     {
              perror("recv");
	          return NULL; 
	     }
	     if(strcmp(recvbuf,"1")==0)
	     {
              memset(sendbuf,0,sizeof(sendbuf));
	          strcpy(sendbuf,"请输入登录账号");
		      if(send(fd,sendbuf,strlen(sendbuf),0)==-1)
		       {
                  perror("send");
		          return NULL;
		       }
		
               if(recv(fd,recvbuf,sizeof(recvbuf),0)==-1)
               {
                perror("recv");
	            return NULL; 
		       }
	        
             memset(sendbuf,0,sizeof(sendbuf));
	         strcpy(sendbuf,"请输入登录密码");
		     if(send(fd,sendbuf,strlen(sendbuf),0)==-1)
		     {
                  perror("send");
		          return NULL;
		     }
		
              if(recv(fd,recvbuf1,sizeof(recvbuf1),0)==-1)
               {
                   perror("recv");
	                return NULL; 
		       } 
               //匹配账号密码是否存在且正确
               if(check_iscount(recvbuf)==0 ||check_coutpasswd(recvbuf,recvbuf1)==0)
               {
                memset(sendbuf,0,sizeof(sendbuf));
				strcpy(sendbuf,"输入账号或密码不正确");
				send(fd,sendbuf,strlen(sendbuf),0);
				continue;
	           }
		       //判断该账号是否在线
                else if(countOnline(recvbuf)==1)
                 {
                memset(sendbuf,0,sizeof(sendbuf));
				strcpy(sendbuf,"此账号已在线");
				send(fd,sendbuf,strlen(sendbuf),0);
				continue;
		         }
                else
               {
                strcpy(recvbuf1,recvbuf);
                memset(sendbuf,0,sizeof(sendbuf));
			    strcpy(sendbuf,"登录成功");
                cliMesg *p;
	            p=head;
			    while(p->next)
			    {
                    if((strcmp((p->next)->id,recvbuf1)==0))
                    {
                               (p->next)->online=1;
				               (p->next)->fd=fd;
							   (p->next)->chatroom=0;
				    }
                    p=p->next;

			    }
			    send(fd,sendbuf,strlen(sendbuf),0);
                //登录时判断是否有新消息
                friMesg *p9;
				p9=head1;
				int len=0;
				while(p9)
				{
					if(strcmp(p9->rid,recvbuf1)==0)
					{
						len=1;
						memset(sendbuf,0,sizeof(sendbuf));
						strcpy(sendbuf,"您有新消息，请输入open查看\n");
						send(fd,sendbuf,strlen(sendbuf),0);
						break;
					}
					p9=p9->next;
				}
                 

			    //主界面
				memset(recvbuf,0,sizeof(recvbuf));
                while(1)
                 {
                    memset(sendbuf,0,sizeof(sendbuf));
					strcpy(sendbuf,"你可以使用的功能如下\n\t1.好友管理\n\t2.个人管理\n\t3.群聊天地\n\t4.私聊蜜语\n\t5.查看聊天记录\n\t6.下载聊天记录\n\t7.退出");
					send(fd,sendbuf,strlen(sendbuf),0);	
                    memset(recvbuf,0,sizeof(recvbuf));					
                    if(recv(fd,recvbuf,sizeof(recvbuf),0)==-1)
					{
						perror("recv");
						return NULL;
					}
					printf("buf=%s",recvbuf);
					
					//对主界面进行分析
					if(strcmp(recvbuf,"1")==0)
					{
						manage_friends(fd,recvbuf1);
					}
					if(strcmp(recvbuf,"2")==0)
					{
						//修改个人消息
						modify(fd,recvbuf1);
					}
					if(strcmp(recvbuf,"3")==0)
					{
						//群聊
						multi_chat(fd,recvbuf1);
					}
					if(strcmp(recvbuf,"4")==0)
					{
						//私聊
						double_chat(fd,recvbuf1);
					}
                    if(strcmp(recvbuf,"5")==0)
					{
						jilu(fd,recvbuf1);
					}
					 if(strcmp(recvbuf,"6")==0)
					{
						load(fd,recvbuf1);
					}
					//打开信息
					if(strcmp(recvbuf,"open")==0)
					{
						friMesg *m;
						int flags=0;
						m=head1->next;
						printf("count1=%d\n",count1);
						while(m)
						{
							if(strcmp(m->rid,recvbuf1)==0)
							{
								flags=1;
								if(m->type==1)    //1为好友请求消息
								{
									memset(sendbuf,0,sizeof(sendbuf));
									strcpy(sendbuf,m->mesg);
									send(fd,sendbuf,strlen(sendbuf),0);
									//有两种情况 同意或不同意
									while(1)
									{
										memset(recvbuf,0,sizeof(recvbuf));
										recv(fd,recvbuf,sizeof(recvbuf),0);
										
										if(strcmp(recvbuf,"y")==0)
										{
											m->type=3;     //好友请求回复
											strcpy(m->rid,m->sid);
											strcpy(m->sid,recvbuf1);
											
											memset(sendbuf,0,sizeof(sendbuf));
											sprintf(sendbuf,"%s已同意添加,添加成功",recvbuf1);
											strcpy(m->mesg,sendbuf);
											
											cliMesg *p1;
											p=head->next;
											p1=head->next;
											//收消息的人
											while(p)
											{
												if(strcmp(p->id,m->rid)==0)
												{
													break;
												}
												p=p->next;
											}
					                        //发消息的人
											while(p1)
											{
												if(strcmp(p1->id,m->sid)==0)
												{
													break;
												}
												p1=p1->next;
											}
											//同意往好友列表添信息
										    strcpy(p->hy[p->hys],p1->id);
											p->hys++;
											strcpy(p1->hy[p1->hys],p->id);
											p1->hys++;
											if(countOnline(m->rid)==1)
											{
												memset(sendbuf,0,sizeof(sendbuf));
												strcpy(sendbuf,"您有新消息，请输入open查看");
												send(p->fd,sendbuf,strlen(sendbuf),0);
											}
											saveclient();
											savefile();
											break;
										}else if(strcmp(recvbuf,"n")==0)
										{
											cliMesg *p;
											p=head->next;
											while(p)
											{
											    if(strcmp(p->id,m->sid)==0)
												{
													break;	
												}	
                                                p=p->next;												
											}
											m->type=3;    //发消息
											strcpy(m->rid,m->sid);
											strcpy(m->sid,recvbuf1);
											memset(sendbuf,0,sizeof(sendbuf));
											sprintf(sendbuf,"%s已拒绝添加请求,添加失败",recvbuf1);
											strcpy(m->mesg,sendbuf);
											if(countOnline(m->rid)==1)
											{
												memset(sendbuf,0,sizeof(sendbuf));
												strcpy(sendbuf,"您有新消息，请输入open查看");
												send(p->fd,sendbuf,strlen(sendbuf),0);
											}
											savefile();
											break;
											
										}		
										
									}
								}
								//好友请求回复
								else if(m->type==3)
								{
									flags=1;
									memset(sendbuf,0,sizeof(sendbuf));
									strcpy(sendbuf,m->mesg);
									send(fd,sendbuf,strlen(sendbuf),0);
									
									friMesg *m1,*m2;
									m1=head1;
									while(m1->next)
									{
										if(strcmp(m1->next->rid,recvbuf1)==0)
										{
											m2=m1->next;
											m1->next=m2->next;
											free(m2);
											break;
										}
										m1=m1->next;
									}
									count1--; 
									savefile();
									
                                   	if(m==NULL);
									break;										
								}
						  }
						  if(m==NULL)
						  {
							break;
						  }
						  m=m->next;
						}
						
					}
					//退出
					if(strcmp(recvbuf,"7")==0)
					{
						
						p=head;
						while(p->next)
						{
							if(strcmp(p->next->id,recvbuf1)==0)
							{
								p->next->online=0;
								p->next->fd=-5;
								p->chatroom=0;
								break;
							}
							p=p->next;
						}
						break;
					}
				
			      }
		       }
		   
         }
	     else if(strcmp(recvbuf,"2")==0)
	     {
	          //注册添加账户
              add(fd);
	     }
	     else if(strcmp(recvbuf,"3")==0)
	     {
           memset(sendbuf,0,sizeof(sendbuf));
		   strcpy(sendbuf,"欢迎下次再来");
		   send(fd,sendbuf,strlen(sendbuf),0);
		   break;
	    }
	     else
	    {
              continue;
	    }
    }
	close(fd);
}
//判断客户端是否依旧在线
void * judgeClient(void)
{
	cliMesg* p;
	char fa[1024];
	p=head;
	while(1)         //判断是否断开连接，如果断开将此账号置为不在线状态，监听套接字-5
	{
		p=head->next;
		while(p)
		{
			if(p->online==1)
			{				
				struct tcp_info info;
				int len=sizeof(info);         
				getsockopt(p->fd,IPPROTO_TCP,TCP_INFO,&info,(socklen_t*)&len);
				if((info.tcpi_state!=TCP_ESTABLISHED))
				{
					p->online=0;
					p->fd=-5;
				}				
			}
			p=p->next;
		}
	}
	
}
int main()
{

    int sockfd=0,confd=0;    //定义并初始化
	int chlid=0;
	int ret=0;
	int len=sizeof(struct sockaddr);
	
	struct sockaddr_in myaddr;
	struct sockaddr_in otheraddr;
	memset(&myaddr,0,len);
	memset(&otheraddr,0,len);	
	
    sockfd = socket(AF_INET, SOCK_STREAM,0);
    
      //初始化结构体
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(8889);
	myaddr.sin_addr.s_addr=htonl(INADDR_ANY);//自动获取本机ip地址
	int reuse = 1;
	 if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
       {
         perror("setsockopet error\n");
         return -1;
       }
	 //绑定套接字
	if(-1==(bind(sockfd,(struct sockaddr*)&myaddr,len)))
	 {
		perror("bind");   
		return -1;		
	 }
	//开始侦听
	if((listen(sockfd,10))==-1)
	{
		perror("listen");
		return -1;		
	}
	printf("server waitting......\n");
	pthread_t id1,id2;

	head=create_count();        //创建账号信息的头结点
	head1=create_buffmsg();    //创建缓存信息的头结点

	while(1)
	{
              //接受客户端信息
              if((confd=accept(sockfd,(struct sockaddr *)&otheraddr,&len))==-1)
              {
                perror("accept");
			    return -1;
		      }
             //输出客户端的编号 、ip和端口
             printf("client fd=%d server fd=%d\n",confd,sockfd);
			 printf("client ip=%s port=%d\n",inet_ntoa(otheraddr.sin_addr),ntohs(otheraddr.sin_port));

			 //处理客户端
             pthread_create(&id1,NULL,handlerClient,&confd);
			 //判断客户端是否在线
			 pthread_create(&id2,NULL,(void *)judgeClient,&confd);
            
             pthread_detach(id1);//线程结束后资源系统自动回收
             pthread_detach(id2); 
	}
	close(sockfd);
	
    return 0;
}











