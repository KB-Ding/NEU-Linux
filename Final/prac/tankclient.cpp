#include <ctime> 
#include <vector>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <curses.h>
#include <cstdio>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include "socklib.h"
#include <sys/types.h>
#include <string.h> 
#include <algorithm> 
#include <sstream> 
//#include<<memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>

#define BUFSIZE 4096


using namespace std;
int points = 0;
int enemypoints = 0;
int flag=0;
int const Fieldx = 72;
int const Fieldy = 28;
int const rightboader = 153;
char notes[]="                                   ";

int p0=0;
int e0=0;
int p1=1;
int e1=1;
int x,y,direc;
bool enemyshootflag;



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
    char center;
};
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
char part = '+';
char cyc = '@';
char acenter = 'A';
char bcenter = 'B';
char aicenter = 'a';
char bicenter = 'b';

int playerposx[2];
int playerposy[2];
int enemyposx[2];
int enemyposy[2];
bool quit = false;
Tank player[2];
Tank enemy[2];
vector<Bullet> bomb;
Bullet bullet;
static char field[Fieldy][Fieldx];
//static char savefield[Fieldy][Fieldx];
static char savefield[Fieldy][Fieldx];
static char ss[Fieldy][Fieldx];
char rankboard[Fieldy][rightboader-Fieldx+1];
void out();
void init();
void initmap();
void makewall(int,int);
int f = -1;
int prevpoints = 0;
pid_t pid;
int pfd[2];
int finalpipe[2];
pthread_t tid;
pthread_t tid1;
pthread_mutex_t mutex;
int byebye=0;
void signalHandler(int signo);
void rendertank(Tank);
void bulletmove(Bullet&, int);
//void playershoot();
//void enemyshoot();
void bulletimpact();
void playermove();
void enemymove();
void resetpos(Tank&);
void renderbullet(Bullet);
void respawn(Tank&);
void tanksgame();
void fun_final(int signo);
void fun_deal(int signo);
int kill(pid_t pid, int sig);
int Close(int fd);
void perr_exit(const char *s);
void sendmessage(char inflag[]);
void sendlongmessage(int x, int y,int z);
void *tfn(void *arg);
void *tffn(void *arg);
//int kills=0;

//int lvl;
int lifes = 3;
int enemylifes=3;
double time_counter1 = 0;
double time_counter2 = 0;
clock_t this_time;
char team;
int room;
int fd;

int main() 
{
    
    
    char buf_in[BUFSIZE];
    char buf_out[BUFSIZE];
     pthread_mutex_init(&mutex, NULL);
    char local_addr[20] = "parallels-vm";
    fd = connect_to_server(local_addr, 8080);
    //通信准备结束

//int fllag;
  //   if ((fllag = fcntl(fd, F_SETFL, O_NONBLOCK)) < 0) {             //将cfd也设置为非阻塞
    //        printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
      //      break;
       // }
   /* int x;
x=fcntl(fd,F_GETFL,0);
fcntl(fd,F_SETFL,x | O_NONBLOCK);
*/
    //匹配对手
    system("clear");
    printf("waiting...\n");

write(fd, "yes", BUFSIZE);



  /*  struct termios echo_false, echo_true;
    tcgetattr(0, &echo_false);
    echo_true = echo_false;
    echo_false.c_lflag &= ~ECHO;
    echo_false.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &echo_false);

*/
    while (read(fd, buf_in,sizeof(buf_in)) <= 0);
    //printf("%s\n",buf_in[0]);

sscanf(buf_in,"%d",&room);
printf("%d\n",room );// make room



    while (read(fd, buf_in,sizeof(buf_in)) <= 0);
    //printf("%s\n",buf_in[0]);

    string str=buf_in;

    if (str.find("a")!=-1)
    {
    //  write(fd, "a", BUFSIZE);
        printf("a\n");
        team='a';
         for (int i = 0; i < Fieldy; i++)
    {
    //read(fd, buf_in,BUFSIZE);

    //read(fd, buf_in,BUFSIZE);
    
    recv(fd,(char*)&field[i], sizeof(field[i]),0);

//recv(fd,(char*)&field[1], BUFSIZE,0);
    //cout << "接收到整形数组："<< endl;
    

}
    cout << endl;
 for (int i = 0; i < Fieldy; i++)
    {
for (int j=0; j < Fieldx; j ++)
        savefield[i][j]=field[i][j];
   
    }
    }
    
    if (str.find("b")!=-1)
    {
    //  write(fd, "b", BUFSIZE);
        printf("b\n");
        team='b';

         for (int i = 0; i < Fieldy; i++)
    {
    //read(fd, buf_in,BUFSIZE);

    //read(fd, buf_in,BUFSIZE);
    
    recv(fd,(char*)&field[i], sizeof(field[i]),0);

//recv(fd,(char*)&field[1], BUFSIZE,0);
    //cout << "接收到整形数组："<< endl;
    

    }

cout << endl;
 for (int i = 0; i < Fieldy; i++)
    {
for (int j=0; j < Fieldx; j ++)
        savefield[i][j]=field[i][j];
    }
    }
// while (read(fd, buf_in,sizeof(buf_in)) >0)
   /* char buffin[BUFSIZE];
    while (read(fd, buffin,sizeof(buffin)) <= 0);
    string as = buffin;
    cout<<as;

    cout << endl;
sleep(30);
*/
//system("pause");
   //s system("clear");
   // sleep(15);
//  char buf[4096] ; 
  //      memset(buf,0,BUFSIZE);          
    //    printf("%d\n",fd );

//while(read(fd, buf,BUFSIZE)<=0);
   
 
  
    struct termios echo_false, echo_true;
    tcgetattr(0, &echo_false);
    echo_true = echo_false;
    echo_false.c_lflag &= ~ECHO;
    echo_false.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &echo_false);
    //tcsetattr(0, TCSANOW, &echo_true);    

  

    char pipe_out[BUFSIZE];
    char pipe_in[BUFSIZE];
    pipe(pfd);
    pipe(finalpipe);
    while ((pid = fork()) == -1);
    if (pid == 0)
    {

    bomb.clear();
    f = -1;
    initscr(); noecho(); curs_set(0); keypad(stdscr, 1); nodelay(stdscr, 1);

   // initmap();
    init();//tank init

        player[0].life = true;
           player[1].life = true;
        
         for (int i = 0; i<2; i++) {
            enemy[i].life = true;
        }
       // enemy.life = true;
        
        signal(10, fun_final);
//       pause();
         string ss="";
         time_counter1 = time_counter2 = clock();
 signal(12, fun_deal);
 //pause();
   
 
    struct itimerval new_value, old_value;
    new_value.it_value.tv_sec = 1;
    new_value.it_value.tv_usec = 0;
    new_value.it_interval.tv_sec = 7;
    new_value.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &new_value, &old_value);
        
        signal(SIGALRM, signalHandler);
      //  pause();
