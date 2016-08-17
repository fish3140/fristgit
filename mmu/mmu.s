.text 
.global _start
_start:
			ldr sp , =4096
			bl close_watchdog
			bl ram_init
			bl copy_code
			bl setup_page
			bl start_mmu
			ldr pc , =0xE4000000
			ldr pc , =main
Loop:
			b Loop
		
