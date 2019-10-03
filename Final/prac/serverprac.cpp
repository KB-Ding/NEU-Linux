 /*
 *epoll基于非阻塞I/O事件驱动
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#include <iostream>
#include <string.h>

#include <pthread.h>
#include <ctype.h>


#define MAX_EVENTS  1024                                    //监听上限数
#define BUFLEN 4096
#define SERV_PORT   8080
#define CAUTION "There is only one int the char room!"
#define SERVER_MESSAGE "ClientID %d say >> %s"


using namespace std;


struct Position
{
    int x;
    int y;
};
struct Tank
{
    Position pos;
    int direction;
    bool life;
    int shoot;
    Position spos;
    int sdir;
    int sshoot;
};

void makewall(int i,int j);
void initmap();
void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);
//int sendBroadcastmessage(int clientfd);
int sendBroadcastmessage(int clientfd,int roomid,char buf[]);
void makehome();
void resetpos(Tank& t,int roomnu,int inputi, int reserx,int resery,int dirct);
/* 描述就绪文件描述符相关信息 */
int const Fieldx = 72;
int const Fieldy = 28;
int const rightboader = 153;
int flag=0;
int roomnumber=1;


int p0=0;
int p1=1;
int h0=2;
int h1=3;


static int gameflag = 0;
static char field[Fieldy][Fieldx];
//static char savefield[Fieldy][Fieldx];
static char savefield[Fieldy][Fieldx];
char rankboard[Fieldy][rightboader-Fieldx+1];

struct Bullet
{
    Position pos;
    int direction;
    bool enemyshoot;
    bool exist;
    Bullet()
    {
        pos.x = -1;
        pos.y = -1;
        direction = 0;
        enemyshoot = true;
        exist = false;
    }
};



struct fdinfor{
    int fd;                                                
  //  struct sockaddr_in cliaddr;//sockaddr_in    
    int gaming;                                
    bool exist;
};


struct room{
    int fd1;  
    int fd2;
    int roomnum; 
    int fd1life;
    int fd1points;
    int fd2life;  
    int fd2points;
    Tank player[4];
  //  Tank player2;
    //Tank help1;
   // Tank help2;   
    char warfield[Fieldy][Fieldx];                           
   
}rooms[30];

Tank player;
Tank helper;
Tank enemy;
Tank enemyhelper;

struct myevent_s {
    int fd;                                                 //要监听的文件描述符
    int events;                                             //对应的监听事件
    void *arg;                                              //泛型参数
    void (*call_back)(int fd, int events, void *arg);       //回调函数
    int status;                                             //是否在监听:1->在红黑树上(监听), 0->不在(不监听)
    char buf[BUFLEN];
    int len;
    long last_active;                                       //记录每次加入红黑树 g_efd 的时间值
};

std::vector<fdinfor> fdlist;
int g_efd;                                                  //全局变量, 保存epoll_create返回的文件描述符
struct myevent_s g_events[MAX_EVENTS+1];                    //自定义结构体类型数组. +1-->listen fd



/*将结构体 myevent_s 成员变量 初始化*/

void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg)
{
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    //memset(ev->buf, 0, sizeof(ev->buf));
    //ev->len = 0;
    ev->last_active = time(NULL);                       //调用eventset函数的时间

    return;
}

/* 向 epoll监听的红黑树 添加一个 文件描述符 */

void eventadd(int efd, int events, struct myevent_s *ev)
{
    struct epoll_event epv = {0, {0}};
    int op;
    epv.data.ptr = ev;
    epv.events = ev->events = events;       //EPOLLIN 或 EPOLLOUT

    if (ev->status == 1) {                                          //已经在红黑树 g_efd 里
        op = EPOLL_CTL_MOD;                                         //修改其属性
    } else {                                //不在红黑树里
        op = EPOLL_CTL_ADD;                 //将其加入红黑树 g_efd, 并将status置1
        ev->status = 1;
    }

    if (epoll_ctl(efd, op, ev->fd, &epv) < 0)                       //实际添加/修改.zhuce
        printf("event add failed [fd=%d], events[%d]\n", ev->fd, events);
    else
        printf("event add OK [fd=%d], op=%d, events[%0X]\n", ev->fd, op, events);

    return ;
}

/* 从epoll 监听的 红黑树中删除一个 文件描述符*/

void eventdel(int efd, struct myevent_s *ev)
{
    struct epoll_event epv = {0, {0}};

    if (ev->status != 1)                                        //不在红黑树上
        return ;

    epv.data.ptr = ev;
    ev->status = 0;                                             //修改状态
    epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv);                //从红黑树 efd 上将 ev->fd 摘除

    return ;
}

/*  当有文件描述符就绪, epoll返回, 调用该函数 与客户端建立链接 */

void acceptconn(int lfd, int events, void *arg)
{
    struct sockaddr_in cin;
    socklen_t len = sizeof(cin);
    int cfd, i;

    if ((cfd = accept(lfd, (struct sockaddr *)&cin, &len)) == -1) {
        if (errno != EAGAIN && errno != EINTR) {
            
        }
        printf("%s: accept, %s\n", __func__, strerror(errno));
        return ;
    }


    do {
        for (i = 0; i < MAX_EVENTS; i++)                                //从全局数组g_events中找一个空闲元素
            if (g_events[i].status == 0)                                //类似于select中找值为-1的元素
                break;                                                  //跳出 for

        if (i == MAX_EVENTS) {
            printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
            break;                                                      //跳出do while(0) 不执行后续代码
        }

        int fllag = 0;
        if ((fllag = fcntl(cfd, F_SETFL, O_NONBLOCK)) < 0) {             //将cfd也设置为非阻塞
            printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
            break;
        }

        /* 给cfd设置一个 myevent_s 结构体, 回调函数 设置为 recvdata */

        eventset(&g_events[i], cfd, recvdata, &g_events[i]);   
        eventadd(g_efd, EPOLLIN, &g_events[i]);                         //将cfd添加到红黑树g_efd中,监听读事件

    } while(0);
    
    printf("%d\n",cfd );

//bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
   // for (int i = 0; i < fdlist.size(); i++)
     //   {
            

             printf("%d",fdlist.size());
            printf("======%d\n",cfd );
       // }

//aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
    printf("new connect [%s:%d][time:%ld], pos[%d]\n", 
            inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i);
    return ;
}