while(1){
   
     tanksgame();//play the game 
     
        if(byebye==1){
          
    ss="you lose";

           // cout << "Your Points: " << points << endl;
           
            break;
        }
              else if(byebye==2){
           
    ss="you win";

           // cout << "Your Points: " << points << endl;
           
            break;
        }
}
    

        nodelay(stdscr, 0);
        endwin();

       // system("clear");
        cout<<ss;
       cout<<endl;

       exit(0);
          //Close(fd);
          //return 0;
/*
        if (player.life == false)
        {
            cout << "\nSorry, you lose!" << endl;
            cout << "Your Points: " << points << endl;
          //  break;
        }
*/
       

    }
    else
    {
          
 /*   char recvbuf[1024];
    while (1)
    {
        memset(recvbuf, 0, sizeof(recvbuf));
        int ret = read(conn, recvbuf, sizeof(recvbuf));
        if (ret == 0)   //客户端关闭了
        {
            printf("client close\n");
            break;
        }
        else if (ret == -1)
            ERR_EXIT("read error");
        fputs(recvbuf, stdout);
        write(conn, recvbuf, ret);
    }*/

sleep(1);

        char buf_out[4096];
       // sleep(4);
        
     //   for (int i = 0; i < Fieldy; i++)
   // {
    //read(fd, buf_in,BUFSIZE);

    //read(fd, buf_in,BUFSIZE);
    
    //recv(fd,(char*)&ss[i], sizeof(ss[i]),0);

//recv(fd,(char*)&field[1], BUFSIZE,0);
    //cout << "接收到整形数组："<< endl;
    

//}

        while (1){

        char buf[BUFSIZE] ; 
        memset(buf,0,BUFSIZE);          
     //   printf("%d\n",fd );

        //;read(fd, buf_in, sizeof(buf_in)) <= 0

        while(read(fd, buf,BUFSIZE)<=0);
    
   // printf("11\n");
            //move(Fieldy + 3, 0);
           // printw("rec");
    string ss = buf;
 //   system("clear");
  // move( 3, 0);
   // printw("rec");
   // move(Fieldy + 4, 0);
  //  cout<<ss;
  //  sleep(10);
      if (ss.find("|")!=-1){

     //   printf("%s\n",ss );
            
//printf("22\n");
          //  char *s = NULL;
           // char flags[10];
           // char team [10];
      //   char loc_x_str[10], loc_y_str[10],dir[10];
      //   char roomsr[10];
        // int sendfd; 


           //                 s = strtok(buf, "|");
           
                           // printf("%s\n",s);

             //               strcpy(flags, s);



            if (ss.find("woo")!=-1)
            {
                 write( finalpipe[1], buf, BUFSIZE);
               
                kill(pid, 10);
                sleep(1);
                break;
            }

  

            else {
                //system("clear");
                write(pfd[1], buf, BUFSIZE);
               //  move(Fieldy + 1, 0);
             //   printw("yes");
                kill(pid, 12);

            }
      }
      else{

      }

           
               
            
        }

         if(wait(NULL) ==-1)
        { //回收子进程状态，避免僵尸进程
            perror("fail to wait\n");
            exit(1);
        }
        
        exit(0);

    }


  

    
   // for (lvl = 1; lvl <= 10; lvl++)
 //   {
       
    /*    if ((points >= 10*lvl+prevpoints)&&(lvl<=10))
        {
            prevpoints = points;
            if (lvl<10)
            {
                cout << "\nCongratulations, you won!" << endl;
                cout << "Your Points: " << kills << endl;
                cout << "Starting Level " << lvl+1 << endl;
            }
            else
            {
                cout << "\nCongratulations, you have completed the game!" << endl;
                cout << "Your Points: " << kills << endl;
            }
                
           
                sleep(3);
           
        }*/
  //  }
   pthread_mutex_destroy(&mutex);
    return 0;
    
}

int Close(int fd)
{
    int n;
    if ((n = close(fd)) == -1)
        perr_exit("close error");

    return n;
}

 
 void perr_exit(const char *s)
{
    perror(s);
    exit(-1);
}


void out()//menu , after win appear
{
    clear();
    for (int i = 0; i < Fieldy; i++)
    {
        move(i, 0);
        for (int j = 0; j < Fieldx; j++)
        {
            addch(field[i][j]);
        }
    }

  /*      for (int i = 0; i < Fieldy; i++)
    {
        move(i , Fieldx-1);
        for (int j = 0; j < rightboader-Fieldx; j++)
        {
            addch(rankboard[i][j]);
        }
    }*/
   // move(Fieldy, 0);
   // printw("Level ");
   // printw("%d", lvl);
    move(Fieldy + 1, 0);
    printw("player:");
    move(Fieldy + 2, 0);
    printw("points:   ");
    printw("%d", points);
   
    move(Fieldy + 3, 0);
    printw("life:    ");
    printw("%d", lifes);

    move(Fieldy + 4, 0);
    printw("note:    ");
    printw("%s", notes);


     move(Fieldy + 1, Fieldx-12);
    printw("enemy:");
    move(Fieldy + 2, Fieldx-12);
    printw("points:   ");
    printw("%d", enemypoints);
   
    move(Fieldy + 3, Fieldx-12);
    printw("life:    ");
    printw("%d", enemylifes);


   // move(Fieldy + 3, 0);
  //  printw("Kills left to complete level: ");
   // printw("%d/%d     ", (points-prevpoints), (lvl*10));
}

