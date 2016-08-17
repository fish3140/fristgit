#define GPBCON  (*(volatile unsigned long *)0xD0000010)
#define GPBDAT  (*(volatile unsigned long *)0xD0000014)
void main()
{
	int i=0;
	GPBCON = (1<<10)|(1<<12)|(1<<14)|(1<<16);
	for(i=0;i<=10;i++)
	{
		for(;i<50;i++);
		GPBDAT |= (1<<8);
		for(;i<50;i++);
		GPBDAT &= ~(1<<8);
		for(;i<50;i++);
	}
	
}