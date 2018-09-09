/*
 * uart64 Character Driver Example
 *
 * The purpose of this driver is to provide an example of a character driver that
 * could be used for custom IP easily. It is a platform device driver that pulls 
 * configuration information from the device tree.  
 *
 * It illustrates the basic character device functionality (open, read, write and close)
 * and adds ioctl also. It creates a device node /dev/uart64 and allows the underlying
 * PS TTC to be exercised by readng and writing to it's registers.
 *
 * The following commands may be used from the command line to exercise it.
 *
 * 'cat /dev/uart64' 				dump the registers of the device.
 * 'echo "<offset> <value>" > /dev/uart64' 	write to a register
 *						offset = 0xY (a hex value, 1 - 8 digits) 
 *						value = 0xY (a hex value, 1 - 8 digits)
 *
 * The offset is from the start of the TTC. The linux kernel uses the 1st TTC so that
 * this driver should be using the 2nd TTC in the PS.
 * 
 * The overflow interupt is enabled by default so that the TTC only has to be enabled
 * to cause an interrupt. The interrupt is serviced by the driver with console output.
 *
 * 'echo "0xC 0x20" > /dev/uart64
 * 
 * It also illustrates the use of ioctl to read and write to the interrupt register.
 * The ioctl_test.c application is compiled and will read the interrupt register and then
 * write it to disable the interrupt capability.
 *
 * #include <stdio.h>
 * #include <fcntl.h>
 * #include <string.h>
 *
 * #include "uart64_ioctl.h"
 *
 * int main()
 * {
 *    int fd;
 *    unsigned int reg;
 *    unsigned int disable = 0;
 *
 *    fd = open("/dev/uart64", O_RDWR);
 *  
 *    ioctl(fd, READ_REG_IOCTL, &reg);
 *    printf("ioctl read executed, reg = 0x%08X\n", reg);
 *
 *    ioctl(fd, WRITE_REG_IOCTL, &disable);
 *
 *    ioctl(fd, READ_REG_IOCTL, &reg);
 *    printf("ioctl read executed, reg = 0x%08X\n", reg);
 *
 *    close(fd);
 *    return 0;
 * }
 *
 * uart64_ioctl.h file contents
 * 
 * #ifndef SIMPLE_IOCTL_H
 * #define SIMPLE_IOCTL_H
 * #include <linux/ioctl.h>
 * 
 * #define READ_REG_IOCTL _IOR('s', 1, unsigned int *)
 * #define WRITE_REG_IOCTL _IOW('s', 2, unsigned int *)
 *
 * #endif
 *
 * The driver also illustrates the use of a file attribute in the sys filesystem as an
 * alternative to ioctl. The file attribute /sys/devices/amba.0/f8002000/irqreg is created
 * and allows the interrupt register to be read and written.
 *
 * 'cat /sys/devices/amba.0/f8002000.ps7-ttc/irqreg'		displays the interupt register
 * 'echo 0x0 > /sys/devices/amba.0/f8002000.ps7-ttc/irqreg'  	disables the interrupt
 * 'echo 0x10 > /sys/devices/amba.0/f8002000.ps7-ttc/irqreg' 	enables the overflow interrupt
 *
 * Assuming dynamic debug is turned on in the kernel, extra debug information will be ouput when
 * the debug level is turned on with 'echo 8 > /proc/sys/kernel/printk' and this file has debug
 * turned on with 'echo file uart64.c line 1-1000 +p > /sys/kernel/debug/dynamic_debug/control'
 * after the module has been inserted. 
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h> 

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/mutex.h>

#include "psttc-ioctl.h"

MODULE_LICENSE("GPL");

#define DRIVER_NAME "uart64"

#define uart64_readreg(offset)         __raw_readl(offset)
#define uart64_writereg(val, offset)   __raw_writel(val, offset)

#define IRQ_ACK_REG		0x54
#define IRQ_CONTROL_REG		0x60


#define TX_TIMEOUT_JF 100

#define LARPIX_BUFFER_SIZE 1024*8

/* The following illustrates the use of a module parameter which is passed into
 * the module when it is inserted. Add module_input=X where X is an integer to 
 * the command line when the module is inserted to use the input parameter. 
 */
