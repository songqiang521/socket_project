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
#include <sys/stat.h>
#include <unistd.h>

//接受服务器的信息
void *recvMessage(void *arg)
{
	int fd=*(int *)arg;
	int ret=0;
	char recvbuf[1024];
	
	while(1)
	{
		memset(recvbuf,0,sizeof(recvbuf));
		if((ret=recv(fd,recvbuf,sizeof(recvbuf),0))==-1)
		{
			return NULL;
		}
		if(strcmp(recvbuf,"xiazai")==0)
		{
			system("rm -f mesg.txt");//确保本地没有此文件
			int fp;		//文件标识符
			fp=open("mesg.txt",O_WRONLY|O_CREAT|O_TRUNC,0666);
			memset(recvbuf,0,1024);
			//接收数据，如果“endend”表示接收结束
			while(1)		
			{
				recv(fd,&ret,sizeof(int),0);
				recv(fd,recvbuf,ret,0);
				if(strncmp(recvbuf,"endend",6)==0)
				{
					break;
				}
				write(fp,recvbuf,ret);
 
			}
			close(fp); 
			continue;
			
		}
		if(strcmp(recvbuf,"连接成功")==0)
		{
			system("clear");
		}
		puts(recvbuf);	
		if(ret==0)
		{
			exit(0);
		}
    }
	
}
void *sendMessage(void *arg)
{
	//发送
	int fd=*(int *)arg;
	char sendmsg[1024];
	while(1)
	{
			memset(sendmsg,0,sizeof(sendmsg));
			scanf("%s",sendmsg);
			if(send(fd,sendmsg,strlen(sendmsg),0)==-1)
			{
				return NULL; 	
			}
		
	}	
}

int main()
{
    int sockfd=0;
	int ret=0;
	int len=sizeof(struct sockaddr);
	
	struct sockaddr_in otheraddr;
	memset(&otheraddr,0,len);	
	//tcp套接字连接
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("sockfd");
		return -1;	
	}
	//初始化结构体，把服务器ip地址和端口号
	otheraddr.sin_family = AF_INET;
	otheraddr.sin_port = htons(8889);
	otheraddr.sin_addr.s_addr=inet_addr("111.231.215.174");
	//连接服务器
	if(connect(sockfd,(struct sockaddr*)&otheraddr,len)==-1)
	{
		perror("connect");
		return -1;		
	}
	printf("connect success...client fd=%d\n",sockfd);
	printf("client ip=%s,port=%d\n",inet_ntoa(otheraddr.sin_addr),ntohs(otheraddr.sin_port));		


	pthread_t id1,id2;

	char recvbuf[1024]={0};
    char sendbuf[1024]={0};
    
    //给服务器 发送信息
    strcpy(sendbuf,"hi,I am client");	

	if(send(sockfd,sendbuf,strlen(sendbuf),0)==-1)
	{
		perror("send");
		return -1;
	}
	if(recv(sockfd,recvbuf,sizeof(recvbuf),0)==-1)
	{
		perror("recv");
		return -1;
	}
	printf("sever say:%s\n",recvbuf);
	
	//创建收发线程
	pthread_create(&id1,NULL,sendMessage,&sockfd);
	pthread_create(&id2,NULL,recvMessage,&sockfd);
	
	//等待发送线程结束
	pthread_join(id1,NULL);
    return 0;
}





