void recvdata(int fd, int events, void *arg)
{
    printf("receive");
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;



    len = recv(fd, ev->buf, sizeof(ev->buf), 0);            //读文件描述符, 数据存入myevent_s成员buf中




    eventdel(g_efd, ev);        //将该节点从红黑树上摘除

    if (len > 0) {

        ev->len = len;
        ev->buf[len] = '\0';                              
        printf("C[%d]:%s\n", fd, ev->buf);

        char sv_msd[BUFLEN];
        strcpy(sv_msd, ev->buf);
        string fu = sv_msd;

        if (fu=="yes")
        {
rooms[roomnumber].fd2points=0;
rooms[roomnumber].fd1points=0;
rooms[roomnumber].fd2life=3;
rooms[roomnumber].fd1life=3;

    struct fdinfor fdcur;
    fdcur.fd=ev->fd;
    fdcur.exist=true;
    fdcur.gaming=0;


    fdlist.push_back(fdcur);
printf("123\n");
 initmap();
   rooms[roomnumber].player[p0].pos.y = (Fieldy-4);//28-3=25
    rooms[roomnumber].player[p0].pos.x = (Fieldx-4);//72-3=69
    rooms[roomnumber].player[p0].direction = 1;
    rooms[roomnumber].player[p0].shoot = 0;
        rooms[roomnumber].player[p0].life = true;
    rooms[roomnumber].player[p0].spos.y= (Fieldy-4);
    rooms[roomnumber].player[p0].spos.x= (Fieldx-4);
  rooms[roomnumber].player[p0].sdir = 1;
   rooms[roomnumber].player[p0].sshoot = 0;



   rooms[roomnumber].player[p1].pos.y = 2;
    rooms[roomnumber].player[p1].pos.x = 2;
rooms[roomnumber].player[p1].direction = 2;
  rooms[roomnumber].player[p1].shoot = 0;
   rooms[roomnumber].player[p1].life = true;
   rooms[roomnumber].player[p1].spos.y = 2;
  rooms[roomnumber].player[p1].spos.x = 2;
  rooms[roomnumber].player[p1].sdir = 2;
   rooms[roomnumber].player[p1].sshoot = 0;

   rooms[roomnumber].player[h0].pos.y = 25;
   rooms[roomnumber].player[h0].pos.x = 36;
    rooms[roomnumber].player[h0].direction = 1;
    rooms[roomnumber].player[h0].shoot = 0;
       rooms[roomnumber].player[h0].life = true;
    rooms[roomnumber].player[h0].spos.y = 25;
    rooms[roomnumber].player[h0].spos.x = 36;
   rooms[roomnumber].player[h0].sdir = 1;
   rooms[roomnumber].player[h0].sshoot = 0;

    rooms[roomnumber].player[h1].pos.y = 2;
   rooms[roomnumber].player[h1].pos.x = 36;
   rooms[roomnumber].player[h1].direction = 2;
      rooms[roomnumber].player[h1].life = true;
    rooms[roomnumber].player[h1].shoot = 0;
  rooms[roomnumber].player[h1].spos.y = 2;
    rooms[roomnumber].player[h1].spos.x = 36;
   rooms[roomnumber].player[h1].sdir = 2;
   rooms[roomnumber].player[h1].sshoot = 0;     

             if (gameflag==0){
                //fdlist[i].gaming=1;
                rooms[roomnumber].fd1=ev->fd;
                usleep(1000);
                char room_str[10];
                sprintf(room_str, "%d", roomnumber);
                 send(ev->fd, room_str, strlen(room_str), 0);
                 usleep(10000);
                gameflag=1;
                   usleep(1000);

                send(ev->fd, "a", strlen("a"), 0);
                usleep(1000);

/*
for (int fh = 0; fh < Fieldy; fh++)
    {
       for (int fhw = 0; fhw < Fieldx; fhw++)
    {
       cout<<field[fh][fhw];

    }
cout<<endl;
    }
*/
 //for (int w = 0; w < Fieldy; w++)
   // {
        send(ev->fd, (char*)&field[0],strlen(field[0]),0);
        usleep(1000);
/*
        char redit[BUFLEN]
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"69"),"|"),"25"),"|"),"1"),"|"),"1"),"|")) ;
   send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"2"),"|"),"2"),"|"),"2"),"|"),"2"),"|")) ;
  send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"36"),"|"),"25"),"|"),"1"),"|"),"3"),"|")) ;
    send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"2"),"|"),"36"),"|"),"2"),"|"),"4"),"|")) ;
    send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);
*/
        
    //}
 // send(fdlist[0].fd, (char*)&field[0],strlen(field[0]),0);
   //     send(fdlist[1].fd, (char*)&field[1],strlen(field[1]),0);
   
                
            }
            else if (gameflag==1)
            {
                //fdlist[i].gaming=1;
                 rooms[roomnumber].fd2=ev->fd;
                 usleep(1000);
              /*   if (rooms[roomnumber].fd2 == rooms[roomnumber].fd1)
                 {
                      send(rooms[roomnumber].fd2, "woo|1|", strlen("woo|1|"), 0);
                       send(rooms[roomnumber].fd1, "woo|1|", strlen("woo|1|"), 0);

                     // close(g_events[checkpos].fd);   
                 }*/
                char room_str[10];
                sprintf(room_str, "%d", roomnumber);
                 send(ev->fd, room_str, strlen(room_str), 0);
                 usleep(10000);
                 roomnumber++;
                 usleep(1000);
                 gameflag=0;
                 usleep(10000);
                 send(ev->fd, "b", strlen("b"), 0);
                 usleep(10000);

               
    for (int fh = 0; fh < Fieldy; fh++)
    {
       for (int fhw = 0; fhw < Fieldx; fhw++)
    {
       cout<<field[fh][fhw];

    }
cout<<endl;
    }
//for (int w = 0; w < Fieldy; w++)
  //  {
          send(ev->fd, (char*)&field[0],strlen(field[0]),0);   
          usleep(10000);    

   /*       char redit[BUFLEN]
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"69"),"|"),"25"),"|"),"1"),"|"),"1"),"|")) ;
   send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"2"),"|"),"2"),"|"),"2"),"|"),"2"),"|")) ;
  send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"36"),"|"),"25"),"|"),"1"),"|"),"3"),"|")) ;
    send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);
    strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat("set","|"),"2"),"|"),"36"),"|"),"2"),"|"),"4"),"|")) ;
    send(ev->fd, redit, strlen(redit), 0);
  usleep(1000);

*/
       // send(fdlist[0].fd, (char*)&field[0],strlen(field[0]),0);
        //send(fdlist[1].fd, (char*)&field[1],strlen(field[1]),0);
   // }
     //            usleep(10000);   
                 flag=0;
                 
               
            }

 


        memset(ev->buf, 0, sizeof(ev->buf));
        ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     
        eventadd(g_efd, EPOLLIN, ev);                      

       return;


        }
  /*      else if (fu=="reset")
        {
            char *s = NULL;
  char flags[10];
   char loc_x_str[10], loc_y_str[10],dir[10];
  
  char team [10];
  
  char roomsr[10];
  int sendfd; 
  int myfd;
  int x=0;
  int y=0;
  int direction =0;

                            s = strtok(sv_msd, "|");
                            strcpy(flags, s);
printf("%s\n",flags );
                            s = strtok(NULL, "|");
                            strcpy(loc_x_str, s);

                            s = strtok(NULL, "|");
                            strcpy(loc_y_str, s);

                             x = atoi(loc_x_str);
                             y = atoi(loc_y_str);
printf("%d\n",x );
printf("%d\n",y );
                       

                            s = strtok(NULL, "|");
                            strcpy(team, s);

                            s = strtok(NULL, "|");
                            strcpy(dir, s);
                             direction = atoi(dir);

                            s = strtok(NULL, "|");
                            strcpy(roomsr, s);

                         printf("%s\n", roomsr);
                            int roomnumcur = atoi(roomsr);
                            string teamcur = team;
                            cout<<teamcur<<endl;
    if (teamcur=="a")
        {
           

         rooms[roomnumcur].player1.direction=direction;
         rooms[roomnumcur].player1.pos.y=y;
         rooms[roomnumcur].player1.pos.x=x;
        
        }
        else if (teamcur=="b")
        {
      
        rooms[roomnumcur].player2.direction=direction;
        rooms[roomnumcur].player2.pos.y=y;
        rooms[roomnumcur].player2.pos.x=x;
        }
          //y--;
       //player.direction=direction;
       //player.pos.y--;

        }
*/

        else if (fu.find("ghpm")==0)
        {
              char *s = NULL;
  char flags[BUFLEN];
  char loc_x_str[10], loc_y_str[10],dir[10],sh[10];
  char loc_x_str1[10], loc_y_str1[10],dir1[10],sh1[10];
  char loc_x_str2[10], loc_y_str2[10],dir2[10],sh2[10];

  char loc_x_str3[10], loc_y_str3[10],dir3[10],sh3[10];
  char loc_x_str4[10], loc_y_str4[10],dir4[10],sh4[10];
  char roomsr[10];
  int sendfd; 
  int myfd;
 // int x=0;
  //int y=0;
  //int direction =0;
  //int shoot=0;
  char redit1[BUFLEN];
  char redit2[BUFLEN];
  char redit3[BUFLEN];
  char redit4[BUFLEN];

                            s = strtok(sv_msd, "|");
                            strcpy(flags, s);

                            s = strtok(NULL, "|");
                            strcpy(roomsr, s);
                         
                            int roomnumcur = atoi(roomsr);

                            myfd=rooms[roomnumcur].fd2;
                            sendfd=rooms[roomnumcur].fd1;
                            char flag1[BUFLEN];
                            strcpy(flag1, flags);
                            char flag2[BUFLEN];
                            strcpy(flag2, flags);
                            printf("%s\n",flag2 );
                            char flag3[BUFLEN];
                             strcpy(flag3, flags);
                             printf("%s\n",flag3 );
                            char flag4[BUFLEN];
                            strcpy(flag4, flags);
                            printf("%s\n",flag4 );

  rooms[roomnumcur].player[h0].direction = rand() % 4 + 1;
       // t.direction = 3;
        rooms[roomnumcur].player[h0].shoot = rand() % 5;


int reserx=rooms[roomnumcur].player[h0].pos.x;
int resery=rooms[roomnumcur].player[h0].pos.y;
int resersdir=rooms[roomnumcur].player[h0].direction;
int resershoot=rooms[roomnumcur].player[h0].shoot;

/*rooms[roomnumcur].player[h0].spos.x=36;
rooms[roomnumcur].player[h0].spos.y=25;
rooms[roomnumcur].player[h0].sdir=1;
rooms[roomnumcur].player[h0].sshoot = 0;*/

        if (rooms[roomnumcur].player[h0].direction == 1)
        {
            rooms[roomnumcur].player[h0].pos.y--;
        }
        else if (rooms[roomnumcur].player[h0].direction == 2)
        {
            rooms[roomnumcur].player[h0].pos.y++;
        }
        else if (rooms[roomnumcur].player[h0].direction == 3)
        {
            rooms[roomnumcur].player[h0].pos.x++;
        }
        else if (rooms[roomnumcur].player[h0].direction == 4)
        {
            rooms[roomnumcur].player[h0].pos.x--;
        }
resetpos(rooms[roomnumcur].player[h0],roomnumcur,h0,reserx,resery,resersdir);




 rooms[roomnumcur].player[h1].direction = rand() % 4 + 1;
       // t.direction = 3;
        rooms[roomnumcur].player[h1].shoot = rand() % 5;

int reserxagain=rooms[roomnumcur].player[h1].pos.x;
int reseryagain=rooms[roomnumcur].player[h1].pos.y;
int resersdiragain=rooms[roomnumcur].player[h1].direction;
int resershootagain=rooms[roomnumcur].player[h1].shoot;

/*rooms[roomnumcur].player[h1].spos.x=2;
rooms[roomnumcur].player[h1].spos.y=36;
rooms[roomnumcur].player[h1].sdir=2;
rooms[roomnumcur].player[h1].sshoot = 0;

*/
        if (rooms[roomnumcur].player[h1].direction == 1)
        {
            rooms[roomnumcur].player[h1].pos.y--;
        }
        else if (rooms[roomnumcur].player[h1].direction == 2)
        {
            rooms[roomnumcur].player[h1].pos.y++;
        }
        else if (rooms[roomnumcur].player[h1].direction == 3)
        {
            rooms[roomnumcur].player[h1].pos.x++;
        }
        else if (rooms[roomnumcur].player[h1].direction == 4)
        {
            rooms[roomnumcur].player[h1].pos.x--;
        }

    resetpos(rooms[roomnumcur].player[h1],roomnumcur,h1,reserxagain,reseryagain,resersdiragain);



    sprintf(loc_x_str1, "%d", rooms[roomnumcur].player[h0].pos.x);
    sprintf(loc_y_str1, "%d", rooms[roomnumcur].player[h0].pos.y);
    sprintf(dir1, "%d", rooms[roomnumcur].player[h0].direction);
    sprintf(sh1, "%d", rooms[roomnumcur].player[h0].shoot);

    sprintf(loc_x_str2, "%d", rooms[roomnumcur].player[h1].pos.x);
    sprintf(loc_y_str2, "%d", rooms[roomnumcur].player[h1].pos.y);
    sprintf(dir2, "%d", rooms[roomnumcur].player[h1].direction);
    sprintf(sh2, "%d", rooms[roomnumcur].player[h1].shoot);

    sprintf(loc_x_str3, "%d", rooms[roomnumcur].player[h0].pos.x);
    sprintf(loc_y_str3, "%d", rooms[roomnumcur].player[h0].pos.y);
    sprintf(dir3, "%d", rooms[roomnumcur].player[h0].direction);
    sprintf(sh3, "%d", rooms[roomnumcur].player[h0].shoot);

    sprintf(loc_x_str4, "%d", rooms[roomnumcur].player[h1].pos.x);
    sprintf(loc_y_str4, "%d", rooms[roomnumcur].player[h1].pos.y);
    sprintf(dir4, "%d", rooms[roomnumcur].player[h1].direction);
    sprintf(sh4, "%d", rooms[roomnumcur].player[h1].shoot);


     char iden1[10];
    char iden2[10];
    char iden3[10];
    char iden4[10];
    strcpy(iden1,"1") ;
    strcpy(iden2,"2") ;
 strcpy(iden3,"1") ; 
 strcpy(iden4,"2") ;


    strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),loc_x_str1),"|"),loc_y_str1),"|"),dir1),"|"),sh1),"|"),iden1),"|")) ;
    strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),loc_x_str2),"|"),loc_y_str2),"|"),dir2),"|"),sh2),"|"),iden2),"|")) ;
   
   strcpy(redit3,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag3,"|"),loc_x_str3),"|"),loc_y_str3),"|"),dir3),"|"),sh3),"|"),iden3),"|")) ;
    strcpy(redit4,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag4,"|"),loc_x_str4),"|"),loc_y_str4),"|"),dir4),"|"),sh4),"|"),iden4),"|")) ;
   

 printf("%s\n",redit1);  
 
     send(myfd, redit1, strlen(redit1), 0);
     usleep(10000);
     send(myfd, redit2, strlen(redit2), 0);