void init()//initial window,enemy,player
{
   if (team=='a')
   {
    
    player[p0].pos.y = (Fieldy-4);
    player[p0].pos.x = (Fieldx-4);
    player[p0].direction = 1;
    player[p0].spos.y= (Fieldy-4);
    player[p0].spos.x= (Fieldx-4);
    player[p0].sdir = 1;
    player[p0].sshoot = 0;
    player[p0].life = true;
    player[p0].center = acenter;


    enemy[e0].pos.y = 2;
    enemy[e0].pos.x = 2;
    enemy[e0].direction = 2;
    enemy[e0].shoot = 0;
    enemy[e0].spos.y = 2;
    enemy[e0].spos.x = 2;
    enemy[e0].sdir = 2;
    enemy[e0].sshoot = 0;
    enemy[e0].life = true;
    enemy[e0].center = bcenter;

    player[p1].pos.y = 25;
    player[p1].pos.x = 36;
    player[p1].direction = 1;
    player[p1].shoot = 0;
    player[p1].spos.y = 25;
    player[p1].spos.x = 36;
    player[p1].sdir = 1;
    player[p1].sshoot = 0;
    player[p1].life = true;
    player[p1].center = aicenter;

    enemy[e1].pos.y = 2;
    enemy[e1].pos.x = 36;
    enemy[e1].direction = 2;
    enemy[e1].shoot = 0;
    enemy[e1].spos.y = 2;
    enemy[e1].spos.x = 36;
    enemy[e1].sdir = 2;
    enemy[e1].sshoot = 0;
    enemy[e1].life = true;
    enemy[e1].center = bicenter;

   }
   else if (team=='b')
   {
    enemy[e0].pos.y = (Fieldy-4);
    enemy[e0].pos.x = (Fieldx-4);
    enemy[e0].direction = 1;
    enemy[e0].spos.y= (Fieldy-4);
    enemy[e0].spos.x= (Fieldx-4);
    enemy[e0].sdir = 1;
    enemy[e0].sshoot = 0;
    enemy[e0].life = true;
    enemy[e0].center = acenter;

    player[p0].pos.y = 2;
    player[p0].pos.x = 2;
    player[p0].direction = 2;
    player[p0].shoot = 0;
    player[p0].spos.y = 2;
    player[p0].spos.x = 2;
    player[p0].sdir = 2;
    player[p0].sshoot = 0;
    player[p0].life = true;
    player[p0].center = bcenter;

    enemy[e1].pos.y = 25;
    enemy[e1].pos.x = 36;
    enemy[e1].direction = 1;
    enemy[e1].shoot = 0;
    enemy[e1].spos.y = 25;
    enemy[e1].spos.x = 36;
    enemy[e1].sdir = 1;
    enemy[e1].sshoot = 0;
    enemy[e1].life = true;
    enemy[e1].center = aicenter;

    player[p1].pos.y = 2;
    player[p1].pos.x = 36;
    player[p1].direction = 2;
    player[p1].shoot = 0;
    player[p1].spos.y = 2;
    player[p1].spos.x = 36;
    player[p1].sdir = 2;
    player[p1].sshoot = 0;
    player[p1].sshoot = 0;
    player[p1].life = true;
    player[p1].center = bicenter;
   }

else{
 
}

   
}
 
void rendertank(Tank t)// tank shape initial
{
    if (t.life == true)
    {
        if (t.direction == 1)
        {
            //field[t.pos.y][t.pos.x] = part;
            field[t.pos.y-1][t.pos.x] = part;
            field[t.pos.y][t.pos.x] = t.center;
            field[t.pos.y][t.pos.x-1] = cyc;
            field[t.pos.y][t.pos.x+1] = cyc;
            field[t.pos.y+1][t.pos.x-1] = cyc;
            field[t.pos.y+1][t.pos.x+1] = cyc;
        }
        else if (t.direction == 2)
        {
         //   field[t.pos.y][t.pos.x] = part;
            field[t.pos.y+1][t.pos.x] = part;
            field[t.pos.y][t.pos.x] = t.center;
            
            field[t.pos.y][t.pos.x-1] = cyc;
            field[t.pos.y][t.pos.x+1] = cyc;
            field[t.pos.y-1][t.pos.x-1] = cyc;
            field[t.pos.y-1][t.pos.x+1] = cyc;
        }
        else if (t.direction == 3)
        {
            field[t.pos.y][t.pos.x] = t.center;
            field[t.pos.y][t.pos.x+1] = part;

            field[t.pos.y-1][t.pos.x] = cyc;
            field[t.pos.y+1][t.pos.x] = cyc;
            field[t.pos.y-1][t.pos.x-1] = cyc;
            field[t.pos.y+1][t.pos.x-1] = cyc;
        }
        else if (t.direction == 4)
        {
            field[t.pos.y][t.pos.x] = t.center;
            field[t.pos.y][t.pos.x-1] = part;
            field[t.pos.y-1][t.pos.x] = cyc;
            field[t.pos.y+1][t.pos.x] = cyc;
            field[t.pos.y-1][t.pos.x+1] = cyc;
            field[t.pos.y+1][t.pos.x+1] = cyc;
        }
    }
    
}
 
void renderbullet(Bullet b)//bullet shape fire
{
    if ((b.exist == true)&&(b.enemyshoot==true))
    {
        field[b.pos.y][b.pos.x] = 'x';
    }  
    else if ((b.exist == true)&&(b.enemyshoot==false))
    {
        field[b.pos.y][b.pos.x] = 'o';
    }
    
}
 
