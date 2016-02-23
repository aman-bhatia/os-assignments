	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register, which should be caller saved.
 */
	.globl thread_switch
thread_switch:

	/* push registers */

	pushal

	/* save stack pointer */
	movl current_thread , %eax
	movl %esp , (%eax)


	/* load next_thread stack pointer */
	movl next_thread , %eax
	movl (%eax) , %esp

	/* set next_thread to be the current_thread */
	movl %eax , current_thread

	/* set next_thread to 0 */
	movl $0, next_thread


	/* pop next threads registers */
	popal

	ret				/* pop return address from stack */

