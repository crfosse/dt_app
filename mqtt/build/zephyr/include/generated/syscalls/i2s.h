
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef Z_INCLUDE_SYSCALLS_I2S_H
#define Z_INCLUDE_SYSCALLS_I2S_H


#ifndef _ASMLANGUAGE

#include <syscall_list.h>
#include <syscall.h>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int z_impl_i2s_configure(struct device * dev, enum i2s_dir dir, struct i2s_config * cfg);
static inline int i2s_configure(struct device * dev, enum i2s_dir dir, struct i2s_config * cfg)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&dir, *(uintptr_t *)&cfg, K_SYSCALL_I2S_CONFIGURE);
	}
#endif
	compiler_barrier();
	return z_impl_i2s_configure(dev, dir, cfg);
}


extern int z_impl_i2s_buf_read(struct device * dev, void * buf, size_t * size);
static inline int i2s_buf_read(struct device * dev, void * buf, size_t * size)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&buf, *(uintptr_t *)&size, K_SYSCALL_I2S_BUF_READ);
	}
#endif
	compiler_barrier();
	return z_impl_i2s_buf_read(dev, buf, size);
}


extern int z_impl_i2s_buf_write(struct device * dev, void * buf, size_t size);
static inline int i2s_buf_write(struct device * dev, void * buf, size_t size)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&buf, *(uintptr_t *)&size, K_SYSCALL_I2S_BUF_WRITE);
	}
#endif
	compiler_barrier();
	return z_impl_i2s_buf_write(dev, buf, size);
}


extern int z_impl_i2s_trigger(struct device * dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd);
static inline int i2s_trigger(struct device * dev, enum i2s_dir dir, enum i2s_trigger_cmd cmd)
{
#ifdef CONFIG_USERSPACE
	if (z_syscall_trap()) {
		return (int) arch_syscall_invoke3(*(uintptr_t *)&dev, *(uintptr_t *)&dir, *(uintptr_t *)&cmd, K_SYSCALL_I2S_TRIGGER);
	}
#endif
	compiler_barrier();
	return z_impl_i2s_trigger(dev, dir, cmd);
}


#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