//unsigned module_input = 0;
//module_param(module_input, int, S_IRUGO);

struct uart64_local {
	void __iomem *base_addr;

	unsigned long larpix_recv;
	unsigned long larpix_send;
	unsigned long larpix_starts;
	unsigned long larpix_stops;
	unsigned long larpix_irq;

	unsigned char *rx_data;
        int bytes_in_rx_data; 
	unsigned char *tx_data;

	dev_t dev_node;
	struct cdev cdev;
	struct device *dev;
        struct mutex uart64_mutex;
};

int nodes_created=0;

/*******************************************************************************
 * Character Device Functionality
 */

/*
 * uart64_open
 * 
 * Open the device so that it's ready to use.
 */
static int uart64_local_open(struct inode *ino, struct file *file)
{
	struct uart64_local *lp;

	/* all char file functions have the file as input parameter. The file 
 	 * data allows driver private data to be saved in it, so make it easier for
 	 * all functions to get to the driver data which will be in the private data
	 *
	 * the inode data is not passed to all char file functions but has a pointer
	 * to the char device data of the driver data in it, container_of() is used to
	 * get a pointer to the driver data from a pointer to the character device 
	 * data contained within it
 	 */  

	file->private_data = container_of(ino->i_cdev, struct uart64_local, cdev);

	lp = (struct uart64_local *)file->private_data;

	dev_dbg(lp->dev, "opened\n");
	return 0;
}

/*
 * uart64_release
 * 
 * Release the device.
 */
static int uart64_release(struct inode *ino, struct file *file)
{
	struct uart64_local *lp = (struct uart64_local *)file->private_data;

	dev_dbg(lp->dev, "released\n");

	return 0;
}



/* 
 * uart64_write
 *
 * Write to a user specified register with a format of "offset value" where offset is a decimal
 * number (0-6) and value is the value to write in the format 0xXXXXXXXX with only one X 
 * being required.
 */
static ssize_t uart64_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
        long unsigned int to_jiffies;
        long unsigned int cur_jiffies;
 	int i;
	volatile uint32_t word[2];
        uint32_t* dbuf;
 
	struct uart64_local *lp = file->private_data; 
	int rc;

//	dev_info(lp->dev,"write\n");

	/* Step 11, copy the data from userspace into a local buffer so that it can be 
	 * used from kernel space
	 */
        rc=count;

	if (copy_from_user(&(lp->tx_data[0]), buf, count) != 0)
	{
		rc = -EFAULT;
		goto out;
	}
	
        dbuf=(uint32_t*)(&(lp->tx_data[0]));

 // now perform actual sendout
        for(i=0; i<count/8; i++) 
           {
           word[0]=dbuf[i*2];  
           word[1]=dbuf[i*2+1] & 0x7FFFFFFF; //reset TX ready bit
      
      cur_jiffies=jiffies;
      to_jiffies=cur_jiffies+TX_TIMEOUT_JF;
           while( ioread8(lp->base_addr+lp->larpix_send+7)<0x80 && cur_jiffies<=to_jiffies) {cur_jiffies=jiffies;} //wait until TX is ready to send
           if( cur_jiffies > to_jiffies) { dev_info(lp->dev,"timeout waiting for TX ready! \n"); goto out;}   
           iowrite32(word[0],(void*)(lp->base_addr+lp->larpix_send));
           iowrite32(word[1],(void*)(lp->base_addr+lp->larpix_send+4));
         }

out:
	return rc;
}

/*
 * uart64_ioctl
 * 
 * Respond to an ioctl call by reading the interrupt register and then providing
 * it's contents or by writing the specified value to the register.
 */
static long uart64_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	//u32 reg;
	struct uart64_local *lp = file->private_data; 
	//int rc=0;

	dev_info(lp->dev,"ioctl\n");

	switch (cmd) {
	case READ_REG_IOCTL:
//		reg = uart64_readreg((lp->base_addr) + IRQ_CONTROL_REG);

		/* copy the value of the register to the user
		 * space buffer for the user space process to use
		 */  

//		rc = copy_to_user((u32 *)arg, &reg, sizeof(u32));

//		if (rc)
//			return -EACCES;
		break;
	case WRITE_REG_IOCTL:

		/* copy the value from user space so that
		 * it can be written to the device register
		 */ 

	//	rc = copy_from_user(&reg, (u32 *)arg, sizeof(u32));

//		if (rc)
//			return -EACCES;
//		uart64_writereg(reg, (lp->base_addr) + IRQ_CONTROL_REG);
		break;
	default:
		return -EINVAL;
	}
 
	return 0;
}





