/*
 * backtrace.c
 *
 *  Created on: 3 Jun 2021
 *      Author: Grant
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__ASSEMBLER__)
# define   U(_x)	(_x)
#else
# define  U(_x)	(_x##U)
#endif

#define UNWIND_LIMIT	20U

#define MODE_EL_SHIFT		U(0x2)
#define MODE_EL_MASK		U(0x3)

#define CONSOLE_FLAG_BOOT		(U(1) << 0)
#define GET_EL(mode)		(((mode) >> MODE_EL_SHIFT) & MODE_EL_MASK)

#define CURRENT_EL_MASK   0x3
#define CURRENT_EL_SHIFT  2

//#include <arch_helpers.h>
//#include <common/debug.h>
//#include <drivers/console.h>

typedef unsigned long u_register_t;

struct frame_record {
	/* Previous frame record in the list */
	struct frame_record *parent;
	/* Return address of the function at this level */
	uintptr_t return_addr;
};

typedef struct console {
	struct console *next;
	/*
	 * Only the low 32 bits are used. The type is u_register_t to align the
	 * fields of the struct to 64 bits in AArch64 and 32 bits in AArch32
	 */
	u_register_t flags;
	int (*const putc)(int character, struct console *console);
	int (*const getc)(struct console *console);
	void (*const flush)(struct console *console);
	uintptr_t base;
	/* Additional private driver data may follow here. */
} console_t;

console_t *console_list;
uint8_t console_state = CONSOLE_FLAG_BOOT;

void console_flush(void)
{
	console_t *console;

	for (console = console_list; console != NULL; console = console->next)
	{
			if ((console->flags & console_state) && (console->flush != NULL)) {
				console->flush(console);
			}
	}
}

static bool is_valid_object(uintptr_t addr, size_t size)
{
	assert(size > 0U);

	if (addr == 0U)
		return false;

	/* Detect overflows */
	if ((addr + size) < addr)
		return false;

	/* A pointer not aligned properly could trigger an alignment fault. */
	if ((addr & (sizeof(uintptr_t) - 1U)) != 0U)
		return false;

	return true;
}

static bool is_valid_jump_address(uintptr_t addr)
{
	if (addr == 0U)
		return false;

	/* Check alignment. Both A64 and A32 use 32-bit opcodes */
	//if ((addr & (sizeof(uint32_t) - 1U)) != 0U)
	if ((addr & (sizeof(uint8_t) - 1U)) != 0U)
		return false;

	return true;
}

static void unwind_stack(struct frame_record *fr, uintptr_t current_pc, uintptr_t link_register)
{
	uintptr_t call_site;
	static const char *backtrace_str = "%u: 0x%lx\n";
	//get_current_el();
	//const char *el_str = get_current_el();

	if (!is_valid_object((uintptr_t)fr, sizeof(struct frame_record))) {
			printf("ERROR: Corrupted frame pointer (frame record address = %p)\n",
			       fr);
			return;
		}

		if (fr->return_addr != link_register) {
			printf("ERROR: Corrupted stack (frame record address = %p)\n",
			       fr);
			return;
		}

	printf(backtrace_str, 0U, current_pc);

	for (unsigned int i = 1U; i < UNWIND_LIMIT; i++) {
			/* If an invalid frame record is found, exit. */
			if (!is_valid_object((uintptr_t)fr, sizeof(struct frame_record)))
			{
				printf("JUMP1\n");
				return;
			}

			call_site = fr->return_addr - 4U;

#if ENABLE_PAUTH
			xpaci(call_site);
#endif

			if (!is_valid_jump_address(call_site))
			{
				printf("JUMP2\n");
				return;
			}

			printf(backtrace_str, i, call_site);

			fr = fr->parent;

	}

}

void backtrace()
{
	uintptr_t return_address = (uintptr_t)__builtin_return_address(0U);
	struct frame_record *fr = __builtin_frame_address(0U);

	console_flush();

	unwind_stack(fr, (uintptr_t)&backtrace, return_address);
}

void call_function(int a, int b, int c)
{
	backtrace();
}

int main()
{
	call_function(1, 3, 2);

	printf("HERE");

	return 0;
}