void playermove()//get keyboard input 
{
   // int fh =0;
   // fh = kills;
   // move(Fieldy+6,0);
  //  printw("%d,fh",fh);
    char flags[10];
   

    switch (getch()) {
        //case KEY_UP:
        case 'w':
         //   player.direction = 1;
       //     player.pos.y--;
             strcpy(flags, "w"); /*给数组赋字符串*/
            break;
        //case KEY_DOWN:
         case 's': 
           // player.direction = 2;
            //player.pos.y++;
             strcpy(flags, "s"); /*给数组赋字符串*/
            break;
        //case KEY_RIGHT:
          case 'd':
            //player.direction = 3;
           // player.pos.x++;
             strcpy(flags, "d"); /*给数组赋字符串*/
            break;
        //case KEY_LEFT:
          case 'a':
           // player.direction = 4;
            //player.pos.x--;
             strcpy(flags, "zzz"); /*给数组赋字符串*/
            break;
        case 'k': 
        //case 'T':
          //  if (points >= 20)
            //{
              //  points = points - 1;
             //   lifes++;
                strcpy(flags, "o"); /*给数组赋字符串*/
            //}
            break;
       // case ' ':
         case 'j':
        //  strcpy(flags, "f"); /*给数组赋字符串*/

           // playershoot();
         strcpy(flags, "f");
            break;
        case 'l': 
           // quit = true;
         //   enemymove();
        strcpy(flags, "l");
            break;
    }

if(strcmp(flags,"w")==0||strcmp(flags,"zzz")==0||strcmp(flags,"s")==0||strcmp(flags,"d")==0||strcmp(flags,"o")==0||strcmp(flags,"f")==0||strcmp(flags,"l")==0){
     char loc_x_str[10], loc_y_str[10],fir[10],teamstr[10];
     
   // sprintf(loc_x_str, "%d", player.pos.x);
    //sprintf(loc_y_str, "%d", player.pos.y);
    //sprintf(fir, "%d", player.direction);
    sprintf(teamstr, "%c", team);
    char room_str[10];
    sprintf(room_str, "%d", room);
    char buf_in[BUFSIZE];
    char buf_out[BUFSIZE];
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),loc_x_str ),"|" ),loc_y_str),"|"),teamstr),"|"),fir),"|"),room_str),"|"));
    strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(flags,"|"),teamstr),"|"),room_str),"|"));

    write(fd, buf_out, BUFSIZE);
    if (strcmp(flags,"l")==0)
    {
        signal(12, fun_deal);
        pause();
        /* code */
    }
    usleep(10);
}
   

}


void enemymove()//make enemy tank move and attack auto 
{
  //  if ((enemy[1].life == true)&&(player[1].life==true))
  //  {
            char outflag[30];
            strcpy(outflag,"ghpm");
//if(strcmp(flags,"w")==0||strcmp(flags,"a")==0||strcmp(flags,"s")==0||strcmp(flags,"d")==0||strcmp(flags,"o")==0||strcmp(flags,"f")==0){

    char room_str[BUFSIZE];
    sprintf(room_str, "%d", room);
    char buf_in[BUFSIZE];
    char buf_out[BUFSIZE];
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(outflag,"|"),loc_x_str ),"|" ),loc_y_str),"|"),teamstr),"|"),fir),"|"),room_str),"|"));
     strcpy(buf_out, strcat(strcat(strcat(outflag,"|"),room_str),"|"));

     write(fd, buf_out, BUFSIZE);
    // sleep(0.5);

    // while(true){
      //  if (write(fd, buf_out, BUFSIZE))
        // {
             /* code */
         //} 
        // if((send(fd, buf_out, BUFSIZE, 0)) <= 0){
          //          perror("error send");
            //    }
 // send(fd, buf_out, strlen(buf_out), 0);
     //}

//   tanksgame();
  //xxx  usleep(1);
        
    //}
}
 



