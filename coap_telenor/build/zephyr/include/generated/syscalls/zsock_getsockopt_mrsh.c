/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/socket.h>

extern int z_vrfy_zsock_getsockopt(int sock, int level, int optname, void * optval, socklen_t * optlen);
uintptr_t z_mrsh_zsock_getsockopt(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
		uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg5;	/* unused */
	int ret = z_vrfy_zsock_getsockopt(*(int*)&arg0, *(int*)&arg1, *(int*)&arg2, *(void **)&arg3, *(socklen_t **)&arg4)
;
	return (uintptr_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