usleep(10000);
  
    printf("%s\n",redit2);  

 
     send(sendfd, redit3, strlen(redit3), 0);
     usleep(10000);
     send(sendfd, redit4, strlen(redit4), 0);
     usleep(10000);
 printf("%s\n",redit3);  
  printf("%s\n",redit4);  


        memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件


                  
        }
        else{

  char *s = NULL;
  char flags[10];
  
  char team [10];
  char team1[10],team2[10];
  char loc_x_str[10], loc_y_str[10],dir[10];
  char loc_x_str1[10], loc_y_str1[10],dir1[10];
  char loc_x_str2[10], loc_y_str2[10],dir2[10];
  char roomsr[10],room_str1[10],room_str2[10];
  int sendfd; 
  int myfd;
  int x=0;
  int y=0;
  int direction =0;
  int num=0;

                            s = strtok(sv_msd, "|");
                            strcpy(flags, s);
if (strcmp(flags,"h")==0||strcmp(flags,"home")==0||strcmp(flags,"hai")==0){
         s = strtok(NULL, "|");
                            strcpy(loc_x_str, s);

                            s = strtok(NULL, "|");
                            strcpy(loc_y_str, s);

                             x = atoi(loc_x_str);
                             y = atoi(loc_y_str);
}
                       

                            s = strtok(NULL, "|");
                            strcpy(team, s);
if (strcmp(flags,"h")==0||strcmp(flags,"hai")==0||strcmp(flags,"home")==0){
                            s = strtok(NULL, "|");
                            strcpy(dir, s);
                             num = atoi(dir);
}
                            s = strtok(NULL, "|");
                            strcpy(roomsr, s);





                         
                            int roomnumcur = atoi(roomsr);
                           

        string checks = flags;
        string teamcur = team;

        if (teamcur=="a")
        {
            myfd=rooms[roomnumcur].fd1;
            sendfd=rooms[roomnumcur].fd2;

        /*    player=rooms[roomnumcur].player1;
            helper=rooms[roomnumcur].help1;
            enemy=rooms[roomnumcur].player2;
            enemyhelper=rooms[roomnumcur].help2;
*/
            printf("send to %d\n",sendfd );

        }
        else if (teamcur=="b")
        {
            myfd=rooms[roomnumcur].fd2;
            sendfd=rooms[roomnumcur].fd1;
/*
             player=rooms[roomnumcur].player2;
            helper=rooms[roomnumcur].help2;
            enemy=rooms[roomnumcur].player1;
            enemyhelper=rooms[roomnumcur].help1;
            */
               printf("send to %d\n",sendfd );
        }
                else if (teamcur=="ai")
        {
            myfd=rooms[roomnumcur].fd1;
            sendfd=rooms[roomnumcur].fd2;
/*
             player=rooms[roomnumcur].player2;
            helper=rooms[roomnumcur].help2;
            enemy=rooms[roomnumcur].player1;
            enemyhelper=rooms[roomnumcur].help1;
            */
               printf("send to %d\n",sendfd );
        }
                else if (teamcur=="bi")
        {
            myfd=rooms[roomnumcur].fd2;
            sendfd=rooms[roomnumcur].fd1;
/*
             player=rooms[roomnumcur].player2;
            helper=rooms[roomnumcur].help2;
            enemy=rooms[roomnumcur].player1;
            enemyhelper=rooms[roomnumcur].help1;
            */
               printf("send to %d\n",sendfd );
        }

//memcpy(loc_x_str1,loc_x_str,strlen(loc_x_str));
//memcpy(loc_x_str2,loc_x_str,strlen(loc_x_str));
//memcpy(loc_y_str1,loc_y_str,strlen(loc_y_str));
//memcpy(loc_y_str2,loc_y_str,strlen(loc_y_str));
//memcpy(dir1,dir,strlen(dir));
//memcpy(dir2,dir,strlen(dir));
//memcpy(team1,team,strlen(team));
//memcpy(team2,team,strlen(team));



if (checks=="w"||checks=="zzz"||checks=="s"||checks=="d"||checks=="l"){

    char redit1[BUFLEN];
    char redit2[BUFLEN];
    char flag1[10];
   strcpy(flag1,flags);
  char flag2[10];
     strcpy(flag2,flags);
 
  printf("%s\n",flags );
 printf("%s\n",flag1);

 printf("%s\n",flag2);

int reservedirection;
 int reservex;
 int reservey;


 if (strcmp(flags, "l") == 0){
strcpy(flag1,"changeok");
strcpy(flag2,"changeok");
  char iden1[10];
    char iden2[10];
    strcpy(iden1,"1") ;//to me
    strcpy(iden2,"2") ;// to you


    if (teamcur=="a")
        {

    int swpy = rooms[roomnumber].player[p0].pos.y;
    int swpx =  rooms[roomnumber].player[p0].pos.x;
    int swpdirection = rooms[roomnumber].player[p0].direction;
    int swpshoot = rooms[roomnumber].player[p0].shoot;
    int swpsy = rooms[roomnumber].player[p0].spos.y;
    int swpsx = rooms[roomnumber].player[p0].spos.x;
    int swpsdir = rooms[roomnumber].player[p0].sdir;
    int swpsshoot =  rooms[roomnumber].player[p0].sshoot;
    int swplife = rooms[roomnumber].player[p0].life;


   rooms[roomnumber].player[p0].pos.y = rooms[roomnumber].player[h0].pos.y;
    rooms[roomnumber].player[p0].pos.x =  rooms[roomnumber].player[h0].pos.x;
    rooms[roomnumber].player[p0].direction = rooms[roomnumber].player[h0].direction;
    rooms[roomnumber].player[p0].shoot = rooms[roomnumber].player[h0].shoot;
    rooms[roomnumber].player[p0].spos.y = rooms[roomnumber].player[h0].spos.y;
    rooms[roomnumber].player[p0].spos.x = rooms[roomnumber].player[h0].spos.x;
    rooms[roomnumber].player[p0].sdir = rooms[roomnumber].player[h0].sdir;
    rooms[roomnumber].player[p0].sshoot =  rooms[roomnumber].player[h0].sshoot;
    rooms[roomnumber].player[p0].life = rooms[roomnumber].player[h0].life;


    rooms[roomnumber].player[h0].pos.y = swpy;
    rooms[roomnumber].player[h0].pos.x =  swpx;
    rooms[roomnumber].player[h0].direction = swpdirection;
    rooms[roomnumber].player[h0].shoot = swpshoot;
    rooms[roomnumber].player[h0].spos.y = swpsy;
    rooms[roomnumber].player[h0].spos.x = swpsx;
    rooms[roomnumber].player[h0].sdir = swpsdir;
    rooms[roomnumber].player[h0].sshoot =  swpsshoot;
    rooms[roomnumber].player[h0].life = swplife;



           int swp=p0;
           p0=h0;
           h0=swp;

            


        }
        else if (teamcur=="b")
        {

    int swpy = rooms[roomnumber].player[p1].pos.y;
    int swpx =  rooms[roomnumber].player[p1].pos.x;
    int swpdirection = rooms[roomnumber].player[p1].direction;
    int swpshoot = rooms[roomnumber].player[p1].shoot;
    int swpsy = rooms[roomnumber].player[p1].spos.y;
    int swpsx = rooms[roomnumber].player[p1].spos.x;
    int swpsdir = rooms[roomnumber].player[p1].sdir;
    int swpsshoot =  rooms[roomnumber].player[p1].sshoot;
    int swplife = rooms[roomnumber].player[p1].life;


    rooms[roomnumber].player[p1].pos.y = rooms[roomnumber].player[h1].pos.y;
    rooms[roomnumber].player[p1].pos.x =  rooms[roomnumber].player[h1].pos.x;
    rooms[roomnumber].player[p1].direction = rooms[roomnumber].player[h1].direction;
    rooms[roomnumber].player[p1].shoot = rooms[roomnumber].player[h1].shoot;
    rooms[roomnumber].player[p1].spos.y = rooms[roomnumber].player[h1].spos.y;
    rooms[roomnumber].player[p1].spos.x = rooms[roomnumber].player[h1].spos.x;
    rooms[roomnumber].player[p1].sdir = rooms[roomnumber].player[h1].sdir;
    rooms[roomnumber].player[p1].sshoot =  rooms[roomnumber].player[h1].sshoot;
    rooms[roomnumber].player[p1].life = rooms[roomnumber].player[h1].life;


    rooms[roomnumber].player[h1].pos.y = swpy;
    rooms[roomnumber].player[h1].pos.x =  swpx;
    rooms[roomnumber].player[h1].direction = swpdirection;
    rooms[roomnumber].player[h1].shoot = swpshoot;
    rooms[roomnumber].player[h1].spos.y = swpsy;
    rooms[roomnumber].player[h1].spos.x = swpsx;
    rooms[roomnumber].player[h1].sdir = swpsdir;
    rooms[roomnumber].player[h1].sshoot =  swpsshoot;
    rooms[roomnumber].player[h1].life = swplife;

      
           int swp=p1;
           p1=h1;
           h1=swp;

              

           }
          //y--;
  
           strcpy(redit1,strcat(strcat(strcat(flag1,"|"),iden1),"|")) ;

           strcpy(redit2,strcat(strcat(strcat(flag2,"|"),iden2),"|")) ;

       

 //printf("%s\n",redit1);  
 
     send(myfd, redit1, strlen(redit1), 0);
    
    usleep(10000);
  
    printf("%s\n",redit2);  

 
     send(sendfd, redit2, strlen(redit2), 0);



printf("change C[%d]to [%d]:%s\n", myfd,myfd, redit1);
printf("change C[%d]to [%d]:%s\n", myfd,sendfd, redit2);
  // strcpy(ev->buf,redit);
  // ev->fd=sendfd;
  // ev->len
   
        memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件
        return;

 }


 if (strcmp(flags, "w") == 0){

    if (teamcur=="a")
        {
           reservedirection= rooms[roomnumcur].player[p0].direction;
           reservex= rooms[roomnumcur].player[p0].pos.x;
           reservey= rooms[roomnumcur].player[p0].pos.y;

         rooms[roomnumcur].player[p0].direction=1;
         rooms[roomnumcur].player[p0].pos.y--;
          resetpos(rooms[roomnumcur].player[p0],roomnumcur,p0,reservex,reservey,reservedirection);
        }
        else if (teamcur=="b")
        {
      
           reservedirection= rooms[roomnumcur].player[p1].direction;
           reservex= rooms[roomnumcur].player[p1].pos.x;
           reservey= rooms[roomnumcur].player[p1].pos.y;

        rooms[roomnumcur].player[p1].direction=1;
        rooms[roomnumcur].player[p1].pos.y--;
         resetpos(rooms[roomnumcur].player[p1],roomnumcur,p1,reservex,reservey,reservedirection);
        }
          //y--;

 }
 if (strcmp(flags, "s") == 0)
 {
            if (teamcur=="a")
        {
             reservedirection= rooms[roomnumcur].player[p0].direction;
           reservex= rooms[roomnumcur].player[p0].pos.x;
           reservey= rooms[roomnumcur].player[p0].pos.y;

         rooms[roomnumcur].player[p0].direction=2;
         rooms[roomnumcur].player[p0].pos.y++;   
          resetpos(rooms[roomnumcur].player[p0],roomnumcur,p0,reservex,reservey,reservedirection);

        }
        else if (teamcur=="b")
        { 
            reservedirection= rooms[roomnumcur].player[p1].direction;
           reservex= rooms[roomnumcur].player[p1].pos.x;
           reservey= rooms[roomnumcur].player[p1].pos.y;

        rooms[roomnumcur].player[p1].direction=2;
        rooms[roomnumcur].player[p1].pos.y++;
         resetpos(rooms[roomnumcur].player[p1],roomnumcur,p1,reservex,reservey,reservedirection);
          
        }


 
 }
 if (strcmp(flags, "d") == 0)
 {
             if (teamcur=="a")
        {
           reservedirection= rooms[roomnumcur].player[p0].direction;
           reservex= rooms[roomnumcur].player[p0].pos.x;
           reservey= rooms[roomnumcur].player[p0].pos.y;

         rooms[roomnumcur].player[p0].direction=3;
         rooms[roomnumcur].player[p0].pos.x++;   
 resetpos(rooms[roomnumcur].player[p0],roomnumcur,p0,reservex,reservey,reservedirection);
        }
        else if (teamcur=="b")
        {
          reservedirection= rooms[roomnumcur].player[p1].direction;
           reservex= rooms[roomnumcur].player[p1].pos.x;
           reservey= rooms[roomnumcur].player[p1].pos.y;

        rooms[roomnumcur].player[p1].direction=3;
        rooms[roomnumcur].player[p1].pos.x++;
           resetpos(rooms[roomnumcur].player[p1],roomnumcur,p1,reservex,reservey,reservedirection);

        }


 }
 if (strcmp(flags, "zzz") == 0)
 {
         if (teamcur=="a")
        {
             reservedirection= rooms[roomnumcur].player[p0].direction;
           reservex= rooms[roomnumcur].player[p0].pos.x;
           reservey= rooms[roomnumcur].player[p0].pos.y;
           
         rooms[roomnumcur].player[p0].direction=4;
         rooms[roomnumcur].player[p0].pos.x--;   
 resetpos(rooms[roomnumcur].player[p0],roomnumcur,p0,reservex,reservey,reservedirection);
        }
        else if (teamcur=="b")
        {

           reservedirection= rooms[roomnumcur].player[p1].direction;
           reservex= rooms[roomnumcur].player[p1].pos.x;
           reservey= rooms[roomnumcur].player[p1].pos.y;

        rooms[roomnumcur].player[p1].direction=4;
        rooms[roomnumcur].player[p1].pos.x--;
           resetpos(rooms[roomnumcur].player[p1],roomnumcur,p1,reservex,reservey,reservedirection);
        }

 }

//



    char iden1[10];
    char iden2[10];
   strcpy(iden1,"1") ;//to me
     strcpy(iden2,"2") ;// to you



 if (teamcur=="a")
        {
    sprintf(loc_x_str1, "%d", rooms[roomnumcur].player[p0].pos.x);
    sprintf(loc_y_str1, "%d", rooms[roomnumcur].player[p0].pos.y);
    sprintf(dir1, "%d", rooms[roomnumcur].player[p0].direction); 
    sprintf(loc_x_str2, "%d", rooms[roomnumcur].player[p0].pos.x);
    sprintf(loc_y_str2, "%d", rooms[roomnumcur].player[p0].pos.y);
    sprintf(dir2, "%d", rooms[roomnumcur].player[p0].direction);//tell both  my input

        }
        else if (teamcur=="b")
        {
    sprintf(loc_x_str1, "%d", rooms[roomnumcur].player[p1].pos.x);
    sprintf(loc_y_str1, "%d", rooms[roomnumcur].player[p1].pos.y);
    sprintf(dir1, "%d", rooms[roomnumcur].player[p1].direction); 
    sprintf(loc_x_str2, "%d", rooms[roomnumcur].player[p1].pos.x);
    sprintf(loc_y_str2, "%d", rooms[roomnumcur].player[p1].pos.y);
    sprintf(dir2, "%d", rooms[roomnumcur].player[p1].direction);//tell both  my input

        }


    


   printf("%s\n",flags );
 printf("%s\n",flag1);

 printf("%s\n",flag2);

   strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),loc_x_str1),"|"),loc_y_str1),"|"),dir1),"|"),iden1),"|")) ;
   // sprintf(len_str1, "%d",  strlen(redit1));

  //  strcpy(redit1,strcat(len_str,strcat("|",redit1))) ;
 

  //  string strr("ABCDEFG");
    //int length = strr.copy(ev->buf, 9);
    //ev->buf[length] = '\0';



  strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),loc_x_str2),"|"),loc_y_str2),"|"),dir2),"|"),iden2),"|")) ;

 printf("%s\n",redit1);  
 
     send(myfd, redit1, strlen(redit1), 0);

  
    printf("%s\n",redit2);  

 
     send(sendfd, redit2, strlen(redit2), 0);