/*
 * uart64_read
 * 
 * Respond to a read of the device by reading all the devices registers and then 
 * converting them to ascii. Copy them back to the user to be displayed.
 */
static ssize_t uart64_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	struct uart64_local *lp = file->private_data; 
	int rc;

 	
	//dev_info(lp->dev,"read() called. count=%d, bytes_in_rx_data=%d\n",count,lp->bytes_in_rx_data);

	if (count > LARPIX_BUFFER_SIZE)
		count = LARPIX_BUFFER_SIZE ;
	//dev_info(lp->dev,"count set to %d\n",count);
	
//        if(lp->bytes_in_rx_data>0)        dev_info(lp->dev,"copying %d bytes to the user space\n",lp->bytes_in_rx_data);
//	if(mutex_lock_killable(&lp->uart64_mutex)) 		return -EINTR;
	if (copy_to_user(buf, lp->rx_data, lp->bytes_in_rx_data) != 0)
	{
		rc = -EFAULT;
		goto out;
	}
	
	rc = lp->bytes_in_rx_data;
        lp->bytes_in_rx_data=0;
	
out:
//	mutex_unlock(&lp->uart64_mutex);
	return rc;

	
}

/*
static ssize_t uart64_read_old(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	struct uart64_local *lp = file->private_data; 
	int rc;
       long unsigned int to_jiffies;
        long unsigned int cur_jiffies;
	int bytes_received;
        uint32_t w32l, w32h; 
	
	dev_info(lp->dev,"read() called. count=%d\n",count);

	if (count > LARPIX_BUFFER_SIZE)
		count = LARPIX_BUFFER_SIZE ;
	dev_info(lp->dev,"count set to %d\n",count);
	
      bytes_received=0;
      cur_jiffies=jiffies;
      to_jiffies=cur_jiffies+RX_TIMEOUT_JF;
      while(bytes_received < count && cur_jiffies<=to_jiffies) {
	 cur_jiffies=jiffies;
         w32h=ioread32((void*)(lp->base_addr+lp->larpix_recv+4));
      //   dev_info(lp->dev,"w32  %0x%x\n",w32h);
	    *((uint32_t*)&(lp->rx_data[bytes_received+4]))  = w32h;
              
//	    if (lp->rx_data[bytes_received+7] > 0x80 ) 
	    if (w32h > 0x80000000 ) 
            { //word received
//                dev_info(lp->dev,"word received 0x%08x",w32);

               w32l  = ioread32((void*)(lp->base_addr+lp->larpix_recv)); //copy lower part
               *((uint32_t*)&(lp->rx_data[bytes_received])) = w32l;
//               dev_info(lp->dev," %08x\n",w32);
                dev_info(lp->dev,"word received 0x%08x%08x",w32h,w32l);

		iowrite8(0x0, (void*)(lp->base_addr+lp->larpix_recv+7) );  //reset data_ready bit
                bytes_received=bytes_received+8; 
            }
      } 
     
 
if (cur_jiffies > to_jiffies) dev_info(lp->dev,"receive timeout.\n");
//done:
//	 printk(KERN_INFO "larpix: copying %d bytes to the user space\n",i*8);
                dev_info(lp->dev,"copying %d bytes to the user space\n",bytes_received);
	if (copy_to_user(buf, lp->rx_data, bytes_received) != 0)
	{
		rc = -EFAULT;
		goto out;
	}
	
	rc = bytes_received;
	
out:
//	mutex_unlock(&dev->larpix_mutex);
	return rc;

	
}


*/


static struct file_operations uart64_fops = {
	.owner    = THIS_MODULE,
	.write    = uart64_write,
	.read	  = uart64_read,
	.open     = uart64_local_open,
	.release  = uart64_release,
	.unlocked_ioctl = uart64_ioctl
};
static struct class *uart64_class=NULL;