void bulletmove(Bullet& b,int i)//deal with event after bomb shoot,during bomb moving
{

          

        if ((b.direction == 1)&&(b.exist == true))//direction and fire
    {
      
        b.pos.y--;

    }
    else if ((b.direction == 2)&&(b.exist == true))//direction and fire
    {
       
        b.pos.y++;
      
         
    }
    else if ((b.direction == 3)&&(b.exist == true))//direction and fire
    {
        
        b.pos.x++;
  
    }
    else if ((b.direction == 4)&&(b.exist == true))//direction and fire
    {
       
         b.pos.x--;
         
    }

    if (b.pos.y < 1 )//out boarder
    {
        b.pos.y++;

        b.exist = false;

    }
    else if (b.pos.y > (Fieldy-2) )//out boarder
    {
        b.pos.y--;
        b.exist = false;
     
    }
    else if (b.pos.x > (Fieldx-2))//out boarder
    {
        b.pos.x--;
        b.exist = false;
      
    } 
    else if (b.pos.x < 1 )//out boarder
    {
        b.pos.x++;
        b.exist = false;
    
    } 


   
   else if ((field[b.pos.y][b.pos.x]=='*'||field[b.pos.y][b.pos.x]=='#')&&(b.direction==4)&&(b.exist==true))

    {
if(field[b.pos.y][b.pos.x]=='#'){
          field[b.pos.y][b.pos.x]=' ';
            savefield[b.pos.y][b.pos.x]=' ';
        }
      
        b.pos.x++;
        b.exist = false;
   
     
    }

else  if ((field[b.pos.y][b.pos.x]=='*'||field[b.pos.y][b.pos.x]=='#')&&(b.direction == 3))//out boarder
    {
          if(field[b.pos.y][b.pos.x]=='#'){
          field[b.pos.y][b.pos.x]=' ';
             savefield[b.pos.y][b.pos.x]=' ';
        }
        b.pos.x--;
        b.exist = false;
    //     bomb.erase(bomb.begin() + i);
    } 
     else  if ((field[b.pos.y][b.pos.x]=='*'||field[b.pos.y][b.pos.x]=='#')&&(b.direction == 2))//out boarder
    {
           if(field[b.pos.y][b.pos.x]=='#'){
            field[b.pos.y][b.pos.x]=' ';
             savefield[b.pos.y][b.pos.x]=' ';
        }
        b.pos.y--;
        b.exist = false;
    //     bomb.erase(bomb.begin() + i);
    } 
     else if ((field[b.pos.y][b.pos.x]=='*'||field[b.pos.y][b.pos.x]=='#')&&(b.direction == 1))//out boarder
    {
        if(field[b.pos.y][b.pos.x]=='#'){
            field[b.pos.y][b.pos.x]=' ';
             savefield[b.pos.y][b.pos.x]=' ';
        }
        b.pos.y++;
        b.exist = false;
    //     bomb.erase(bomb.begin() + i);
    } 

    //for (int j = 0; j < 2; j++)
 //  { 
        if((field[b.pos.y][b.pos.x]=='$')// i hurt enemy on my map, tell service ,service tell other one 
            ||((b.pos.x - enemy[e0].pos.x < 2)
                &&(b.pos.y - enemy[e0].pos.y < 2)
                &&(b.pos.x - enemy[e0].pos.x > -2)
                &&(b.pos.y - enemy[e0].pos.y > -2)
                &&(b.enemyshoot == false) // my own bullet: enemy shoot=false,on my stage,he is enemy
                &&(b.exist == true)
                &&(enemy[e0].life == true)))
       //enemy alive ,bullet not shooten,when pos shoot -> just fire it
        {
           // move(Fieldy + 5, 0);
            // printw("here");
        
        //    enemylifes--;
        //  enemy.life = false;
          //  b.exist = false;
         //    move(Fieldy + 6, 0);
           //  printw("%d",points);
            //points++;
          //   move(Fieldy + 7, 0);
         //   printw("%d",kills);
           // kills++;

    char flags[10],loc_x_str[10], loc_y_str[10],fir[10],teamstr[10];
      char buf_in[BUFSIZE];
    char buf_out[BUFSIZE];
    sprintf(loc_x_str, "%d", b.pos.x);
    sprintf(loc_y_str, "%d", b.pos.y);
    sprintf(fir, "%d", i);
 sprintf(teamstr, "%c", team);
    char room_str[10];
    sprintf(room_str, "%d", room);
    if (field[b.pos.y][b.pos.x]=='$')
    { 
        strcpy(flags, "home");
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(flags,"|"),teamstr),"|"),room_str),"|"));
   
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),teamstr),"|"),fir),"|"),room_str),"|"));//tell enemy which bullet hurt his fuxxing tank
    
  //  write(fd, buf_out, BUFSIZE);
    }//i win
  else {
     strcpy(flags, "h");
//strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),teamstr),"|"),fir),"|"),room_str),"|"));
  }//i hurt you
   // else if (j==0){
        strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),loc_x_str ),"|" ),loc_y_str),"|"),teamstr),"|"),fir),"|"),room_str),"|"));
   write(fd, buf_out, BUFSIZE);
  }
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(flags,"|"),teamstr),"|"),room_str),"|"));

 if((b.pos.x - enemy[e1].pos.x < 2)
                &&(b.pos.y - enemy[e1].pos.y < 2)
                &&(b.pos.x - enemy[e1].pos.x > -2)
                &&(b.pos.y - enemy[e1].pos.y > -2)
                &&(b.enemyshoot == false) // my own bullet: enemy shoot=false,on my stage,he is enemy
                &&(b.exist == true)
                &&(enemy[e1].life == true))
       //enemy alive ,bullet not shooten,when pos shoot -> just fire it
        {
           // move(Fieldy + 5, 0);
            // printw("here");
        
        //    enemylifes--;
        //  enemy.life = false;
          //  b.exist = false;
         //    move(Fieldy + 6, 0);
           //  printw("%d",points);
            //points++;
          //   move(Fieldy + 7, 0);
         //   printw("%d",kills);
           // kills++;

    char flags[10],loc_x_str[10], loc_y_str[10],fir[10],teamstr[10];
      char buf_in[BUFSIZE];
    char buf_out[BUFSIZE];
    sprintf(loc_x_str, "%d", b.pos.x);
    sprintf(loc_y_str, "%d", b.pos.y);
    sprintf(fir, "%d", i);
 sprintf(teamstr, "%c", team);
    char room_str[10];
    sprintf(room_str, "%d", room);

 
     strcpy(flags, "hai");
//strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),teamstr),"|"),fir),"|"),room_str),"|"));
  //i hurt you
   // else if (j==0){
        strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),loc_x_str ),"|" ),loc_y_str),"|"),teamstr),"|"),fir),"|"),room_str),"|"));
   write(fd, buf_out, BUFSIZE);
  }


//i hurt ai
   
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),teamstr),"|"),fir),"|"),room_str),"|"));//tell enemy which bullet hurt his fuxxing tank
    
   
//    }





        
     if ((b.pos.x - enemy[e0].pos.x < 2)&&(b.pos.y - enemy[e0].pos.y < 2)
            &&(b.pos.x - enemy[e0].pos.x > -2)&&(b.pos.y - enemy[e0].pos.y > -2)
            &&(b.enemyshoot == false)&&(b.exist == true)&&(enemy[e0].life == true))//after fired bullet not exit,on my stage disapper it
        {//
          
            b.exist = false;//bie ren de zi dan ,wo rang ta xiao shi 
           // enemy[j].life=false;
        }


    if ((b.pos.x - enemy[e1].pos.x < 2)&&(b.pos.y - enemy[e1].pos.y < 2)
            &&(b.pos.x - enemy[e1].pos.x > -2)&&(b.pos.y - enemy[e1].pos.y > -2)
            &&(b.enemyshoot == false)&&(b.exist == true)&&(enemy[e1].life == true))//after fired bullet not exit,on my stage disapper it
        {//
          
            b.exist = false;//bie ren de zi dan ,wo rang ta xiao shi 
           // enemy[j].life=false;
        }
/*        else if ((b.pos.x - player.pos.x < 2)&&(b.pos.y - player.pos.y < 2)
            &&(b.pos.x - player.pos.x > -2)&&(b.pos.y - player.pos.y > -2)
            &&(b.enemyshoot == true)&&(b.exist == true)&&(player.life == true))//i am attacked by bullet
        {
            lifes--;
            if (lifes == 0)
            {
                player.life = false;
            }
            b.exist = false;



        }  wo zhong dan le */
    
  //  }
           //  move(Fieldy + 5, 0);
           //  printw("%d",kills);
             //  sleep(1);
    for (int i = 0; i<bomb.size(); i++)//bullet attacked by bullet , erase it
    {
        for (int j = 0; j<bomb.size(); j++)
        {
            if(i == j)
            {
                continue;
            }
            else if ((bomb[i].pos.x - bomb[j].pos.x < 1)&&(bomb[i].pos.y - bomb[j].pos.y < 1)
                &&(bomb[i].pos.x - bomb[j].pos.x > -1)&&(bomb[i].pos.y - bomb[j].pos.y > -1)
                &&(bomb[i].exist == true)&&(bomb[j].exist == true))
            {
                bomb[i].exist = false;
                bomb[j].exist = false;
            }
        }
    }
