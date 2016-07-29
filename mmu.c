示例代码解析：
开启MMU，并将虚拟地址0xA0000000~0xA0100000映射到物理地址0x56000000~0x56100000(GPFCON物理地址为0x56000050，GPFDAT物理地址为0x56000054)；将虚拟地址0xB0000000~0xB3FFFFFF映射到物理地址0x30000000~0x33FFFFFF。本示例以段的方式进行地址映射，只使用一级页表，通过上面内容可知一级页表使用4096个描述符来表示4G空间(每个描述符对应1MB)，每个描述符占4字节，所以一级页表占16KB。使用SDRAM的开始16KB存放一级页表，所以剩下的内存开始地址就为0x30004000，这个地址最终会对应虚拟地址0xB0004000(所以代码运行地址为0xB0004000)。
★程序执行主要流程的示例代码。
.text
.global _start
_start:
    bl  disable_watch_dog                   @ 关闭WATCHDOG，否则CPU会不断重启
    bl  mem_control_setup                  @ 设置存储控制器以使用SDRAM
    ldr sp, =4096                                    @ 设置栈指针，以下是C函数调用前需要设好栈
    bl  copy_2th_to_sdram                   @ 将第二部分代码复制到SDRAM
    bl  create_page_table                     @ 设置页表
    bl  mmu_init                                      @ 启动MMU，启动以后下面代码都用虚拟地址
    ldr sp, =0xB4000000                       @ 重设栈指针，指向SDRAM顶端(使用虚拟地址)
    ldr pc, =0xB0004000                        @ 跳到SDRAM中继续执行第二部分代码
halt_loop:
    b   halt_loop
★设置页表。
void create_page_table(void)
{
	/* 
	* 用于段描述符的一些宏定义：[31:20]段基址，[11:10]AP，[8:5]Domain，[3]C，[2]B，[1:0]0b10为段描述符
	*/ 
	#define MMU_FULL_ACCESS     (3 << 10)   /* 访问权限AP */
	#define MMU_DOMAIN          (0 << 5)    /* 属于哪个域 Domain*/
	#define MMU_SPECIAL         (1 << 4)    /* 必须是1 */
	#define MMU_CACHEABLE       (1 << 3)    /* cacheable C位*/
	#define MMU_BUFFERABLE      (1 << 2)    /* bufferable B位*/
	#define MMU_SECTION         (2)         /* 表示这是段描述符 */
	#define MMU_SECDESC         (MMU_FULL_ACCESS | MMU_DOMAIN | MMU_SPECIAL | MMU_SECTION)
	#define MMU_SECDESC_WB      (MMU_FULL_ACCESS | MMU_DOMAIN | MMU_SPECIAL | MMU_CACHEABLE | MMU_BUFFERABLE | MMU_SECTION)
	#define MMU_SECTION_SIZE    0x00100000        /*每个段描述符对应1MB大小空间*/


    unsigned long virtuladdr, physicaladdr;
    unsigned long *mmu_tlb_base = (unsigned long *)0x30000000;        /*SDRAM开始地址存放页表*/
    
    /*
     * Steppingstone的起始物理地址为0，第一部分程序的起始运行地址也是0， 为了在开启MMU后仍能运行第一部分的程序， 将0～1M的虚拟地址映射到同样的物理地址
     */
    virtuladdr = 0;
    physicaladdr = 0;
    /*虚拟地址[31:20]用于索引一级页表，找到它对应的描述符，对应于(virtualaddr>>20)。段描述符中[31:20]保存段的物理地址，对应(physicaladdr & 0xFFF00000)*/
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | MMU_SECDESC_WB;

    /*
     * 0x56000000是GPIO寄存器的起始物理地址，GPBCON和GPBDAT这两个寄存器的物理地址0x56000010、0x56000014， 为了在第二部分程序中能以地址0xA0000010、0xA0000014来操作GPBCON、GPBDAT，
     * 把从0xA0000000开始的1M虚拟地址空间映射到从0x56000000开始的1M物理地址空间
     */
    virtuladdr = 0xA0000000;
    physicaladdr = 0x56000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | MMU_SECDESC;

    /*
     * SDRAM的物理地址范围是0x30000000～0x33FFFFFF， 将虚拟地址0xB0000000～0xB3FFFFFF映射到物理地址0x30000000～0x33FFFFFF上， 总共64M，涉及64个段描述符
     */
    virtuladdr = 0xB0000000;
    physicaladdr = 0x30000000;
    while (virtuladdr < 0xB4000000)
    {
        *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | MMU_SECDESC_WB;
        virtuladdr += MMU_SECTION_SIZE; 
        physicaladdr += MMU_SECTION_SIZE; 
    }
}
★ 启动MMU。
void mmu_init(void)
{
    unsigned long ttb = 0x30000000;


__asm__(
    "mov    r0, #0\n"
    "mcr    p15, 0, r0, c7, c7, 0\n"    /* 使无效ICaches和DCaches */
    
    "mcr    p15, 0, r0, c7, c10, 4\n"   /* drain write buffer on v4 */
    "mcr    p15, 0, r0, c8, c7, 0\n"    /* 使无效指令、数据TLB */
    
    "mov    r4, %0\n"                   /* r4 = 页表基址（ttb） */
    "mcr    p15, 0, r4, c2, c0, 0\n"    /* 设置页表基址寄存器 */
    
    "mvn    r0, #0\n"                   
    "mcr    p15, 0, r0, c3, c0, 0\n"    /* 域访问控制寄存器设为0xFFFFFFFF， 不进行权限检查*/    
    /* 
     * 对于控制寄存器，先读出其值，在这基础上修改感兴趣的位，然后再写入
     */
    "mrc    p15, 0, r0, c1, c0, 0\n"    /* 读出控制寄存器的值 */
    
    /* 控制寄存器的低16位含义为：.RVI ..RS B... .CAM
     * R : 表示换出Cache中的条目时使用的算法，0 = Random replacement；1 = Round robin replacement
     * V : 表示异常向量表所在的位置，0 = Low addresses = 0x00000000；1 = High addresses = 0xFFFF0000
     * I : 0 = 关闭ICaches；1 = 开启ICaches
     * R、S : 用来与页表中的描述符一起确定内存的访问权限
     * B : 0 = CPU为小字节序；1 = CPU为大字节序
     * C : 0 = 关闭DCaches；1 = 开启DCaches
     * A : 0 = 数据访问时不进行地址对齐检查；1 = 数据访问时进行地址对齐检查
     * M : 0 = 关闭MMU；1 = 开启MMU
     */
    
    /*  
     * 先清除不需要的位，往下若需要则重新设置它们    
     */
                                        /* .RVI ..RS B... .CAM */ 
    "bic    r0, r0, #0x3000\n"          /* ..11 .... .... .... 清除V、I位 */
    "bic    r0, r0, #0x0300\n"          /* .... ..11 .... .... 清除R、S位 */
    "bic    r0, r0, #0x0087\n"          /* .... .... 1... .111 清除B/C/A/M */


    /*
     * 设置需要的位
     */
    "orr    r0, r0, #0x0002\n"          /* .... .... .... ..1. 开启对齐检查 */
    "orr    r0, r0, #0x0004\n"          /* .... .... .... .1.. 开启DCaches */
    "orr    r0, r0, #0x1000\n"          /* ...1 .... .... .... 开启ICaches */
    "orr    r0, r0, #0x0001\n"          /* .... .... .... ...1 使能MMU */
    
    "mcr    p15, 0, r0, c1, c0, 0\n"    /* 将修改的值写入控制寄存器 */
    : /* 无输出 */
    : "r" (ttb) );
}