/*
 * uart64_cdev_init 
 * 
 * Initialize the driver to be a character device such that is responds to 
 * file operations.
 */
static int uart64_cdevice_init(struct cdev *cdev, 
				struct device *dev,
				dev_t *dev_node)
{
	int rc;
	struct device *subdev;
        char str[128];
	dev_info(dev,"uart64_cdevice_init called for node %d.\n", nodes_created);

	/* Step 6, allocate a character device from the kernel for this 
	 * driver
	 */ 
        sprintf(str,"uart64gpio%d",nodes_created);
	rc = alloc_chrdev_region(dev_node, 0, 1, str);

	if (rc) {
		dev_err(dev, "unable to get a char device number\n");
		return rc;
	}

	/* Step 7, initialize the character device data structure before
	 * registering the character device with the kernel
	 */ 

	cdev_init(cdev, &uart64_fops);
	cdev->owner = THIS_MODULE;
	rc = cdev_add(cdev, *dev_node, 1);

	if (rc) {
		dev_err(dev, "unable to add char device\n");
		goto init_error1;
	}

	/* Step 8, create the device in sysfs which will allow the device node 
	 * in /dev to be created 
	 */

	if(uart64_class==NULL) uart64_class = class_create(THIS_MODULE, DRIVER_NAME);

	if (IS_ERR(uart64_class)) {
		dev_err(dev, "unable to create class\n");
		goto init_error2;
	}	

	/* Step 9, create the device node in /dev so the device is accessible 
	 * as a character device
	 */
	
        sprintf(str,"uart64%d",nodes_created);

	subdev = device_create(uart64_class, dev, *dev_node, NULL, str);

	if (IS_ERR(subdev)) {
		dev_err(dev, "unable to create the device %s\n", str);
		goto init_error3;
	}
         
	return 0;

init_error3:
	class_destroy(uart64_class);

init_error2:
	cdev_del(cdev);

init_error1:
	unregister_chrdev_region(*dev_node, 1);
	return rc;
}

/*
 * uart64_cdevice_exit 
 * 
 * Exit the character device by freeing up the resources that it created and 
 * disconnecting itself from the kernel.
 */
static void uart64_cdevice_exit(struct cdev *cdev, 
		 	    	struct device *dev,
			    	dev_t *dev_node)
{
	/* Step 12, take everything down in the reverse order
	 * from how it was created for the char device
	 */

	device_destroy(uart64_class, *dev_node);
//	class_destroy(uart64_class);

	cdev_del(cdev);
	unregister_chrdev_region(*dev_node, 1);
}


/*******************************************************************************
 * Interrupt Functionality
 */

/* 
 * uart64_irq
 *
 * Respond to the interrupt by clearing the interrupt in the device.
 */
static irqreturn_t uart64_irq(int irq, void *p)
{
        uint32_t w32l, w32h; 
	struct uart64_local *lp = (struct uart64_local *)p;
  if(lp->bytes_in_rx_data <= LARPIX_BUFFER_SIZE-8)
  {
        
   //     iowrite8(0x0, (void*)(lp->base_addr+0x20) );  //reset irq bit (1), reset IRQ_ENA (0)
	//dev_info(lp->dev, "uart64 interrupt\n");

	
         w32h  = ioread32((void*)(lp->base_addr+lp->larpix_recv+4));
         w32l  = ioread32((void*)(lp->base_addr+lp->larpix_recv)); //copy lower part
        //        dev_info(lp->dev,"word received 0x%08x%08x",w32h,w32l);
              
	    if (w32h > 0x80000000 ) 
            { //word received
//                dev_info(lp->dev,"word received 0x%08x",w32);
//	       if(mutex_lock_killable(&lp->uart64_mutex)) 		return -EINTR;
	       *((uint32_t*)&(lp->rx_data[lp->bytes_in_rx_data+4]))  = w32h;
               *((uint32_t*)&(lp->rx_data[lp->bytes_in_rx_data])) = w32l;
                lp->bytes_in_rx_data=lp->bytes_in_rx_data+8;
//	       mutex_unlock(&lp->uart64_mutex);

		iowrite8(0x0, (void*)(lp->base_addr+lp->larpix_recv+7) );  //reset data_ready bit
       //          dev_info(lp->dev,"bytes_in_rx_data=%d",lp->bytes_in_rx_data);
           }
            else dev_err(lp->dev,"uart interrupt called, but RX data ready bit is not present! Skipping.");
     
  }
        iowrite8(0x0, (void*)(lp->base_addr+lp->larpix_recv+7) );  //reset data_ready bit	
        iowrite8(0x1, (void*)(lp->base_addr+0x20) );  // reset irq bit (1), set IRQ_ENA  (0)
	return IRQ_HANDLED;
}