printf("move C[%d]to [%d]:%s\n", myfd,myfd, redit1);
printf("move C[%d]to [%d]:%s\n", myfd,sendfd, redit2);
  // strcpy(ev->buf,redit);
  // ev->fd=sendfd;
  // ev->len
   
        memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件

return;
}

else if (checks=="f"){
    char redit1[BUFLEN];
    char redit2[BUFLEN];
    char flag1[10];
   strcpy(flag1,flags);
  char flag2[10];
     strcpy(flag2,flags);
 int xx;
 int yy;
  printf("%s\n",flags );
 printf("%s\n",flag1);

 printf("%s\n",flag2);
    char iden1[10];
    char iden2[10];
  

if (teamcur=="a")
{
    if(rooms[roomnumcur].player[p0].direction == 1)
    {
        xx = rooms[roomnumcur].player[p0].pos.x;
        yy = rooms[roomnumcur].player[p0].pos.y-2;
    }
    else if(rooms[roomnumcur].player[p0].direction == 2)
    {
       xx = rooms[roomnumcur].player[p0].pos.x;
       yy = rooms[roomnumcur].player[p0].pos.y+2;
    }
    else if(rooms[roomnumcur].player[p0].direction == 3)
    {
        xx =rooms[roomnumcur].player[p0].pos.x+2;
        yy =rooms[roomnumcur].player[p0].pos.y;
    }
    else if(rooms[roomnumcur].player[p0].direction == 4)
    {
        xx =rooms[roomnumcur].player[p0].pos.x-2;
       yy =rooms[roomnumcur].player[p0].pos.y;
    }
    sprintf(loc_x_str1, "%d", xx);
    sprintf(loc_y_str1, "%d", yy);
    sprintf(dir1, "%d", rooms[roomnumcur].player[p0].direction);

    sprintf(loc_x_str2, "%d", xx);
    sprintf(loc_y_str2, "%d", yy);
    sprintf(dir2, "%d", rooms[roomnumcur].player[p0].direction);
     strcpy(iden1,"1") ;
    strcpy(iden2,"2") ;
    strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),loc_x_str1),"|"),loc_y_str1),"|"),dir1),"|"),iden1),"|")) ;
   
    strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),loc_x_str2),"|"),loc_y_str2),"|"),dir2),"|"),iden2),"|")) ;

}
else if (teamcur=="b")
{
     if(rooms[roomnumcur].player[p1].direction == 1)
    {
        xx = rooms[roomnumcur].player[p1].pos.x;
        yy = rooms[roomnumcur].player[p1].pos.y-2;
    }
    else if(rooms[roomnumcur].player[p1].direction == 2)
    {
       xx = rooms[roomnumcur].player[p1].pos.x;
       yy = rooms[roomnumcur].player[p1].pos.y+2;
    }
    else if(rooms[roomnumcur].player[p1].direction == 3)
    {
        xx =rooms[roomnumcur].player[p1].pos.x+2;
        yy =rooms[roomnumcur].player[p1].pos.y;
    }
    else if(rooms[roomnumcur].player[p1].direction == 4)
    {
        xx =rooms[roomnumcur].player[p1].pos.x-2;
       yy =rooms[roomnumcur].player[p1].pos.y;
    }

    sprintf(loc_x_str1, "%d", xx);
    sprintf(loc_y_str1, "%d", yy);
    sprintf(dir1, "%d", rooms[roomnumcur].player[p1].direction);

    sprintf(loc_x_str2, "%d", xx);
    sprintf(loc_y_str2, "%d", yy);
    sprintf(dir2, "%d", rooms[roomnumcur].player[p1].direction);

     strcpy(iden1,"1") ;
    strcpy(iden2,"2") ;
    strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),loc_x_str1),"|"),loc_y_str1),"|"),dir1),"|"),iden1),"|")) ;
   
    strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),loc_x_str2),"|"),loc_y_str2),"|"),dir2),"|"),iden2),"|")) ;


}
else if (teamcur=="ai")
{ 
    if(rooms[roomnumcur].player[h0].direction == 1)
    {
        xx = rooms[roomnumcur].player[h0].pos.x;
        yy = rooms[roomnumcur].player[h0].pos.y-2;
    }
    else if(rooms[roomnumcur].player[h0].direction == 2)
    {
       xx = rooms[roomnumcur].player[h0].pos.x;
       yy = rooms[roomnumcur].player[h0].pos.y+2;
    }
    else if(rooms[roomnumcur].player[h0].direction == 3)
    {
        xx =rooms[roomnumcur].player[h0].pos.x+2;
        yy =rooms[roomnumcur].player[h0].pos.y;
    }
    else if(rooms[roomnumcur].player[h0].direction == 4)
    {
        xx =rooms[roomnumcur].player[h0].pos.x-2;
       yy =rooms[roomnumcur].player[h0].pos.y;
    } 


    sprintf(loc_x_str1, "%d", xx);
    sprintf(loc_y_str1, "%d", yy);
    sprintf(dir1, "%d", rooms[roomnumcur].player[h0].direction);

    sprintf(loc_x_str2, "%d", xx);
    sprintf(loc_y_str2, "%d", yy);
    sprintf(dir2, "%d", rooms[roomnumcur].player[h0].direction);
    strcpy(iden1,"3") ;
    strcpy(iden2,"4") ;
    strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),loc_x_str1),"|"),loc_y_str1),"|"),dir1),"|"),iden1),"|")) ;
   
    strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),loc_x_str2),"|"),loc_y_str2),"|"),dir2),"|"),iden2),"|")) ;


}
else if (teamcur=="bi")
{
       if(rooms[roomnumcur].player[h1].direction == 1)
    {
        xx = rooms[roomnumcur].player[h1].pos.x;
        yy = rooms[roomnumcur].player[h1].pos.y-2;
    }
    else if(rooms[roomnumcur].player[h1].direction == 2)
    {
       xx = rooms[roomnumcur].player[h1].pos.x;
       yy = rooms[roomnumcur].player[h1].pos.y+2;
    }
    else if(rooms[roomnumcur].player[h1].direction == 3)
    {
        xx =rooms[roomnumcur].player[h1].pos.x+2;
        yy =rooms[roomnumcur].player[h1].pos.y;
    

    }
    else if(rooms[roomnumcur].player[h1].direction == 4)
    {
        xx =rooms[roomnumcur].player[h1].pos.x-2;
        yy =rooms[roomnumcur].player[h1].pos.y;
   
    }

    sprintf(loc_x_str1, "%d", xx);
    sprintf(loc_y_str1, "%d", yy);
    sprintf(dir1, "%d", rooms[roomnumcur].player[h1].direction);

    sprintf(loc_x_str2, "%d", xx);
    sprintf(loc_y_str2, "%d", yy);
    sprintf(dir2, "%d", rooms[roomnumcur].player[h1].direction);

    strcpy(iden1,"3") ;
    strcpy(iden2,"4") ;
    strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),loc_x_str1),"|"),loc_y_str1),"|"),dir1),"|"),iden1),"|")) ;
   
    strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),loc_x_str2),"|"),loc_y_str2),"|"),dir2),"|"),iden2),"|")) ;


}


  //strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),loc_x_str),"|"),loc_y_str),"|"),dir),"|")) ;
    //  send(sendfd, redit, strlen(redit), 0);
      //    printf("fire C[%d]to [%d]:%s\n", fd,sendfd, redit);
 
  //  string strr("ABCDEFG");
    //int length = strr.copy(ev->buf, 9);
    //ev->buf[length] = '\0';
  // strcpy(ev->buf,redit);
  
    //    eventset(ev, sendfd, senddata, ev);                     //设置该 fd 对应的回调函数为 senddata
      //  eventadd(g_efd, EPOLLOUT, ev);                      //将fd加入红黑树g_efd中,监听其写事件

    
  