//   }
            // move(Fieldy + 6, 0);
            // printw("%d",kills);
          //   sleep(1);
}
void respawn(Tank& t)//player get another live
{
    //int servx = t.spos.x;
    //int servy = t.spos.y;

  //  if (field[servy][servx]==' ')
   // {
         if (t.life == false)
    {
        t.life = true;
        t.pos.y = t.spos.y;
        t.pos.x = t.spos.x;
        t.direction = t.sdir;
        t.shoot = t.sshoot;
    }
    //}
 /*   else{
        if ((t.life == false)&&(field[servy][servx]==' '))
    {
        t.life = true;
        t.pos.y = t.spos.y;
        t.pos.x = t.spos.x+4;
        t.direction = t.sdir;
        t.shoot = t.sshoot;
    }
        else{
        t.life = true;
        t.pos.y = t.spos.y;
        t.pos.x = t.spos.x+8;
        t.direction = t.sdir;
        t.shoot = t.sshoot;
    }
    }*/
   
}

void *tffn(void *arg)
{
    
    pthread_mutex_lock(&mutex);


        for (int i = 0; i<bomb.size(); i++)
        {
            renderbullet(bomb[i]);//fire bullet,know its shape
        }



        pthread_mutex_unlock(&mutex);



    return NULL;
}




void tanksgame()//play
{
   // int fhw =0;

  //  while(true)
    //{ 

        initmap();

         for (int i = 0; i < 2; ++i)
 {
    rendertank(player[i]);//inital tank shape
     
 }

       
 for (int i = 0; i < 2; ++i)
 {
    rendertank(enemy[i]);
     
 }
        
        //inital enemy tank shape
         for (int i = 0; i < 2; ++i)
 {
          playerposx[i] = player[i].pos.x;
        playerposy[i] = player[i].pos.y;
     
 }
       // playerposx = player.pos.x;
        //playerposy = player.pos.y;//my tank positon record

       for (int i = 0; i<2; i++)
        { 
        enemyposx[i]= enemy[i].pos.x;
        enemyposy[i] = enemy[i].pos.y;//enemy tank position record

 }


     //}
      
    pthread_create(&tid1, NULL, tffn, NULL);



/*input*/
        playermove();//get keyboard input
/*poll*/
        this_time = clock();//get time now

        if (this_time - time_counter1 >= CLOCKS_PER_SEC /3.0)//poll for if enemy die ,ask frequently
        {
            time_counter1 += CLOCKS_PER_SEC/3.0 ;
          for (int i = 0; i<2; i++)
        { 

                respawn(enemy[i]);// relive enemy tank
            }
                   for (int i = 0; i<2; i++)
        { 

                respawn(player[i]);
            }
               
    /*     char outflag[10];
            strcpy(outflag,"auto");

    char buf_out[BUFSIZE];
    strcpy(buf_out, strcat(outflag,"|"));
    
    write(fd, buf_out, BUFSIZE);
    usleep(1);
*/
           
                
            
        }

        if (this_time - time_counter2 >= CLOCKS_PER_SEC / 20.0)//poll for if bullet move ,ask frequently
        {
            time_counter2 += CLOCKS_PER_SEC / 20.0;
               pthread_mutex_lock(&mutex);
            for (int i = 0; i<bomb.size(); i++)
            {
                 bulletmove(bomb[i],i);// change bullet move trail
            }
               pthread_mutex_unlock(&mutex);
        }


         pthread_join(tid1, NULL);// for out

/*after input, set positon*/
  //         for (int i = 0; i<2; i++)
//        {
      //      resetpos(player[i]);
    //    }
            

/* set enemy positon*/
 //  for (int i = 0; i<2; i++)
   //     {
     //       resetpos(enemy[i]);
   // }
/* erase false bomb */
         pthread_mutex_lock(&mutex);
        for (int i = 0; i < bomb.size(); i++)
        {
            if (bomb[i].exist == false)
            {
                bomb.erase(bomb.begin()+i);
                f--;
            }
        }
        pthread_mutex_unlock(&mutex);
        //usleep(1000);
        out();//menu
            
    //    if ((player.life == false))
      //  {
        //    break;
        //}
       
        // usleep(10);
       
   // }//after quit
}


void *tfn(void *arg)
{
    
    pthread_mutex_lock(&mutex);
/*

 f++;
    bomb.push_back(bullet);
    bomb[f].pos.x = x;
    bomb[f].pos.y = y;
    bomb[f].exist = true;
    bomb[f].direction = direc;
    bomb[f].enemyshoot = true;

    */
    f++;
    bomb.push_back(bullet);
    bomb[f].pos.x = x;
    bomb[f].pos.y = y;
    bomb[f].exist = true;
    bomb[f].direction = direc;
    bomb[f].enemyshoot = enemyshootflag;

        pthread_mutex_unlock(&mutex);



    return NULL;
}








void fun_final(int signo)
{
     char b[BUFSIZE];
     while (read(finalpipe[0], b, BUFSIZE) <= 0);


char *aa = NULL;
            char fla[10];
          
         char x[10];


                            aa = strtok(b, "|");
                            strcpy(fla, aa);

                        

                            aa = strtok(NULL, "|");
                            strcpy(x, aa);

int byeget = atoi(x);

    lifes=3;
    points=0;
    flag=0;
    f=-1;
    enemylifes=3;
    time_counter1 = 0;
    time_counter2 = 0;


     byebye=byeget;
}

