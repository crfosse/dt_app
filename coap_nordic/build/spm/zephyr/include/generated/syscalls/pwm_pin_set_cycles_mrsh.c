/* auto-generated by gen_syscalls.py, don't edit */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <syscalls/pwm.h>

extern int z_vrfy_pwm_pin_set_cycles(struct device * dev, u32_t pwm, u32_t period, u32_t pulse, pwm_flags_t flags);
uintptr_t z_mrsh_pwm_pin_set_cycles(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,
		uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, void *ssf)
{
	_current_cpu->syscall_frame = ssf;
	(void) arg5;	/* unused */
	int ret = z_vrfy_pwm_pin_set_cycles(*(struct device **)&arg0, *(u32_t*)&arg1, *(u32_t*)&arg2, *(u32_t*)&arg3, *(pwm_flags_t*)&arg4)
;
	return (uintptr_t) ret;
}

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