printf("%s\n",flags );
 printf("%s\n",flag1);

 printf("%s\n",flag2);

  // strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),loc_x_str1),"|"),loc_y_str1),"|"),dir1),"|"),iden1),"|")) ;
   // sprintf(len_str1, "%d",  strlen(redit1));

  //  strcpy(redit1,strcat(len_str,strcat("|",redit1))) ;
 

  //  string strr("ABCDEFG");
    //int length = strr.copy(ev->buf, 9);
    //ev->buf[length] = '\0';



  //strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),loc_x_str2),"|"),loc_y_str2),"|"),dir2),"|"),iden2),"|")) ;

 //printf("%s\n",redit1);  
 
     send(myfd, redit1, strlen(redit1), 0);

     usleep(10000);

    // sleep(0.5)
      usleep(10000);
    
     send(sendfd, redit2, strlen(redit2), 0);
     
//usleep(100000);
printf("fire C[%d]to [%d]:%s\n", myfd,myfd, redit1);
printf("fire C[%d]to [%d]:%s\n", myfd,sendfd, redit2);
   memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件
return;

}

else if (checks=="h"||checks=="home"||checks=="hai"){

    char point_str[10];
    char reser_point_str[10];

   char life_str[10];
   char reser_life_str[10];

   char host[10];
     strcpy(host, "0");

     if ((checks=="hai")&&(sendfd==rooms[roomnumcur].fd2))
        {
rooms[roomnumcur].player[h1].pos.x = rooms[roomnumcur].player[h1].spos.x;
rooms[roomnumcur].player[h1].pos.y = rooms[roomnumcur].player[h1].spos.y;
rooms[roomnumcur].player[h1].direction = rooms[roomnumcur].player[h1].sdir;
rooms[roomnumcur].player[h1].shoot = rooms[roomnumcur].player[h1].sshoot;
  sprintf(point_str, "%d", rooms[roomnumcur].fd1points);
    printf("%s\n",point_str );
      
    sprintf(life_str, "%d", rooms[roomnumcur].fd2life);
     printf("%s\n",life_str );

    sprintf(reser_point_str, "%d", rooms[roomnumcur].fd2points);
 printf("%s\n",reser_point_str );

    sprintf(reser_life_str, "%d", rooms[roomnumcur].fd1life);
 printf("%s\n",reser_life_str );
        }

    else if (sendfd==rooms[roomnumcur].fd2)//fd2 is hurt
    {
        if(rooms[roomnumcur].fd2life>0&&(checks=="h")){
             rooms[roomnumcur].fd1points++;//he is hurt,my point increase
         rooms[roomnumcur].fd2life--;//he is hurt,his life decrease

        }

        
             
    sprintf(point_str, "%d", rooms[roomnumcur].fd1points);
    printf("%s\n",point_str );
      
    sprintf(life_str, "%d", rooms[roomnumcur].fd2life);
     printf("%s\n",life_str );

    sprintf(reser_point_str, "%d", rooms[roomnumcur].fd2points);
 printf("%s\n",reser_point_str );

    sprintf(reser_life_str, "%d", rooms[roomnumcur].fd1life);
 printf("%s\n",reser_life_str );


rooms[roomnumcur].player[p1].pos.x=rooms[roomnumcur].player[p1].spos.x;
rooms[roomnumcur].player[p1].pos.y=rooms[roomnumcur].player[p1].spos.y;
rooms[roomnumcur].player[p1].direction=rooms[roomnumcur].player[p1].sdir;
rooms[roomnumcur].player[p1].shoot=rooms[roomnumcur].player[p1].sshoot;


    }


else if ((checks=="hai")&&(sendfd==rooms[roomnumcur].fd1))
        {
rooms[roomnumcur].player[h0].pos.x = rooms[roomnumcur].player[h0].spos.x;
rooms[roomnumcur].player[h0].pos.y = rooms[roomnumcur].player[h0].spos.y;
rooms[roomnumcur].player[h0].direction = rooms[roomnumcur].player[h0].sdir;
rooms[roomnumcur].player[h0].shoot = rooms[roomnumcur].player[h0].sshoot;
 sprintf(point_str, "%d", rooms[roomnumcur].fd2points);
 printf("%s\n",point_str );
    sprintf(life_str, "%d", rooms[roomnumcur].fd1life);
    printf("%s\n",life_str );

 sprintf(reser_point_str, "%d", rooms[roomnumcur].fd1points);
printf("%s\n",reser_point_str );
    sprintf(reser_life_str, "%d", rooms[roomnumcur].fd2life);
printf("%s\n",reser_life_str );
        }

    else if (sendfd==rooms[roomnumcur].fd1)//fd1 is hurt
    {
        if (rooms[roomnumcur].fd1life>0&&(checks=="h"))
        {
             rooms[roomnumcur].fd2points++;
         rooms[roomnumcur].fd1life--;
        }

        
            
    sprintf(point_str, "%d", rooms[roomnumcur].fd2points);
 printf("%s\n",point_str );
    sprintf(life_str, "%d", rooms[roomnumcur].fd1life);
    printf("%s\n",life_str );

 sprintf(reser_point_str, "%d", rooms[roomnumcur].fd1points);
printf("%s\n",reser_point_str );
    sprintf(reser_life_str, "%d", rooms[roomnumcur].fd2life);
printf("%s\n",reser_life_str );


rooms[roomnumcur].player[p0].pos.x=rooms[roomnumcur].player[p0].spos.x;
rooms[roomnumcur].player[p0].pos.y=rooms[roomnumcur].player[p0].spos.y;
rooms[roomnumcur].player[p0].direction=rooms[roomnumcur].player[p0].sdir;
rooms[roomnumcur].player[p0].shoot=rooms[roomnumcur].player[p0].sshoot;

    }

   


    char redit1[BUFLEN];
    char redit2[BUFLEN];
    char flag1[10];
   strcpy(flag1,flags);
  char flag2[10];
     strcpy(flag2,flags);
 
  printf("%s\n",flags );
 printf("%s\n",flag1);

 printf("%s\n",flag2);


  //  char redit[BUFLEN];

     char iden1[10];
    char iden2[10];
   strcpy(iden1,"1") ;
     strcpy(iden2,"2") ;
     sprintf(dir1, "%d", num);//which bullet attack his shit tank
printf("%d\n",num );

   
     sprintf(dir2, "%d", num);
printf("!!!%d\n",roomnumcur );

    if (rooms[roomnumcur].fd1life==0||(y==14&&x==Fieldx-3))//die 
    {
        strcpy(flag1, "woo");
        printf("%s\n",flag1 );
        strcpy(flag2, "woo");
        printf("%s\n",flag2 );
        //sendfd=rooms[roomnumcur].fd2;
 strcpy(redit1,strcat(strcat(strcat(flag1,"|"),"1"),"|"));

 strcpy(redit2,strcat(strcat(strcat(flag2,"|"),"2"),"|"));//win

send(rooms[roomnumcur].fd1, redit1, strlen(redit1), 0);

send(rooms[roomnumcur].fd2, redit2, strlen(redit2), 0); 

 
     memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件
return;
    }
    else if (rooms[roomnumcur].fd2life==0||(x==2&&y==14))
    {
         strcpy(flag1, "woo");
         printf("%s\n", flag1);
        strcpy(flag2, "woo");
        printf("%s\n", flag2);
        //sendfd=rooms[roomnumcur].fd2;
 strcpy(redit1,strcat(strcat(strcat(flag1,"|"),"2"),"|"));//win

 printf("%s\n",redit1 );

 strcpy(redit2,strcat(strcat(strcat(flag2,"|"),"1"),"|"));
printf("%s\n",redit2 );
send(rooms[roomnumcur].fd1, redit1, strlen(redit1), 0);

send(rooms[roomnumcur].fd2, redit2, strlen(redit2), 0); 

     memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件
    return;
    }


    else{


  strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),point_str),"|"),reser_life_str),"|"),dir1),"|"),iden1),"|")) ;

  strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),reser_point_str),"|"),life_str),"|"),dir2),"|"),iden2),"|")) ;

 printf("%s\n",redit1);  



 
     send(myfd, redit1, strlen(redit1), 0);//other message to me,my message to other


   printf("%s\n",redit2);  

 
     send(sendfd, redit2, strlen(redit2), 0);


      //  strcpy(redit,strcat(strcat(strcat(strcat(strcat(flags,"|"),point_str),"|"),life_str),"|")) ;
   //  strcpy(redit,strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),point_str),"|"),life_str),"|"),dir),"|")) ;
  
   // send(sendfd, redit, strlen(redit), 0);
          printf("you are hurt C[%d]to [%d]:%s\n", myfd,myfd, redit1);
           printf("you are hurt C[%d]to [%d]:%s\n", myfd,sendfd, redit2);

   memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件

