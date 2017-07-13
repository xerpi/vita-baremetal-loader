#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/display.h>
#include <taihen.h>
typedef unsigned int u32;

#define UART0_PRINTF(s, ...) \
	do { \
		char _buffer[256]; \
		snprintf(_buffer, sizeof(_buffer), s, ##__VA_ARGS__); \
		uart0_print(_buffer); \
	} while (0)

void _start() __attribute__ ((weak, alias ("module_start")));

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#define PAYLOAD_PADDR ((void *)0x00000000)

extern const unsigned char _binary_payload_bin_start;
extern const unsigned char _binary_payload_bin_size;

static const unsigned int payload_size = (unsigned int)&_binary_payload_bin_size;
static const void *const payload_addr = (void *)&_binary_payload_bin_start;

extern void trampoline(void);

#define SYSCON_CMD_RESET_DEVICE	0x0C

#define SYSCON_RESET_POWEROFF	0x00
#define SYSCON_RESET_SUSPEND	0x01
#define SYSCON_RESET_SOFT_RESET	0x11

extern int kscePowerRequestStandby(void);

extern int ksceUartReadAvailable(int bus);
extern int ksceUartWrite(int bus, unsigned char data);
extern int ksceUartRead(int bus);
extern int ksceUartInit(int bus);

extern int ScePervasiveForDriver_18DD8043(int uart_bus);
extern int ScePervasiveForDriver_788B6C61(int uart_bus);
extern int ScePervasiveForDriver_A7CE7DCC(int uart_bus);
extern int ScePervasiveForDriver_EFD084D8(int uart_bus);

extern int ksceSysconResetDevice(int type, int unk);
extern int ksceSysconSendCommand(unsigned int cmd, void *args, int size);


static void uart0_print(const char *str)
{
	while (*str) {
		ksceUartWrite(0, *str);
		if (*str == '\n')
			ksceUartWrite(0, '\r');
		str++;
	}
}

static unsigned long get_cpu_id(void);
static unsigned long get_ttbr0(void);
static unsigned long get_ttbcr(void);
static unsigned long get_paddr(unsigned long vaddr);
static int find_paddr(unsigned long paddr, unsigned long vaddr, unsigned int size,
		      unsigned int step, unsigned long *found_vaddr);
static unsigned long page_table_entry(unsigned long paddr);
static void map_scratchpad(void);

typedef struct SceSysconSuspendContext {
	unsigned int size;
	unsigned int unk;
	unsigned int buff_vaddr;
	unsigned int resume_func_vaddr;
	unsigned int SCTLR;
	unsigned int ACTLR;
	unsigned int CPACR;
	unsigned int TTBR0;
	unsigned int TTBR1;
	unsigned int TTBCR;
	unsigned int DACR;
	unsigned int PRRR;
	unsigned int NMRR;
	unsigned int VBAR;
	unsigned int CONTEXTIDR;
	unsigned int TPIDRURW;
	unsigned int TPIDRURO;
	unsigned int TPIDRPRW;
	unsigned int unk2[6];
	unsigned long long time;
} SceSysconSuspendContext;

static SceSysconSuspendContext suspend_ctx;
static unsigned int suspend_ctx_paddr = 0;

static tai_hook_ref_t SceSyscon_ksceSysconResetDevice_ref;
static SceUID SceSyscon_ksceSysconResetDevice_hook_uid = -1;
static tai_hook_ref_t SceSyscon_SceSysconForDriver_4278E614_ref;
static SceUID SceSyscon_SceSysconForDriver_4278E614_hook_uid = -1;
static tai_hook_ref_t SceSyscon_ksceSysconSendCommand_ref;
static SceUID SceSyscon_ksceSysconSendCommand_hook_uid = -1;

static tai_hook_ref_t ScePower_scePowerRequestStandby_ref;
static SceUID ScePower_scePowerRequestStandby_hook_uid = -1;
static tai_hook_ref_t ScePower_kscePowerRequestStandby_ref;
static SceUID ScePower_kscePowerRequestStandby_hook_uid = -1;

static tai_hook_ref_t SceLowio_ScePervasiveForDriver_788B6C61_ref;
static SceUID SceLowio_ScePervasiveForDriver_788B6C61_hook_uid = -1;

static tai_hook_ref_t SceLowio_ScePervasiveForDriver_81A155F1_ref;
static SceUID SceLowio_ScePervasiveForDriver_81A155F1_hook_uid = -1;


static int ksceSysconResetDevice_hook_func(int type, int unk)
{
	UART0_PRINTF("ksceSysconResetDevice(0x%08X, 0x%08X), CPU ID: %ld\n", type, unk, get_cpu_id());

	if (type == 0)
		type = SYSCON_RESET_SOFT_RESET;

	//((void (*)(void))PAYLOAD_PADDR)();

	return TAI_CONTINUE(int, SceSyscon_ksceSysconResetDevice_ref, type, unk);
}

static int SceSysconForDriver_4278E614_hook_func(int unk)
{
	UART0_PRINTF("SceSysconForDriver_4278E614(0x%08X)\n", unk);
	return TAI_CONTINUE(int, SceSyscon_SceSysconForDriver_4278E614_ref, unk);
}

static int ksceSysconSendCommand_hook_func(int cmd, void *buffer, unsigned int size)
{
	UART0_PRINTF("ksceSysconSendCommand(0x%08X, %p, 0x%08X)\n", cmd, buffer, size);

	if (cmd == SYSCON_CMD_RESET_DEVICE)
		buffer = &suspend_ctx_paddr;

	return TAI_CONTINUE(int, SceSyscon_ksceSysconSendCommand_ref, cmd, buffer, size);
}

static int scePowerRequestStandby_hook_func(void)
{
	UART0_PRINTF("scePowerRequestStandby\n");
	return TAI_CONTINUE(int, ScePower_scePowerRequestStandby_ref);
}

static int kscePowerRequestStandby_hook_func(void)
{
	UART0_PRINTF("kscePowerRequestStandby\n");
	return TAI_CONTINUE(int, ScePower_kscePowerRequestStandby_ref);
}

static int ScePervasiveForDriver_788B6C61_hook_func(int uart_bus)
{
	UART0_PRINTF("ScePervasiveForDriver_788B6C61(0x%08X)\n", uart_bus);
	//return TAI_CONTINUE(int, SceLowio_ScePervasiveForDriver_788B6C61_ref, uart_bus);
	return 0;
}

static int ScePervasiveForDriver_81A155F1_hook_func(void)
{
	static unsigned int tmp[0x100];
	UART0_PRINTF("ScePervasiveForDriver_81A155F1()\n");
	//return TAI_CONTINUE(int, SceLowio_ScePervasiveForDriver_81A155F1_ref);
	return (int)tmp;
}
/*
static int __attribute__((naked)) resume_func(u32 TTBR1, u32 CONTEXTIDR, u32 DACR)
{
	asm volatile("dsb\n\t");
	asm volatile("mcr p15, 0, %0, c13, c0, 1\n\t" : : "r"(0)); // CONTEXTIDR
	asm volatile("isb\n\t");
	asm volatile("mcr p15, 0, %0, c2, c0, 1\n\t" : : "r"(TTBR1));
	asm volatile("isb\n\t");
	asm volatile("dsb\n\t");
	asm volatile("mcr p15, 0, %0, c13, c0, 1\n\t" : : "r"(CONTEXTIDR));
	asm volatile("mcr p15, 0, %0, c3, c0, 1\n\t" : : "r"(DACR));
	asm volatile("dsb\n\t");
	asm volatile("isb\n\t");
	asm volatile("mcr p15, 0, %0, c8, c7, 0\n\t" : : "r"(0)); // TLBIALL (TLB Invalidate All)

	map_scratchpad();

	if (get_cpu_id() != 0) {
		while (1)
			;
	}

	ksceKernelCpuUnrestrictedMemcpy(PAYLOAD_PADDR, payload_addr, payload_size);
	ksceKernelCpuDcacheWritebackRange(PAYLOAD_PADDR, payload_size);
	ksceKernelCpuIcacheAndL2WritebackInvalidateRange(PAYLOAD_PADDR, payload_size);

	ScePervasiveForDriver_EFD084D8(0); // Turn on clock
	ScePervasiveForDriver_A7CE7DCC(0); // Out of reset

	ksceUartInit(0);
	ksceUartWrite(0, 'A');
	ksceUartWrite(0, 'B');
	ksceUartWrite(0, 'C');
	ksceUartWrite(0, 'D');

	((void (*)(void))PAYLOAD_PADDR)();

	return 0;
}
*/
static SceUID sema;

#if 0
static int payload_trampoline_thread(SceSize args, void *argp)
{
	map_scratchpad();

	ksceKernelSignalSema(sema, 1);

	if (get_cpu_id() == 0) {
		ksceKernelWaitSema(sema, 4, NULL);

		/*
		 * Copy the payload to the scratchpad.
		 */
		ksceKernelCpuUnrestrictedMemcpy(PAYLOAD_PADDR, payload_addr, payload_size);
		ksceKernelCpuDcacheWritebackRange(PAYLOAD_PADDR, payload_size);
		ksceKernelCpuIcacheAndL2WritebackInvalidateRange(PAYLOAD_PADDR, payload_size);

		kscePowerRequestStandby();
	}

	while (1)
		;

	return 0;
}
#endif

static char hex[] = "0123456789ABCDEF";

#define UART0_PRINT_HEX(x) \
	do { \
		for (int i = 28; i >= 0; i -= 4) \
			ksceUartWrite(0, hex[((x) >> i) & 0xF]); \
		ksceUartWrite(0, '\n'); \
		ksceUartWrite(0, '\r'); \
	} while (0)

static void __attribute__((naked, noreturn)) resume_func(u32 TTBR1, u32 CONTEXTIDR, u32 DACR)
{
	/*
	 * Restore Vita OS context.
	 */
	asm volatile(
		"dsb\n\t"
		"mcr p15, 0, %0, c13, c0, 1\n\t"
		"isb\n\t"
		"mcr p15, 0, %1, c2, c0, 1\n\t"
		"isb\n\t"
		"dsb\n\t"
		"mcr p15, 0, %2, c13, c0, 1\n\t"
		"mcr p15, 0, %3, c3, c0, 1\n\t"
		"dsb\n\t"
		"isb\n\t"
		"mcr p15, 0, %0, c8, c7, 0\n\t"
		: : "r"(0), "r"(TTBR1), "r"(CONTEXTIDR), "r"(DACR)
	);

	/*
	 * TODO: improve this.
	 */
	ksceKernelCpuUnrestrictedMemcpy(PAYLOAD_PADDR, payload_addr, payload_size);
	ksceKernelCpuDcacheWritebackRange(PAYLOAD_PADDR, payload_size);
	ksceKernelCpuIcacheAndL2WritebackInvalidateRange(PAYLOAD_PADDR, payload_size);

	((void (*)(void))PAYLOAD_PADDR)();

	while (1)
		;
}

int module_start(SceSize argc, const void *args)
{
	ScePervasiveForDriver_EFD084D8(0); // Turn on clock
	ScePervasiveForDriver_A7CE7DCC(0); // Out of reset

	ksceUartInit(0);

	UART0_PRINTF("Baremetal sample by xerpi\n");

	SceSyscon_ksceSysconResetDevice_hook_uid = taiHookFunctionExportForKernel(KERNEL_PID,
		&SceSyscon_ksceSysconResetDevice_ref, "SceSyscon", 0x60A35F64,
		0x8A95D35C, ksceSysconResetDevice_hook_func);

	SceSyscon_SceSysconForDriver_4278E614_hook_uid = taiHookFunctionExportForKernel(KERNEL_PID,
		&SceSyscon_SceSysconForDriver_4278E614_ref, "SceSyscon", 0x60A35F64,
		0x4278E614, SceSysconForDriver_4278E614_hook_func);

	SceSyscon_ksceSysconSendCommand_hook_uid = taiHookFunctionExportForKernel(KERNEL_PID,
		&SceSyscon_ksceSysconSendCommand_ref, "SceSyscon", 0x60A35F64,
		0xE26488B9, ksceSysconSendCommand_hook_func);

	ScePower_scePowerRequestStandby_hook_uid = taiHookFunctionExportForKernel(KERNEL_PID,
		&ScePower_scePowerRequestStandby_ref, "ScePower", 0x5EAE6AEC,
		0x2B7C7CF4, scePowerRequestStandby_hook_func);

	ScePower_kscePowerRequestStandby_hook_uid = taiHookFunctionExportForKernel(KERNEL_PID,
		&ScePower_kscePowerRequestStandby_ref, "ScePower", 0x1590166F,
		0x2B7C7CF4, kscePowerRequestStandby_hook_func);

	SceLowio_ScePervasiveForDriver_788B6C61_hook_uid = taiHookFunctionExportForKernel(KERNEL_PID,
		&SceLowio_ScePervasiveForDriver_788B6C61_ref, "SceLowio", 0xE692C727,
		0x788B6C61, ScePervasiveForDriver_788B6C61_hook_func);

	SceLowio_ScePervasiveForDriver_81A155F1_hook_uid = taiHookFunctionExportForKernel(KERNEL_PID,
		&SceLowio_ScePervasiveForDriver_81A155F1_ref, "SceLowio", 0xE692C727,
		0x81A155F1, ScePervasiveForDriver_81A155F1_hook_func);

	UART0_PRINTF("Hooks done\n");

	memset(&suspend_ctx, 0, sizeof(suspend_ctx));
	suspend_ctx.size = sizeof(suspend_ctx);
	suspend_ctx.resume_func_vaddr = (unsigned int)&resume_func;
	asm volatile("mrc p15, 0, %0, c1, c0, 0\n\t" : "=r"(suspend_ctx.SCTLR));
	asm volatile("mrc p15, 0, %0, c1, c0, 1\n\t" : "=r"(suspend_ctx.ACTLR));
	asm volatile("mrc p15, 0, %0, c1, c0, 2\n\t" : "=r"(suspend_ctx.CPACR));
	asm volatile("mrc p15, 0, %0, c2, c0, 0\n\t" : "=r"(suspend_ctx.TTBR0));
	asm volatile("mrc p15, 0, %0, c2, c0, 1\n\t" : "=r"(suspend_ctx.TTBR1));
	asm volatile("mrc p15, 0, %0, c2, c0, 2\n\t" : "=r"(suspend_ctx.TTBCR));
	asm volatile("mrc p15, 0, %0, c3, c0, 0\n\t" : "=r"(suspend_ctx.DACR));
	asm volatile("mrc p15, 0, %0, c10, c2, 0\n\t" : "=r"(suspend_ctx.PRRR));
	asm volatile("mrc p15, 0, %0, c10, c2, 1\n\t" : "=r"(suspend_ctx.NMRR));
	asm volatile("mrc p15, 0, %0, c12, c0, 0\n\t" : "=r"(suspend_ctx.VBAR));
	asm volatile("mrc p15, 0, %0, c13, c0, 1\n\t" : "=r"(suspend_ctx.CONTEXTIDR));
	asm volatile("mrc p15, 0, %0, c13, c0, 2\n\t" : "=r"(suspend_ctx.TPIDRURW));
	asm volatile("mrc p15, 0, %0, c13, c0, 3\n\t" : "=r"(suspend_ctx.TPIDRURO));
	asm volatile("mrc p15, 0, %0, c13, c0, 4\n\t" : "=r"(suspend_ctx.TPIDRPRW));
	suspend_ctx.time = ksceKernelGetSystemTimeWide();

	ksceKernelCpuDcacheAndL2WritebackInvalidateRange(&suspend_ctx, sizeof(suspend_ctx));

	ksceKernelGetPaddr(&suspend_ctx, &suspend_ctx_paddr);

	UART0_PRINTF("Suspend context paddr: 0x%08X\n", suspend_ctx_paddr);

	sema = ksceKernelCreateSema("baremetal_sema", 0, 0, 4, NULL);

	UART0_PRINTF("Semaphore UID: 0x%08X\n", sema);

	map_scratchpad();

	UART0_PRINTF("Scratchpad mapped.\n");
	UART0_PRINTF("Payload paddr: 0x%08X\n", (unsigned int)PAYLOAD_PADDR);

	/*
	 * Copy the payload to the scratchpad.
	 */
	ksceKernelCpuUnrestrictedMemcpy(PAYLOAD_PADDR, payload_addr, payload_size);
	ksceKernelCpuDcacheWritebackRange(PAYLOAD_PADDR, payload_size);
	ksceKernelCpuIcacheAndL2WritebackInvalidateRange(PAYLOAD_PADDR, payload_size);

	kscePowerRequestStandby();

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
	return SCE_KERNEL_STOP_SUCCESS;
}

unsigned long get_cpu_id(void)
{
	unsigned long mpidr;

	asm volatile("mrc p15, 0, %0, c0, c0, 5\n" : "=r"(mpidr));

	return mpidr & 3;
}

unsigned long get_ttbr0(void)
{
	unsigned long ttbr0;

	asm volatile("mrc p15, 0, %0, c2, c0, 0\n" : "=r"(ttbr0));

	return ttbr0;
}

unsigned long get_ttbcr(void)
{
	unsigned long ttbcr;

	asm volatile("mrc p15, 0, %0, c2, c0, 2\n" : "=r"(ttbcr));

	return ttbcr;
}

unsigned long get_paddr(unsigned long vaddr)
{
	unsigned long paddr;

	ksceKernelGetPaddr((void *)vaddr, (uintptr_t *)&paddr);

	return paddr;
}

int find_paddr(unsigned long paddr, unsigned long vaddr, unsigned int size, unsigned int step, unsigned long *found_vaddr)
{
	unsigned long vaddr_end = vaddr + size;

	for (; vaddr < vaddr_end; vaddr += step) {
		unsigned long cur_paddr = get_paddr(vaddr);

		if ((cur_paddr & ~(step - 1)) == (paddr & ~(step - 1))) {
			if (found_vaddr)
				*found_vaddr = vaddr;
			return 1;
		}
	}

	return 0;
}

unsigned long page_table_entry(unsigned long paddr)
{
	unsigned long base_addr = paddr >> 12;
	unsigned long XN        = 0b0;   /* XN disabled */
	unsigned long C_B       = 0b10;  /* Outer and Inner Write-Through, no Write-Allocate */
	unsigned long AP_1_0    = 0b11;  /* Full access */
	unsigned long TEX_2_0   = 0b000; /* Outer and Inner Write-Through, no Write-Allocate */
	unsigned long AP_2      = 0b0;   /* Full access */
	unsigned long S         = 0b1;   /* Shareable */
	unsigned long nG        = 0b0;   /* Global translation */

	return  (base_addr << 12) |
		(nG        << 11) |
		(S         << 10) |
		(AP_2      <<  9) |
		(TEX_2_0   <<  6) |
		(AP_1_0    <<  4) |
		(C_B       <<  2) |
		(1         <<  1) |
		(XN        <<  0);
}

void map_scratchpad(void)
{
	int i;
	unsigned long ttbcr_n;
	unsigned long ttbr0_paddr;
	unsigned long ttbr0_vaddr;
	unsigned long first_page_table_paddr;
	unsigned long first_page_table_vaddr;
	unsigned long pt_entries[4];

	/*
	 * Identity-map the start of the scratchpad (PA 0x00000000-0x00003FFF) to
	 * the VA 0x00000000-0x00003FFF.
	 * To do such thing we will use the first 4 PTEs of the
	 * first page table of TTBR0 (which aren't used).
	 */

	ttbcr_n = get_ttbcr() & 7;
	ttbr0_paddr = get_ttbr0() & ~((1 << (14 - ttbcr_n)) - 1);
	find_paddr(ttbr0_paddr, 0, 0xFFFFFFFF, 0x1000, &ttbr0_vaddr);

	first_page_table_paddr = (*(unsigned int *)ttbr0_vaddr) & 0xFFFFFC00;
	find_paddr(first_page_table_paddr, 0, 0xFFFFFFFF, 0x1000, &first_page_table_vaddr);

	for (i = 0; i < 4; i++)
		pt_entries[i] = page_table_entry(i << 12);

	ksceKernelCpuUnrestrictedMemcpy((void *)first_page_table_vaddr, pt_entries, sizeof(pt_entries));
	ksceKernelCpuDcacheAndL2WritebackRange((void *)first_page_table_vaddr, sizeof(pt_entries));

	asm volatile(
		"dsb\n\t"
		"isb\n\t"
		/* Drain write buffer */
		"mcr p15, 0, %0, c7, c10, 4\n"
		/* Flush I,D TLBs */
		"mcr p15, 0, %0, c8, c7, 0\n"
		"mcr p15, 0, %0, c8, c6, 0\n"
		"mcr p15, 0, %0, c8, c5, 0\n"
		"mcr p15, 0, %0, c8, c3, 0\n"
		/* Instruction barrier */
		"mcr p15, 0, %0, c7, c5, 4\n"
		: : "r"(0));
}
