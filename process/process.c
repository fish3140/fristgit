#include<unistd.h>
#include<stdio.h>
int main()
{
	pid_t pid;
	int i=0;
	pid=fork();
	if(pid<0)
		printf("fork error...\n");
	else if(pid=0)
	{
		i++;
		printf("this is child,i is %d..\n",i);
	}
	else
	{
		i=i+2;
		printf("this is a parent..., i is %d..\n",i);
	}

}