return;
    }

   

        }

   
  //  string strr("ABCDEFG");
    //int length = strr.copy(ev->buf, 9);
    //ev->buf[length] = '\0';
  // strcpy(ev->buf,redit);
  
    //    eventset(ev, sendfd, senddata, ev);                     //设置该 fd 对应的回调函数为 senddata
     //   eventadd(g_efd, EPOLLOUT, ev);                      //将fd加入红黑树g_efd中,监听其写事件

else if (checks=="o"){//tell others my state, for judge
   char point_str[10];
   char life_str[10];

     char reser_point_str[10];

 
   char reser_life_str[10];

    if (sendfd==rooms[roomnumcur].fd2)//fd1 l
    {
        if (rooms[roomnumcur].fd1points>0)
        {
             rooms[roomnumcur].fd1points--;
         // rooms[roomnumcur].fd1points--;
         rooms[roomnumcur].fd1life++;
        }
        
             
    sprintf(point_str, "%d", rooms[roomnumcur].fd1points);
    sprintf(life_str, "%d", rooms[roomnumcur].fd1life);
    sprintf(reser_point_str, "%d", rooms[roomnumcur].fd2points);
    sprintf(reser_life_str, "%d", rooms[roomnumcur].fd2life);


    }
    else if (sendfd==rooms[roomnumcur].fd1)// fd2 l
    {
        if (rooms[roomnumcur].fd2points>0)
        {
            rooms[roomnumcur].fd2points--;
        // rooms[roomnumcur].fd2points--;
         rooms[roomnumcur].fd2life++;
            
        }
         
    sprintf(point_str, "%d", rooms[roomnumcur].fd2points);
    sprintf(life_str, "%d", rooms[roomnumcur].fd2life);

    sprintf(reser_point_str, "%d", rooms[roomnumcur].fd1points);

    sprintf(reser_life_str, "%d", rooms[roomnumcur].fd1life);


    }



    char redit1[BUFLEN];
    char redit2[BUFLEN];
    char flag1[10];
   strcpy(flag1,flags);
  char flag2[10];
     strcpy(flag2,flags);
 
  printf("%s\n",flags );
 printf("%s\n",flag1);

 printf("%s\n",flag2);


  //  char redit[BUFLEN];

    char iden1[10];
    char iden2[10];
    strcpy(iden1,"1") ;
    strcpy(iden2,"2") ;
    sprintf(dir1, "%d", num);

   
     sprintf(dir2, "%d", num);


   //char redit[BUFLEN];


       // strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),point_str),"|"),life_str),"|"),dir),"|")) ;
 
  strcpy(redit1,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag1,"|"),point_str),"|"),life_str),"|"),dir1),"|"),iden1),"|")) ;

  strcpy(redit2,strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flag2,"|"),point_str),"|"),life_str),"|"),dir2),"|"),iden2),"|")) ;

 //send(myfd, redit, strlen(redit), 0);

  
    send(myfd, redit1, strlen(redit1), 0);

   // printf("%s\n",redit2);  
 
     send(sendfd, redit2, strlen(redit2), 0);

  printf("point to life C[%d]to [%d]:%s\n", fd,sendfd, redit1);
    printf("point to life C[%d]to [%d]:%s\n", sendfd,fd, redit2);




   memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件
        return;

}
else{
     memset(ev->buf, 0, sizeof(ev->buf));
            ev->len = 0;
        eventset(ev, fd, recvdata, ev);                     //设置该 fd 对应的回调函数为 senddata
        eventadd(g_efd, EPOLLIN, ev);                      //将fd加入红黑树g_efd中,监听其写事件
return;
  //  char bufferfor[BUFLEN];
    //strcpy(bufferfor, "asdfghjkl");
    //sendBroadcastmessage(ev->fd,roomnumcur,bufferfor);//clinetfd,room,buffer
}


}



      //  eventset(ev, fd, senddata, ev);                     //设置该 fd 对应的回调函数为 senddata
        //eventadd(g_efd, EPOLLOUT, ev);                      //将fd加入红黑树g_efd中,监听其写事件

    } else if (len == 0) {
        close(ev->fd);
        /* ev-g_events 地址相减得到偏移元素位置 */
          for (int i = 0; i < fdlist.size(); i++)
        {
            if (fdlist[i].fd == ev->fd)
            {
                fdlist.erase(fdlist.begin()+i);
                 printf("erase %d\n", fdlist[i].fd);
            }
        }
        printf("[fd=%d] pos[%ld], closed\n", fd, ev-g_events);

    } else {
        close(ev->fd);
         for (int i = 0; i < fdlist.size(); i++)
        {
            if (fdlist[i].fd == ev->fd)
            {
                fdlist.erase(fdlist.begin()+i);
                 printf("erase %d\n", fdlist[i].fd);
            }
        }
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
    }

    return;
}

