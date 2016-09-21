#include <unistd.h>  
#include <sys/types.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <signal.h>  
  
static int alarm_fired = 0;  
  
void ouch(int sig)  
{  
    alarm_fired = 1;  
}  
  
int main()  
{  
    pid_t pid;  
    pid = fork();  
    switch(pid)  
    {  
    case -1:  
        perror("fork failed\n");  
        exit(1);  
    case 0:  
        //�ӽ���  
        sleep(5);  
        //�򸸽��̷����ź�  
        kill(getppid(), SIGALRM);  
        exit(0);  
    default:;  
    }  
    //���ô�����  
    signal(SIGALRM, ouch);  
    while(!alarm_fired)  
    {  
        printf("Hello World!\n");  
        sleep(1);  
    }  
    if(alarm_fired)  
        printf("\nI got a signal %d\n", SIGALRM);  
  
    exit(0);  
}  