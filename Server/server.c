#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h> 
#include <pthread.h> 

#define SERVER_PORT 5430
#define MC_DATA_PORT_1 5450
#define MC_STREAM_PORT_1 5451
#define MC_DATA_PORT_2 5452
#define MC_STREAM_PORT_2 5453
#define MC_LIVE_STREAM_PORT 5454
#define BUF_SIZE 1024

#define MAX_PENDING 5
#define MAX_LINE 256
#define MAX_SENDERS 5

int timeLeft[MAX_SENDERS]={0};
int endFlag[MAX_SENDERS]={0};

struct data {
  char buffer[BUF_SIZE];
};

struct streamInfo {
  char songName[200];
  char nextSongName[200];
  int secLeft;
};

struct station_info {
    uint8_t station_number;
    uint8_t station_name_size;
    char station_name[200];
    char multicast_address[16];
    uint16_t data_port;
    uint16_t info_port;
    uint32_t bit_rate;
};

struct site_info {
    uint8_t type;
    uint8_t site_name_size;
    char site_name[200];
    uint8_t site_desc_size;
    char site_desc[200];
    uint8_t station_count;
    struct station_info station_list[2];
};

struct senderArguments
{
    int stationNumber;
    int sData;
    int sStream;
    struct sockaddr_in sinData;
    struct sockaddr_in sinStream;
};

struct liveSenderArgumemnts
{
    int stationNumber;
    int sLive;
    struct sockaddr_in sinLive;
};