/*******************************************************************************
 * Platform Driver Functionality
 */

/*
 * uart64_ioctl 
 * 
 * Respond to an ioctl call by reading the interrupt register and then providing
 * it's contents or by writing the specified value to the register.
 */
static int uart64_probe(struct platform_device *pdev)
{
	int irq;
	struct resource *res; 
	struct device *dev = &pdev->dev;
	struct uart64_local *lp = NULL;
	const char *compatible;
	int rc = 0;
        const void *ptr;
	
	dev_info(dev, "Device Tree Probing\n");
//	dev_info(dev, "Module input parameter = %d\n", module_input);

	/* Step 1, allocate the memory for the driver data, zeroing it so it's
	 * initialized and ready to use
	 */ 

	lp = (struct uart64_local *)devm_kzalloc(dev, sizeof(struct uart64_local), GFP_KERNEL);

	if (!lp) {
		dev_err(dev, "Cound not allocate memory for uart64 device\n");
		return -ENOMEM;
	}

	/* Step 2, connect the device to the driver data and the driver data to the device
	 * so that the all data is easily accessible
	 */	 
	
	dev_set_drvdata(dev, lp);
	lp->dev = dev;

	/* Step 3, setup the memory to allow the device to be accessed, this includes
	 * getting the memory info from the device tree and mapping the physical memory 
	 * into the virtual address space 
	 */

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lp->base_addr = devm_ioremap_resource(dev, res);

	if (IS_ERR(lp->base_addr)) {
		dev_err(dev, "Could not map physical memory to virtual\n");
		return PTR_ERR(lp->base_addr);
	}
	
	/* Step 4, setup the interrupt for the device, this includes getting the
	 * interrupt data from the device tree and then requesting that 
	 * interrupt from the kernel
	 */

	//irq = platform_get_irq(pdev, 0);
        irq = irq_of_parse_and_map(dev->of_node, 0);
        	dev_info(dev, "IRQ %d obtained from irq_of_parse_and_map()",irq);
	rc = devm_request_irq(dev, irq, &uart64_irq, 0, DRIVER_NAME, lp);
    
	if (rc) {
		dev_err(dev, "Could not allocate interrupt\n");
		return rc; 
	}

	/* for an example only, read the compatible string from the device
 	 * tree just to illustrate some other OF functionality
 	 */

 	rc = of_property_read_string(dev->of_node, "compatible", &compatible);

 	if (!rc) {
 		char temp[128];
 		sprintf(temp, "device tree compatible property = %s", compatible);  
 		dev_info(dev, temp);
 	}


 	ptr =of_get_property(dev->of_node,"larpix,recv",NULL);
        if(!ptr) dev_err(dev, "Device tree entry larpix,recv not found\n"); 
           else  lp->larpix_recv = be32_to_cpup(ptr);

 	if (ptr) {
  		dev_info(dev, "larpix,recv = %ld",lp->larpix_recv);
 	}


 	ptr =of_get_property(dev->of_node,"larpix,send",NULL);
        if(!ptr) dev_err(dev, "Device tree entry larpix,send not found\n"); 
           else  lp->larpix_send = be32_to_cpup(ptr);
 	if (ptr) {
 		dev_info(dev, "larpix,send = %ld",lp->larpix_send);
 	}

 	ptr =of_get_property(dev->of_node,"larpix,starts",NULL);
        if(!ptr) dev_err(dev, "Device tree entry larpix,starts not found\n"); 
           else  lp->larpix_starts = be32_to_cpup(ptr);
 	if (ptr) {
 		dev_info(dev, "larpix,starts = %ld",lp->larpix_starts);
 	}

 	ptr =of_get_property(dev->of_node,"larpix,stops",NULL);
        if(!ptr) dev_err(dev, "Device tree entry larpix,stops not found\n"); 
           else  lp->larpix_stops = be32_to_cpup(ptr);
 	if (ptr) {
 		dev_info(dev, "larpix,stops = %ld",lp->larpix_stops);
 	}

 	ptr =of_get_property(dev->of_node,"larpix,irq",NULL);
        if(!ptr) dev_err(dev, "Device tree entry larpix,irq not found\n"); 
           else  lp->larpix_irq = be32_to_cpup(ptr);
 	if (ptr) {
 		dev_info(dev, "larpix,irq = %ld",lp->larpix_irq);
 	}



	/* Initialize the device now that it is accessible, enable the overflow
	 * interrupt by default
 	 */
	//IK uart64_writereg(0x10, lp->base_addr + IRQ_CONTROL_REG);

	/* Step 5, create the interrupt register device attribute in sysfs so that the
 	 * register can be accessed easily
	 */
/*
	rc = device_create_file(dev, &dev_attr_irqreg);

	if (rc) {
		dev_err(dev, "Count not create sysfs attribute file\n");
		return rc;
	}
*/
	/* Initalize the device to make it a character device so that it 
	 * can be accessed using char mode read/write functions
	 */ 
	rc = uart64_cdevice_init(&lp->cdev, dev, &lp->dev_node);
	if (rc) { 
	//	device_remove_file(&pdev->dev, &dev_attr_irqreg);
		return rc;
	}

		lp->rx_data = (unsigned char*)kzalloc(LARPIX_BUFFER_SIZE, GFP_KERNEL);
		if (lp->rx_data == NULL)
		{
			dev_err(dev, "[target] open(): out of memory\n");
			return -ENOMEM;
		}
                lp->bytes_in_rx_data=0;

		lp->tx_data = (unsigned char*)kzalloc(LARPIX_BUFFER_SIZE, GFP_KERNEL);
		if (lp->tx_data == NULL)
		{
			dev_err(dev, "[target] open(): out of memory\n");
			return -ENOMEM;
		}


		
	dev_info(dev,"uart64 at 0x%08x mapped to 0x%08x, irq=%d\n",
		(unsigned int __force)res->start,
		(unsigned int __force)lp->base_addr,
		irq);

        nodes_created++;
        mutex_init(&lp->uart64_mutex);

	return 0;
}