void senddata(int fd, int events, void *arg)
{
    printf("send");
    struct myevent_s *ev = (struct myevent_s *)arg;
    int len;

    len = send(fd, ev->buf, ev->len, 0);                    //直接将数据 回写给客户端。未作处理


   // sendBroadcastmessage(fd);
    /*
    printf("fd=%d\tev->buf=%s\ttev->len=%d\n", fd, ev->buf, ev->len);
    printf("send len = %d\n", len);
    */

    if (len > 0) {

        printf("send[fd=%d], [%d]%s\n", fd, len, ev->buf);
        eventdel(g_efd, ev);                                //从红黑树g_efd中移除
        eventset(ev, fd, recvdata, ev);                     //将该fd的 回调函数改为 recvdata
        eventadd(g_efd, EPOLLIN, ev);                       //从新添加到红黑树上， 设为监听读事件

    } else {
        close(ev->fd);                                      //关闭链接
        eventdel(g_efd, ev);                                //从红黑树g_efd中移除
        printf("send[fd=%d] error %s\n", fd, strerror(errno));
    }

    return ;
}

/*创建 socket, 初始化lfd */

void initlistensocket(int efd, short port)
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(lfd, F_SETFL, O_NONBLOCK);                                            //将socket设为非阻塞

    /* void eventset(struct myevent_s *ev, int fd, void (*call_back)(int, int, void *), void *arg);  */
    eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);

    /* void eventadd(int efd, int events, struct myevent_s *ev) */
    eventadd(efd, EPOLLIN, &g_events[MAX_EVENTS]);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));                                               //bzero(&sin, sizeof(sin))
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    bind(lfd, (struct sockaddr *)&sin, sizeof(sin));

    listen(lfd, 20);

    return ;
}