void delay(int t)
{
    long pause;
    clock_t now,then;

    pause = t*(CLOCKS_PER_SEC/1000000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

struct senderArguments getSenderArguments(char * mcast_addr)
{
  int sData, sStream;
  struct sockaddr_in sinData;
  struct sockaddr_in sinStream;
  struct senderArguments sArg;
  int dLength = sizeof(sinData);
  int sLength = sizeof(sinStream); 
  int dPort, sPort;

  if(strcmp(mcast_addr, "225.1.1.1")==0)
  {
    dPort = MC_DATA_PORT_1;
    sPort = MC_STREAM_PORT_1;
    sArg.stationNumber = 1;
  }
  else
  {
    dPort = MC_DATA_PORT_2;
    sPort = MC_STREAM_PORT_2;
    sArg.stationNumber = 2;
  }
   
  if ((sData = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server: socket");
    exit(1);
  }

  if ((sStream = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server: socket");
    exit(1);
  }
 
  memset((char *)&sinData, 0, sizeof(sinData));
  memset((char *)&sinStream, 0, sizeof(sinStream));
  sinData.sin_family = AF_INET;
  sinData.sin_addr.s_addr = inet_addr(mcast_addr);
  sinData.sin_port = htons(dPort);

  sinStream.sin_family = AF_INET;
  sinStream.sin_addr.s_addr = inet_addr(mcast_addr);
  sinStream.sin_port = htons(sPort);

  sArg.sData = sData;
  sArg.sinData = sinData;
  sArg.sStream = sStream;
  sArg.sinStream = sinStream;
  return sArg;
}

struct liveSenderArgumemnts setupLiveSender(char * mcast_addr)
{
    int sLive;
    struct sockaddr_in sinLive;
    int liveLength = sizeof(sinLive);

    if((sLive = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
      perror("server: socket");
      exit(1);
    }

    memset((char *)&sinLive, 0, sizeof(sinLive));
    sinLive.sin_family = AF_INET;
    sinLive.sin_addr.s_addr = inet_addr(mcast_addr);
    sinLive.sin_port = htons(MC_LIVE_STREAM_PORT);

    struct liveSenderArgumemnts lsArg;
    lsArg.stationNumber = 3;
    lsArg.sLive = sLive;
    lsArg.sinLive = sinLive;
    return lsArg;
}

void *sendLiveStream(struct liveSenderArgumemnts * temp)
{
  int n = 0, packet_index = 0;
  struct liveSenderArgumemnts lsArg;
  lsArg.stationNumber = temp->stationNumber;
  lsArg.sLive = temp->sLive;
  lsArg.sinLive = temp->sinLive;

  struct data d;
  memset(d.buffer, "\0", sizeof(d.buffer));

  FILE* fp = NULL;

  char a[1000], b[23], t[48];
  strcpy(a, "/home/manav/Videos/Webcam/");
  fp = fopen("final.txt", "r");
  if(fp == NULL)
  {
      system("touch final.txt");
  }
  else
  {
      system("rm final.txt");
      system("touch final.txt");
  }
  fp = NULL;
  fp = fopen("final.txt","r");
  system("ls -Art /home/manav/Videos/Webcam | tail -n 1 >> final.txt");

  memset(b, '\0', sizeof(b));  
  fread(b, 1, sizeof(b), fp);   
  strcat(a, b);
  for(int e = 0 ; e < 48 ; e++)
  {
      t[e] = a[e];
  }

  FILE* fp2 = NULL;
  fp2 = fopen(t, "r");

  while(1)
  {
      memset(d.buffer, '\0', sizeof(d.buffer));
      n = fread(d.buffer, 1, sizeof(d.buffer), fp2);

      if(sendto(lsArg.sLive, &d, n, 0, (struct sockaddr *)&lsArg.sinLive, sizeof(lsArg.sinLive)) < 0)
      {
          fprintf(stderr, "error while sending data!\n");
          exit(-1);
      }

      packet_index++;
      printf("Current packet size: %d\n", n);
      //printf("Live Stream-Packet Index: %d\n", packet_index);
      delay(1000);
      // if(packet_index%nPackets==0 && timeLeft[sArg.stationNumber-1]!=0)
      // {
      //     timeLeft[sArg.stationNumber-1]--;
      //     start = clock();
      //     executionTime = (double)(start-prevClock) / CLOCKS_PER_SEC;
      //     executionTime = 0.9 - executionTime;
      //     printf("Delay: %lf\n", executionTime);
      //     usleep((int)(executionTime*1000000));
      //     prevClock = clock();
      // }
  }
  printf("Live Stream complete!\n");

}

void *sendStreamThread(struct senderArguments * temp)
{
  struct senderArguments sArg;
  sArg.stationNumber = temp->stationNumber;
  sArg.sStream = temp->sStream;
  sArg.sinStream = temp->sinStream;

  struct streamInfo sInfo;
  if(sArg.stationNumber==1)
  {
    strcpy(sInfo.songName, "Song-1");
    strcpy(sInfo.nextSongName, "Song-2");
  }
  else
  {
    strcpy(sInfo.songName, "Song-7");
    strcpy(sInfo.nextSongName, "Song-8");
  }

  while(1)
  {
    if(endFlag[sArg.stationNumber-1]==1)
      break;
    sInfo.secLeft = timeLeft[sArg.stationNumber-1];
    printf("%d-time left: %d\n", sArg.stationNumber, sInfo.secLeft);

    if(sendto(sArg.sStream, &sInfo, sizeof(sInfo), 0, (struct sockaddr *)&sArg.sinStream, sizeof(sArg.sinStream)) == -1)
    {
        fprintf(stderr, "error while sending data!\n");
        exit(-1);
    }
    sleep(1);
  }
  close(sArg.sStream);
}

void *sendDataThread(struct senderArguments * temp)
{

  int nPackets = 0, nBlocks = 0, n = 0, count = 0;
  int duration = 0, size = 0, packet_index = 1;
  int h,m,s;
  char fileDuration[12];
  clock_t start, prevClock;
  double executionTime=0.0;

  struct senderArguments sArg;

  sArg.stationNumber = temp->stationNumber;
  sArg.sData = temp->sData;
  sArg.sinData = temp->sinData;

  FILE *picture;
  FILE* fp;

  if(sArg.stationNumber == 1)
  {
    picture = fopen("input1.mp4", "r");
    if(picture==NULL)
    {
      printf("Error in opening input1.mp4");
      exit(1);
    }   
    fp = fopen("duration1.txt", "r");  
    if(fp==NULL)
    {
      system("touch duration1.txt");
    }
    else
    {
      system("rm duration1.txt");
      system("touch duration1.txt");
    }
    system("ffmpeg -i input1.mp4 2>&1 | grep Duration | cut -d ' ' -f 4 | sed s/,// >> duration1.txt");
  }
  else if(sArg.stationNumber == 2)
  {
    picture = fopen("input2.mp4", "r");   
    if(picture==NULL)
    {
      printf("Error in opening input2.mp4");
      exit(1);
    }   
    fp = fopen("duration2.txt", "r");  
    if(fp==NULL)
    {
      system("touch duration2.txt");
    }
    else
    {
      system("rm duration2.txt");
      system("touch duration2.txt");
    }
    system("ffmpeg -i input2.mp4 2>&1 | grep Duration | cut -d ' ' -f 4 | sed s/,// >> duration2.txt");
  }

  fread(fileDuration, sizeof(fileDuration), 1, fp);
  //printf("2-Duration Before: %s\n", fileDuration);
  h = ((int)(fileDuration[0]) - 48)*10 + (int)fileDuration[1] - 48;
  m = ((int)(fileDuration[3]) - 48)*10 + (int)fileDuration[4] - 48;
  s = ((int)(fileDuration[6]) - 48)*10 + (int)fileDuration[7] - 48;
  duration = h * 60 * 60 + m * 60 + s;
  printf("Total File Duration:  %d\n", duration);

  timeLeft[sArg.stationNumber-1]=duration;

  fseek(picture, 0, SEEK_END);
  size = ftell(picture);
  fseek(picture, 0, SEEK_SET);
  printf("Total File size: %d\n",size);

  if(size%duration == 0)
      nBlocks = size/duration;
  else
      nBlocks = size/duration + 1;

  if(nBlocks%BUF_SIZE == 0)
      nPackets = nBlocks/BUF_SIZE;
  else
      nPackets = nBlocks/BUF_SIZE + 1;

  struct data d;
  memset(d.buffer, "\0", sizeof(d.buffer));
  prevClock = clock();

  while(!feof(picture))
  {
      n = fread(d.buffer, 1, sizeof(d.buffer), picture);
      count += n;
      if(sendto(sArg.sData, &d, n, 0, (struct sockaddr *)&sArg.sinData, sizeof(sArg.sinData)) < 0)
      {
          fprintf(stderr, "error while sending data!\n");
          exit(-1);
      }

      packet_index++;
      if(packet_index%nPackets==0 && timeLeft[sArg.stationNumber-1]!=0)
      {
          timeLeft[sArg.stationNumber-1]--;
          start = clock();
          executionTime = (double)(start-prevClock) / CLOCKS_PER_SEC;
          executionTime = 0.8 - executionTime;
          printf("Delay: %lf\n", executionTime);
          usleep((int)(executionTime*1000000));
          prevClock = clock();
      }

  }
  printf("%d-Packets sent: %d\n", sArg.stationNumber, packet_index);
  printf("%d-Total bytes sent: %d\n", sArg.stationNumber, count);
  endFlag[sArg.stationNumber-1]=1;

  close(sArg.sData);

}

int main(int argc, char * argv[]){

  struct sockaddr_in sin;
  char buf[MAX_LINE];
  int s, new_s;
  char str[INET_ADDRSTRLEN];
  int serverPort = SERVER_PORT;

  if(argc==2)
    sscanf(argv[1], "%d", &serverPort);
  else
  {
    printf("Invalid arguments!\n");
    exit(1);
  }

  /* build address data structure */
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(serverPort);
  int len = sizeof(sin);
  /* setup passive open */
  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("simplex-talk: socket");
    exit(1);
  }
 
  inet_ntop(AF_INET, &(sin.sin_addr), str, INET_ADDRSTRLEN);
  printf("Server is using address %s and port %d.\n", str, serverPort);

  if ((bind(s, (struct sockaddr *)&sin, len)) < 0) {
    perror("simplex-talk: bind");
    exit(1);
  }
  else
    printf("Server bind done.\n");

  int count=0;

  struct station_info station1;
  station1.station_number = 1;
  station1.station_name_size = 10;
  strcpy(station1.station_name, "Station1");
  strcpy(station1.multicast_address, "225.1.1.1");
  station1.data_port = 2000;
  station1.info_port = 3000;
  station1.bit_rate = 2000000;

  struct station_info station2;
  station2.station_number = 2;
  station2.station_name_size = 10;
  strcpy(station2.station_name, "Station2");
  strcpy(station2.multicast_address, "225.2.1.1");
  station2.data_port = 2001;
  station2.info_port = 3001;
  station2.bit_rate = 2000000;

  struct site_info site1;
  site1.type = 1;
  site1.site_name_size = 200;
  strcpy(site1.site_name, "Site1");
  site1.site_desc_size = 200;
  strcpy(site1.site_desc, "Desc");
  site1.station_count = 2;

  site1.station_list[0] = station1;
  site1.station_list[1] = station2;

  listen(s, MAX_PENDING);
  printf("Listen complete!\n");

  /* wait for connection, then receive and print text */
  while(1) 
  {
    if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
      perror("simplex-talk: accept");
      exit(1);
    }
    count++;
    printf("Sending Station List to Client!\n");
    printf("count: %d\n", count);

    printf("Hello!\n");
    send(new_s, &site1, sizeof(site1), 0);
    close(new_s);
    printf("TCP Connection Closed!!\n");

    struct senderArguments sender1, sender2;
    struct liveSenderArgumemnts sender3;
    sender1 = getSenderArguments(station1.multicast_address);
    sender2 = getSenderArguments(station2.multicast_address);
    sender3 = setupLiveSender("225.3.1.1");   // Live Stream Multicast Address

    pthread_t thread1, thread2, thread3, thread4, thread5;
    pthread_create(&thread1, NULL, sendDataThread, &sender1); 
    pthread_create(&thread2, NULL, sendStreamThread, &sender1); 
    pthread_create(&thread3, NULL, sendDataThread, &sender2); 
    pthread_create(&thread4, NULL, sendStreamThread, &sender2); 
    // pthread_create(&thread5, NULL, sendLiveStream, &sender3);
    //sendData(s, &sin, sLength);

  }

}
