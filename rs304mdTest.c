#include<stdio.h>
#include<stdlib.h>
#include<strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

//サーボIDをデフォルトの1に設定。自分のサーボIDが違う人はここで設定
#define ID_SERVO1 1

//Raspberry Piのポート設定
#define SERIAL_PORT "/dev/tty.usbserial-A90172IE"
#define BAUDRATE B115200 //Futabaサーボのデフォルトの通信レート(bps)

#define ON 1
#define OFF 0

char get_check_sum(char *com, int buf_size);
void servo_torque_set(int fd, int servo_id, int mode);
void move_target_degree(int fd, int servo_id, float degree);

int main(int argc, char *argv[])
{
  int fd; //シリアルポートのファイルデスクリプタ
  char *com;
  struct termios newtio, oldtio;    //ターミナルの設定

  printf("connect start...\n");
  // if(!(fd = open(SERIAL_PORT, O_RDWR | O_NONBLOCK ))){
  fd = open(SERIAL_PORT, O_RDWR);
  // fd = open(SERIAL_PORT, O_RDWR | O_NONBLOCK);
  if(fd < 0){
    printf("Can not open a serial port\n");
    return -1;
  }   //シリアルポートを開く
  printf("Open a serial port\n");
  // ioctl(fd, TCGETS, &oldtio);  //ターミナルの設定を読み込む（現在の設定を保存)
  tcgetattr(fd, &oldtio);

  bzero(&newtio, sizeof(newtio)); //ターミナルの設定用にメモリを確保
  newtio = oldtio;                  //ターミナルの現在設定をコピー
  newtio.c_cflag = (BAUDRATE | CS8 | CLOCAL | CREAD); //ボーレート等を設定
  newtio.c_iflag = (IGNPAR | ICRNL);
  newtio.c_oflag = 0;
  newtio.c_lflag = ICANON; //カノニカルモードで起動

  ioctl(fd, TCSETS, &newtio);       //ターミナルの設定を書き込み
  // tcsetattr(fd, TCSANOW, &newtio);
  //トルクをオン
  servo_torque_set(fd, ID_SERVO1, ON);
  printf("torque on\n");
  //サーボを指定角度まで移動
  move_target_degree(fd, ID_SERVO1, 90.0);
  printf("servo 90\n");
  sleep(1);
  move_target_degree(fd, ID_SERVO1, -90.0);
  sleep(2);
  move_target_degree(fd, ID_SERVO1, 0);
  sleep(1);
  printf("servo 0\n");

  //トルクをオフ
  servo_torque_set(fd, ID_SERVO1, OFF);

  ioctl(fd, TCSETS, &oldtio);       //ターミナルの設定を元に戻しておく
  close(fd);                        //シリアルポートを閉じる

  return 0; //終了
}


//Check sum generate
char get_check_sum(char *com, int buf_size)
{
  char check_sum;
  int i;

  check_sum = com[2];

  for (i = 3; i < buf_size - 1; i++)
  {
    check_sum = (char)(check_sum ^ com[i]);
  }
  return check_sum;
}


//Torque on
void servo_torque_set(int fd, int servo_id, int mode)
{
  int i;
  char com[9];
  i = 0;

  com[i++] = (char)0xFA;          //Header
  com[i++] = (char)0xAF;          //Header
  com[i++] = (char)servo_id;      //ID
  com[i++] = (char)0;             //Flag
  com[i++] = (char)0x24;          //Torque map address
  com[i++] = (char)0x01;          //length
  com[i++] = (char)0x01;          //count

  //Torque on
  if(mode == ON){
    com[i++] = (char)0x01;
  }
  else{ //OFF
    com[i++] = (char)0;
  }

  com[i++] = get_check_sum(com, i+1);

  if(com !=NULL)
  {
    write(fd, com, sizeof(com)*9);      /* デバイスへコマンド書き込み */
  }
}

//target degree set and move
void move_target_degree(int fd, int servo_id, float degree)
{
  char com[10];
  char l_bit;
  char h_bit;
  int target; // target degree

  //data calculation
  target =(int)degree*10;
  h_bit = (char)((target & 0xFF00) >> 8);
  l_bit = (char)(target & 0xFF);

  // packet set
  com[0] = 0xFA; //Header
  com[1] = 0xAF; //Header
  com[2] = (char)servo_id; //ID
  com[3] = 0x00; //Flag
  com[4] = 0x1E; //map address
  com[5] = 0x02; //length
  com[6] = 0x01; //count
  com[7] = l_bit; //data lower
  com[8] = h_bit; //data higher
  com[9] = get_check_sum(com, 10);

  if(fd){
    write(fd, com, sizeof(com)*10);
  }
}