int main(int argc, char *argv[])
{
    unsigned short port = SERV_PORT;

    if (argc == 2)
        port = atoi(argv[1]);                           //使用用户指定端口.如未指定,用默认端口

    g_efd = epoll_create(MAX_EVENTS+1);                 //创建红黑树,返回给全局 g_efd 
    if (g_efd <= 0)
        printf("create efd in %s err %s\n", __func__, strerror(errno));

    initlistensocket(g_efd, port);                      //初始化监听socket

    struct epoll_event events[MAX_EVENTS+1];            //保存已经满足就绪事件的文件描述符数组 
    printf("server running:port[%d]\n", port);

    int checkpos = 0, i;
    while (1) {
        /* 超时验证，每次测试100个链接，不测试listenfd 当客户端30秒内没有和服务器通信，则关闭此客户端链接 */

        long now = time(NULL);                          //当前时间
        for (i = 0; i < 100; i++, checkpos++) {         //一次循环检测100个。 使用checkpos控制检测对象
            if (checkpos == MAX_EVENTS)
                checkpos = 0;
            if (g_events[checkpos].status != 1)         //不在红黑树 g_efd 上
                continue;

            long duration = now - g_events[checkpos].last_active;       //客户端不活跃

            if (duration >= 30) {
                struct room *p;
                int sendfanotherfd=0;
               
               // int ff=1;
                //int emproom=0;

 /*   for(p=rooms;p<rooms+7;p++){
        
         printf("%d\n%d\n",p->fd1,p->fd2);
         if (p->fd1==g_events[checkpos].fd)
         {
            sendfanotherfd= p->fd2;
           // emproom=ff;
            
         }
         else if (p->fd2==g_events[checkpos].fd)
         {
             sendfanotherfd= p->fd1;
             //  emproom=ff;
         }
         //ff++;

    }*/
    //用p<boy+7控制遍历,7是boy的元素个数
       // if (sendfanotherfd!=0)
     //   {
   
   //send(sendfanotherfd, "woo|2|", strlen("woo|2|"), 0);
    //    }
                gameflag=0;
  send(g_events[checkpos].fd, "woo|1|", strlen("woo|1|"), 0);


            //    send(g_events[checkpos].fd, "timebyebye|", strlen("timebyebye"), 0);
              
 //              sleep(1);
                close(g_events[checkpos].fd);                           //关闭与该客户端链接
 //rooms[emproom].fd1=0;
//rooms[emproom].fd2=0;
//printf("!!!!%d\n",rooms[1].fd1);

//printf("!!!!!%d\n",rooms[1].fd2);
     /*            for (int i = 0; i < fdlist.size(); i++)
        {
            if (fdlist[i].fd == g_events[checkpos].fd)
            {
                fdlist.erase(fdlist.begin()+i);
            }
        }
*/
                printf("[fd=%d] timebyebye\n", g_events[checkpos].fd);
                eventdel(g_efd, &g_events[checkpos]);                   //将该客户端 从红黑树 g_efd移除
            }
                 else if (duration >= 20) {
                send(g_events[checkpos].fd, "timeout|", strlen("timeout|"), 0);
               
              //  close(g_events[checkpos].fd);                           //关闭与该客户端链接

     /*            for (int i = 0; i < fdlist.size(); i++)
        {
            if (fdlist[i].fd == g_events[checkpos].fd)
            {
                fdlist.erase(fdlist.begin()+i);
            }
        }
*/
                printf("[fd=%d] timeout1\n", g_events[checkpos].fd);
              //  eventdel(g_efd, &g_events[checkpos]);                   //将该客户端 从红黑树 g_efd移除
            }
        } 


        /*监听红黑树g_efd, 将满足的事件的文件描述符加至events数组中, 1秒没有事件满足, 返回 0*/
        int nfd = epoll_wait(g_efd, events, MAX_EVENTS+1, 1000);
        if (nfd < 0) {
            printf("epoll_wait error, exit\n");
            break;
        }
         

 

        for (i = 0; i < nfd; i++) {
            /*使用自定义结构体myevent_s类型指针, 接收 联合体data的void *ptr成员*/
            struct myevent_s *ev = (struct myevent_s *)events[i].data.ptr;  

            if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {           //读就绪事件
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
            if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {         //写就绪事件
                ev->call_back(ev->fd, events[i].events, ev->arg);
            }
        }
    }

    /* 退出前释放所有资源 */
    return 0;
}



//将服务器收到的clientfd的消息进行广播
int sendBroadcastmessage(int clientfd,int roomid,char buf[])
{

   // char buf[BUFLEN], message[BUFLEN];
    //清零
    //bzero(buf, BUFLEN);
    //bzero(message, BUFLEN);

    printf("read from client(clientID = %d)\n", clientfd);
  // int len = recv(clientfd, buf, BUFLEN, 0);
    int len =strlen(buf);
    if(len == 0)  // len = 0 client关闭了连接
    {
        close(clientfd);
        for (int i = 0; i < fdlist.size(); i++)
        {
            if (fdlist[i].fd == clientfd)
            {
                fdlist.erase(fdlist.begin()+i);
            }
        } //list删除fd
        printf("ClientID = %d closed.\n now there are %d client in the char room\n", clientfd, (int)fdlist.size());
        return -1;
    }
    else  //进行广播
    {
      /*  if(fdlist.size() == 1) {
            send(clientfd, CAUTION, strlen(CAUTION), 0);
            return len;
        }
*/
   //     sprintf(message, SERVER_MESSAGE, clientfd, buf);



//for (int i = 0; i < fdlist.size(); i++)
  //      {
    //        if (fdlist[i].fd != clientfd)
      //      {
                  if( send(rooms[roomid].fd1, buf, strlen(buf), 0) < 0 ) { perror("error"); exit(-1);}
                  if( send(rooms[roomid].fd2, buf, strlen(buf), 0) < 0 ) { perror("error"); exit(-1);}
        //   }
          //  }
        }



    return len;
}






void initmap(){

if (flag==0)
{
     for (int i = 0; i < Fieldy; i++)
    {
        for (int j = 0; j < Fieldx; j++)
        {
           // int fh =  rand() % 2;
            
            if (i == 0)
            {
                field[i][j] = '*';
                savefield[i][j]='*';
                rooms[roomnumber].warfield[i][j]='*';
            }
            else if (j == 0)
            {
                field[i][j] = '*';
                savefield[i][j]='*';
                 rooms[roomnumber].warfield[i][j]='*';
            }
            else if (i == (Fieldy-1))
            {
                field[i][j] = '*';
                savefield[i][j]='*';
                 rooms[roomnumber].warfield[i][j]='*';
            }
            else if (j == (Fieldx-1))
            {
                field[i][j] = '*';
                savefield[i][j]='*';
                 rooms[roomnumber].warfield[i][j]='*';
            }
           // else if (i==(rightboader-1))
           // {
          //        field[i][j] = '*';
          //  }
            else
            {
                
              //  if (j!=1&&(j)%12==0&&fh==0)
                //{
                  // field[i][j] = '*';
                //}
                //else{

                    field[i][j] = ' ';
                    savefield[i][j]=' ';

 rooms[roomnumber].warfield[i][j]=' ';
if(i>5&&i<13){

                    if(j>5&&j<36){

int fh = rand()%39;
if(fh==1){
    makewall(i,j);
}
                    }
                    else if(j>35&&j<67){
int fh = rand()%39;
if(fh==1){
    makewall(i,j);
}
                    }
              
}
else if (i>12&&i<21)
{
                    if(j>5&&j<36){

int fh = rand()%39;
if(fh==1){
    makewall(i,j);
}
                    }
                    else if(j>35&&j<67){
int fh = rand()%39;
if(fh==1){
    makewall(i,j);
}
                    }
             
}
           

                //}
                
            }
        }
    }
flag=1;
}
else{
     for (int i = 0; i < Fieldy; i++)
    {
        for (int j = 0; j < Fieldx; j++)
        {

    field[i][j]=savefield[i][j];
     rooms[roomnumber].warfield[i][j]=savefield[i][j];
}
}

}

   for (int i = 0; i < Fieldy; i++)
    {
        for (int j = 0; j < rightboader-Fieldx; j++)
        {
             if (i == 0)
            {
                rankboard[i][j] = '*';
            }
           else if (j == 0)
            {
                rankboard[i][j] = '*';
            }
            else if (i == (Fieldy-1))
            {
                rankboard[i][j] = '*';
            }
            else if (j == (rightboader-Fieldx-1))
            {
                rankboard[i][j] = '*';
            }
            else
            {
                rankboard[i][j] = ' ';
            }

             //field[i][j] = '*';
        }
    }

makehome();
}

void makewall(int i,int j){

field[i][j]='*';
savefield[i][j]='*';
 rooms[roomnumber].warfield[i][j]='*';

field[i+1][j+1]='*';
savefield[i+1][j+1]='*';
 rooms[roomnumber].warfield[i+1][j+1]='*';

field[i-1][j-1]='*';
savefield[i-1][j-1]='*';
 rooms[roomnumber].warfield[i-1][j-1]='*';


field[i-1][j+1]='*';
savefield[i-1][j+1]='*';
 rooms[roomnumber].warfield[i-1][j+1]='*';

field[i+1][j-1]='*';
savefield[i+1][j-1]='*';
 rooms[roomnumber].warfield[i+1][j-1]='*';

field[i-1][j]='*';
savefield[i-1][j]='*';
 rooms[roomnumber].warfield[i-1][j]='*';


field[i+1][j]='*';
savefield[i+1][j]='*';
 rooms[roomnumber].warfield[i+1][j]='*';

field[i][j-1]='*';
savefield[i][j-1]='*';
 rooms[roomnumber].warfield[i][j-1]='*';

field[i][j+1]='*';
savefield[i][j+1]='*';
 rooms[roomnumber].warfield[i][j+1]='*';

}

void makehome(){

field[13][1]='#';
field[13][2]='#';
field[13][3]='#';
 rooms[roomnumber].warfield[13][1]='#';
  rooms[roomnumber].warfield[13][2]='#';
   rooms[roomnumber].warfield[13][3]='#';

field[14][1]='#';
field[14][2]='$';
field[14][3]='#';
 rooms[roomnumber].warfield[14][1]='#';
  rooms[roomnumber].warfield[14][2]='$';
   rooms[roomnumber].warfield[14][3]='#';

field[15][1]='#';
field[15][2]='#';
field[15][3]='#';
 rooms[roomnumber].warfield[15][1]='#';
  rooms[roomnumber].warfield[15][2]='#';
   rooms[roomnumber].warfield[15][3]='#';


field[13][Fieldx-2]='#';
field[13][Fieldx-3]='#';
field[13][Fieldx-4]='#';
 rooms[roomnumber].warfield[13][Fieldx-2]='#';
  rooms[roomnumber].warfield[13][Fieldx-3]='#';
   rooms[roomnumber].warfield[13][Fieldx-4]='#';

field[14][Fieldx-2]='#';
field[14][Fieldx-3]='$';
field[14][Fieldx-4]='#';
 rooms[roomnumber].warfield[14][Fieldx-2]='#';
  rooms[roomnumber].warfield[14][Fieldx-3]='$';
   rooms[roomnumber].warfield[14][Fieldx-4]='#';


field[15][Fieldx-2]='#';
field[15][Fieldx-3]='#';
field[15][Fieldx-4]='#';
rooms[roomnumber].warfield[15][Fieldx-2]='#';
  rooms[roomnumber].warfield[15][Fieldx-3]='#';
   rooms[roomnumber].warfield[15][Fieldx-4]='#';

}




void resetpos(Tank& t,int roomnu,int inputi, int reserx,int resery,int dirct)//after move , re set positon:tank,room,player,xreserve,y reserve
{   
    if (t.pos.y < 2)//move to border stop!
    {
        t.pos.y++;
           
              resetpos(t,roomnu,inputi,reserx,resery+1,dirct);
    }
    else if (t.pos.y > (Fieldy-3))//move to border stop!
    {
        t.pos.y--;
       
              resetpos(t,roomnu,inputi,reserx,resery-1,dirct);
    }
    else if (t.pos.x > (Fieldx-3))//move to border stop!
    {
        t.pos.x--;
 
 resetpos(t,roomnu,inputi,reserx-1,resery,dirct);
    } 
    else if (t.pos.x < 2)//move to border stop!
    {
        t.pos.x++;
        resetpos(t,roomnu,inputi,reserx+1,resery,dirct);

    } 
    
if (t.life == true)
    {
        if (t.direction == 1)
        {
            if (rooms[roomnu].warfield[t.pos.y-1][t.pos.x]=='*'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x]=='*'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x-1]=='*'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x+1]=='*'
                || rooms[roomnu].warfield[t.pos.y+1][t.pos.x-1]=='*'
                ||rooms[roomnu].warfield[t.pos.y+1][t.pos.x+1]=='*'
                ||rooms[roomnu].warfield[t.pos.y-1][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x+1]=='#'
                || rooms[roomnu].warfield[t.pos.y+1][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y+1][t.pos.x+1]=='#'
                )
            {
                t.pos.y++;
     
            }
  
            
   
        }
        else if (t.direction == 2)
        {
         
           if(rooms[roomnu].warfield[t.pos.y+1][t.pos.x] =='*'
            ||rooms[roomnu].warfield[t.pos.y][t.pos.x]=='*'
            || rooms[roomnu].warfield[t.pos.y][t.pos.x-1]=='*'
            ||rooms[roomnu].warfield[t.pos.y][t.pos.x+1]=='*'
            ||rooms[roomnu].warfield[t.pos.y-1][t.pos.x-1]=='*'
            ||rooms[roomnu].warfield[t.pos.y-1][t.pos.x+1]=='*'
                ||rooms[roomnu].warfield[t.pos.y-1][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x+1]=='#'
                || rooms[roomnu].warfield[t.pos.y+1][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y+1][t.pos.x+1]=='#'
            ) 
           {
             t.pos.y--;

           }
            
        }
        else if (t.direction == 3)
        {

            if( rooms[roomnu].warfield[t.pos.y][t.pos.x] =='*'
            ||rooms[roomnu].warfield[t.pos.y][t.pos.x+1]=='*'
            ||  rooms[roomnu].warfield[t.pos.y-1][t.pos.x]=='*'
            || rooms[roomnu].warfield[t.pos.y+1][t.pos.x]=='*'
            || rooms[roomnu].warfield[t.pos.y-1][t.pos.x-1]=='*'
            || rooms[roomnu].warfield[t.pos.y+1][t.pos.x-1]=='*'
                ||rooms[roomnu].warfield[t.pos.y-1][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x+1]=='#'
                || rooms[roomnu].warfield[t.pos.y+1][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y+1][t.pos.x+1]=='#'
            ) 
           {
             t.pos.x--;
            /* char inflag[10];
             strcpy(inflag,"a");
             sendmessage(inflag);
              clear();
               rendertank(t);
               out();
               refresh();
              resetpos(t);*/
             
           }


        
        }
        else if (t.direction == 4)
        {


             if(  rooms[roomnu].warfield[t.pos.y][t.pos.x] =='*'
            || rooms[roomnu].warfield[t.pos.y][t.pos.x-1]=='*'
            ||  rooms[roomnu].warfield[t.pos.y-1][t.pos.x]=='*'
            ||  rooms[roomnu].warfield[t.pos.y+1][t.pos.x]=='*'
            ||   rooms[roomnu].warfield[t.pos.y-1][t.pos.x+1]=='*'
            || rooms[roomnu].warfield[t.pos.y+1][t.pos.x+1]=='*'
                ||rooms[roomnu].warfield[t.pos.y-1][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y][t.pos.x+1]=='#'
                || rooms[roomnu].warfield[t.pos.y+1][t.pos.x-1]=='#'
                ||rooms[roomnu].warfield[t.pos.y+1][t.pos.x+1]=='#'
            ) 
           {
             t.pos.x++;
        

           }


        }
    }
    

    for (int i = 0; i<4; i++)//check distance : two tank not more than 3,they meet,they stop(enemy->enemy)
    {
        for (int j = 0; j<4; j++)
        {
            if(i == j)
            {
                continue;
            }
            else if ((rooms[roomnu].player[i].pos.x - rooms[roomnu].player[j].pos.x < 4)&&(rooms[roomnu].player[i].pos.y - rooms[roomnu].player[j].pos.y < 4)
                &&(rooms[roomnu].player[i].pos.x - rooms[roomnu].player[j].pos.x > -4)&&(rooms[roomnu].player[i].pos.y - rooms[roomnu].player[j].pos.y > -4)
                &&(rooms[roomnu].player[j].life == true))
            {
                rooms[roomnu].player[inputi].pos.x = reserx;
                rooms[roomnu].player[inputi].pos.y = resery;
                rooms[roomnu].player[inputi].direction=dirct;
            }
        }
    }
        
    
}

/*void move(Tank &t,char teamid){

    if (t.life == true)
    {
      
        
    }
}
*/