/*
 * uart64_remove
 *
 * Remove the driver by freeing up all resources and disconnecting
 */
static int uart64_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct uart64_local *lp = dev_get_drvdata(dev);
	dev_info(dev, "Device Tree Remove called for node %d\n",lp->dev_node);
	kfree(lp->rx_data);
        lp->bytes_in_rx_data=0;
	kfree(lp->tx_data);
        mutex_destroy(&lp->uart64_mutex);

	uart64_cdevice_exit(&lp->cdev, dev, &lp->dev_node);
//	device_remove_file(dev, &dev_attr_irqreg);
	
	dev_set_drvdata(dev, NULL);
        nodes_created--;

        if(nodes_created==0) class_destroy(uart64_class);

	return 0;
}

static struct of_device_id uart64_of_match[] = {
	{ .compatible = "larpix,uart64", },
//	{ .compatible = "xlnx,uart64", },
	{ /* end of list */ },
};
MODULE_DEVICE_TABLE(of, uart64_of_match);

static struct platform_driver uart64_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table	= uart64_of_match,
	},
	.probe		= uart64_probe,
	.remove		= uart64_remove,
};

/*******************************************************************************
 * Module Functionality
 */

/*
 * uart64_init - initialized the module by connecting the driver to the kernel 
 */
static int __init uart64_init(void)
{
	printk(KERN_ALERT "uart64 module initialized\n");
	return platform_driver_register(&uart64_driver);
}


/*
 * uart64_exit - exit the module by disconnecting the driver from the kernel
 */
static void __exit uart64_exit(void)
{
	platform_driver_unregister(&uart64_driver);
	printk(KERN_ALERT "uart64 module exited\n");
}

module_init(uart64_init);
module_exit(uart64_exit);