void fun_deal(int signo)
{


 char buf[BUFSIZE];
    while (read(pfd[0], buf, BUFSIZE) <= 0);

//move(Fieldy + 2, 0);
//printw("%s",buf);
 
 //   system("clear");
  // move( 3, 0);
   // printw("rec");
   // move(Fieldy + 4, 0);
  //  cout<<ss;
  //  sleep(10);
      


char *s = NULL;
            char flags[10];
           // char team [10];
         char loc_x_str[10], loc_y_str[10];
         char dir[10],iden_str[10],sh[10];
         int shh;
         char judge[10];
int judgenum;
int identi;
      //   char roomsr[10];
        // int sendfd; 


                            s = strtok(buf, "|");
                            strcpy(flags, s);

    if (strcmp(flags, "timeout") == 0)
    {
         clear();
         strcpy(notes,"check the network and try again, quit soon");
    
          out();
        refresh();
        return;
    }

      else  if (strcmp(flags, "changeok") == 0)
    {

        s = strtok(NULL, "|");
        strcpy(judge, s);
         judgenum = atoi(judge);
    }
    else{

                            s = strtok(NULL, "|");
                            strcpy(loc_x_str, s);

                            s = strtok(NULL, "|");
                            strcpy(loc_y_str, s);

                             s = strtok(NULL, "|");
                            strcpy(dir, s);
                        if (strcmp(flags, "ghpm") == 0)
                        {
                             s = strtok(NULL, "|");
                            strcpy(sh, s);
                             shh = atoi(sh);


                        }

                              s = strtok(NULL, "|");
                            strcpy(iden_str, s);

                           x = atoi(loc_x_str);
                           y = atoi(loc_y_str);
                           direc = atoi(dir);
                           identi = atoi(iden_str);
    }




if (strcmp(flags, "changeok") == 0){
    clear();
 if (judgenum== 1)
    {


    int swpy = player[p0].pos.y;
    int swpx =  player[p0].pos.x;
    int swpdirection = player[p0].direction;
    int swpshoot = player[p0].shoot;
  //  int swpsy = player[p0].spos.y;
   // int swpsx = player[p0].spos.x;
   // int swpsdir = player[p0].sdir;
    //int swpsshoot =  player[p0].sshoot;
    int swplife = player[p0].life;


    player[p0].pos.y = player[p1].pos.y;
    player[p0].pos.x =  player[p1].pos.x;
    player[p0].direction = player[p1].direction;
    player[p0].shoot = player[p1].shoot;
  //  player[p0].spos.y = player[p1].spos.y;
   // player[p0].spos.x = player[p1].spos.x;
   // player[p0].sdir = player[p1].sdir;
   // player[p0].sshoot =  player[p1].sshoot;
    player[p0].life = player[p1].life;


    player[p1].pos.y = swpy;
    player[p1].pos.x =  swpx;
    player[p1].direction = swpdirection;
    player[p1].shoot = swpshoot;
   // player[p1].spos.y = swpsy;
   // player[p1].spos.x = swpsx;
  //  player[p1].sdir = swpsdir;
   // player[p1].sshoot =  swpsshoot;
    player[p1].life = swplife;


        int swp=p0;
           p0=p1;
           p1=swp;


    }
else if (judgenum==2)
{
    int swpy = enemy[e0].pos.y;
    int swpx =  enemy[e0].pos.x;
    int swpdirection = enemy[e0].direction;
    int swpshoot = enemy[e0].shoot;
  //  int swpsy = enemy[e0].spos.y;
   // int swpsx = enemy[e0].spos.x;
   // int swpsdir = enemy[e0].sdir;
    //int swpsshoot =  enemy[e0].sshoot;
    int swplife = enemy[e0].life;


    enemy[e0].pos.y = enemy[e1].pos.y;
   enemy[e0].pos.x =  enemy[e1].pos.x;
    enemy[e0].direction =enemy[e1].direction;
    enemy[e0].shoot = enemy[e1].shoot;
 //  enemy[e0].spos.y = enemy[e1].spos.y;
//   enemy[e0].spos.x = enemy[e1].spos.x;
  //  enemy[e0].sdir = enemy[e1].sdir;
 //   enemy[e0].sshoot =  enemy[e1].sshoot;
    enemy[e0].life = enemy[e1].life;


    enemy[e1].pos.y = swpy;
    enemy[e1].pos.x =  swpx;
    enemy[e1].direction = swpdirection;
    enemy[e1].shoot = swpshoot;
//    enemy[e1].spos.y = swpsy;
 //   enemy[e1].spos.x = swpsx;
//    enemy[e1].sdir = swpsdir;
//    enemy[e1].sshoot =  swpsshoot;
    enemy[e1].life = swplife;



    int swp=e0;
           e0=e1;
           e1=swp;
   
}
enemymove();
 out();
        refresh();
}


else if (strcmp(flags, "ghpm") == 0){
    if (team== 'a')
    {
        if(identi==1){
 clear();
        player[p1].pos.x=x;
        player[p1].pos.y=y;
        player[p1].direction=direc;
     //   player[p1].spos.x=x;
      //  player[p1].spos.y=y;
       // player[p1].sdir=direc;
       // player[1].shoot=shh;

player[p1].shoot=1;
//        resetpos(player[0]);
        rendertank(player[p1]);

        if (player[p1].shoot==1)
        {
    char loc_x_str[10], loc_y_str[10],fir[10],teamstr[10];
     char flagforfire[10];
     strcpy(flagforfire,"f");
     strcpy(teamstr,"ai");
  
   // sprintf(teamstr, "%c", team);
    char room_str[10];
    sprintf(room_str, "%d", room);
    char buf_in[BUFSIZE];
    char buf_out[BUFSIZE];
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),loc_x_str ),"|" ),loc_y_str),"|"),teamstr),"|"),fir),"|"),room_str),"|"));
    strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(flagforfire,"|"),teamstr),"|"),room_str),"|"));

    write(fd, buf_out, BUFSIZE);
    usleep(100);
        }
       // initmap();
      out();
        refresh();
        }
        else if(identi==2){
 clear();
        enemy[e1].pos.x=x;
        enemy[e1].pos.y=y;
        enemy[e1].direction=direc;
      //  enemy[e1].spos.x=x;
      //  enemy[e1].spos.y=y;
      //  enemy[e1].sdir=direc;


       // enemy[e1].shoot=shh;
       enemy[e1].shoot=1;
     //   resetpos(enemy[0]);
        rendertank(enemy[e1]);
       // initmap();
      out();
        refresh();
        }
    }
    else if (team=='b')
    {
        if(identi==1){
 clear();
        enemy[e1].pos.x=x;
        enemy[e1].pos.y=y;
        enemy[e1].direction=direc;
      //  enemy[e1].spos.x=x;
      //  enemy[e1].spos.y=y;
      //  enemy[e1].sdir=direc;
        enemy[e1].shoot=1;
       // enemy[1].shoot=shh;
//        resetpos(player[0]);
        rendertank(enemy[e1]);
       // initmap();
      out();
        refresh();
        }
        else if(identi==2){
 clear();
       player[p1].pos.x=x;
        player[p1].pos.y=y;
        player[p1].direction=direc;
   //      player[p1].spos.x=x;
   //     player[p1].spos.y=y;
    //    player[p1].sdir=direc;
       // player[1].shoot=shh;
     player[p1].shoot=1;

     //   resetpos(enemy[0]);
        rendertank(player[p1]);
       // initmap();
         if (player[p1].shoot==1)
        {
    char loc_x_str[10], loc_y_str[10],fir[10],teamstr[10];
     char flagforfire[10];
     strcpy(flagforfire,"f");
     strcpy(teamstr,"bi");
  
   // sprintf(teamstr, "%c", team);
    char room_str[10];
    sprintf(room_str, "%d", room);
    char buf_in[BUFSIZE];
    char buf_out[BUFSIZE];
   // strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(flags,"|"),loc_x_str ),"|" ),loc_y_str),"|"),teamstr),"|"),fir),"|"),room_str),"|"));
    strcpy(buf_out, strcat(strcat(strcat(strcat(strcat(flagforfire,"|"),teamstr),"|"),room_str),"|"));

    write(fd, buf_out, BUFSIZE);
    usleep(25000);
        }

      out();
        refresh();
        }
    }
   

}

   
  else if (strcmp(flags, "w") == 0||strcmp(flags, "zzz") == 0||strcmp(flags, "s") == 0||strcmp(flags, "d") == 0)
    {
        if(identi==1){
 clear();
        player[p0].pos.x=x;
        player[p0].pos.y=y;
        player[p0].direction=direc;
//        resetpos(player[0]);
        rendertank(player[p0]);
       // initmap();
      out();
        refresh();
        }
        else if(identi==2){
 clear();
        enemy[e0].pos.x=x;
        enemy[e0].pos.y=y;
        enemy[e0].direction=direc;
     //   resetpos(enemy[0]);
        rendertank(enemy[e0]);
       // initmap();
      out();
        refresh();
        }

       
       

        //tanksgame();
    }
    else if (strcmp(flags, "f") == 0)
    {
        clear();
     //   enemyshoot();
        if(identi==1||identi==3){// my and myai shoot bullet
         //   usleep(50);
        //    move(Fieldx+5,0);
          //  printw("1");


        
    enemyshootflag = false;

   



  
}
     else if(identi==2||identi==4){//he and his ai shoot
       // usleep(73000);
         // move(Fieldx+5,0);
           // printw("1");
        enemyshootflag = true;




     }
  /*   else if(identi==3){
       // usleep(73000);
         // move(Fieldx+5,0);
           // printw("1");
    f++;
    bomb.push_back(bullet);
    bomb[f].pos.x = x;
    bomb[f].pos.y = y;
    bomb[f].exist = false;
    bomb[f].direction = direc;
    bomb[f].enemyshoot = true;

     }
     else if(identi==4){
       // usleep(73000);
         // move(Fieldx+5,0);
           // printw("1");
    f++;
    bomb.push_back(bullet);
    bomb[f].pos.x = x;
    bomb[f].pos.y = y;
    bomb[f].exist = true;
    bomb[f].direction = direc;
    bomb[f].enemyshoot = true;

     }*/

    //       for (int i = 0; i<bomb.size(); i++)
      //  {
        //    renderbullet(bomb[i]);//fire bullet,know its shape
        //}
     
    pthread_create(&tid, NULL, tfn, NULL);


    
 pthread_join(tid, NULL);


out();
refresh();
       // tanksgame();

        
    }
    else if (strcmp(flags, "h") == 0)
    {
            clear();
             if(identi==1){//me to me
                enemylifes--;
                points=x;
                lifes=y;
                if (team=='a')
                {
                    enemy[e0].life=false;
                }
                else{
                    enemy[e0].life=false;
                }
                enemy[e0].life = false;

             }
                else if(identi==2){//he to me
                    enemypoints++;
                    points=x;
                    lifes=y;
                    if (team=='a')
                    {
                       player[p0].life = false;
                    }
                    else
                    {
                       player[p0].life = false;
                    }
                    
                }
         
       
         bomb[direc].exist = false;

         respawn(player[p0]);
         respawn(enemy[e0]);
         rendertank(player[p0]);
         rendertank(enemy[e0]);
         renderbullet(bomb[direc]);
           
          //  b.exist = false;
       
          //  points++;


         out();
refresh();
         //    move(Fieldy + 6, 0);
           //  printw("%d",points);
        //points++;
    }

     else if (strcmp(flags, "hai") == 0){
 clear();
             if(identi==1){//me to me
                points=x;
                lifes=y;
                enemy[e1].life = false;

             }
                else if(identi==2){//he to me
               
                    points=x;
                    lifes=y;
                    player[p1].life = false;
                     
                }
         
       
       
 bomb[direc].exist = false;
         respawn(player[p1]);
         respawn(enemy[e1]);
         rendertank(player[p1]);
         rendertank(enemy[e1]);
         renderbullet(bomb[direc]);
           
          //  b.exist = false;
       
          //  points++;


         out();
refresh();
     }

    else if (strcmp(flags, "o") == 0)
    {
        clear();
       //!!!!!! enemylifes=y;
        if(identi==1){//me to me
                
        points=x;
        lifes=y;
               // enemy.life = false;

             }
                else if(identi==2){//he to me
                  //  points=x;
                    enemylifes=y;
                    enemypoints=x;
                  //  lifes=y;
                   // player.life = false;
                }
        
out();
refresh();
    }
    else 
    {
  //      printf("something happened..\n");
    //    exit(1);
    }
}

void initmap(){


     for (int i = 0; i < Fieldy; i++)
    {
        for (int j = 0; j < Fieldx; j++)
        {

    field[i][j]=savefield[i][j];
}
}



}


void signalHandler(int signo)
{
    switch (signo){
        case SIGALRM:
             if (team=='b')
            {
               enemymove();//move relive enemy tank auto
//
            }
            break;
   }
}


