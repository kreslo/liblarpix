#ifndef SIMPLE_IOCTL_H
#define SIMPLE_IOCTL_H
#include <linux/ioctl.h>
  
#define READ_REG_IOCTL _IOR('s', 1, unsigned int *)
#define WRITE_REG_IOCTL _IOW('s', 2, unsigned int *)

#endif

