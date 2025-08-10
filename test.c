#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <asm/set_memory.h>

static char hello_world[]="Hello World\n";

static dev_t hello_dev_number;
static struct cdev *driver_object;
static struct class *hello_class;
static struct device *hello_dev;
int errorType;

void *virtAddr;
size_t size = PAGE_SIZE;
dma_addr_t physAddr;
int flag = GFP_KERNEL | GFP_ATOMIC | GFP_DMA;
    struct page *page;
 struct sg_table *table;

static ssize_t driver_read( struct file *instance, char __user *user, size_t count, loff_t *offset )
{
    unsigned long not_copied, to_copy;

printk("driver_read\n");
    not_copied=copy_to_user(user,virtAddr,10);
    return 10;
}

static ssize_t driver_write( struct file *instance, const char  *user, size_t count, loff_t *offset )
{
    unsigned long not_copied, to_copy;
printk("driver_write\n");
    not_copied=copy_from_user(virtAddr,user, 10);
    return 10;
}



static int my_mmap(struct file *filp, struct vm_area_struct *vma) {
    size_t max_len = PAGE_SIZE; // length of coherent memory
    unsigned long len;
    unsigned long pgoff;
    int ret;

    /*
     * Check the requested size of the region is within range
     */
    len = vma->vm_end - vma->vm_start;
    if (len > max_len)
        return -EINVAL;

    /*
     * We need to temporarily clear vm_pgoff for dma_mmap_coherent()
     */
    pgoff = vma->vm_pgoff;
    vma->vm_pgoff = 0;
    //set_memory_uc((unsigned long)virtAddr, 1);
    //vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#if 1
    ret = dma_mmap_coherent(hello_dev, vma, virtAddr, physAddr, PAGE_SIZE);
#else
    ret = remap_pfn_range ( vma
                    , vma->vm_start
                    , virt_to_phys(virtAddr)>>PAGE_SHIFT
                    ,PAGE_SIZE
                    , vma->vm_page_prot
                    );
#endif
    vma->vm_pgoff = pgoff;

    return ret;
}

static struct file_operations fops = {
    .owner= THIS_MODULE,
    .read= driver_read,
    .write= driver_write,
    .mmap = my_mmap,
};

static int __init mod_init( void )
{
    int debug;
    u64 dma_mask=0xffffffff;
        int err;

    printk("starting insertion");
    if (alloc_chrdev_region(&hello_dev_number,0,1,"Hello")<0)
        return -EIO;
    driver_object = cdev_alloc();
    if (driver_object==NULL){
        errorType = EIO;
        goto free_device_number;
    }
    driver_object->owner = THIS_MODULE;
    driver_object->ops = &fops;
    if (cdev_add(driver_object,hello_dev_number,1)){
        errorType=EIO;
        goto free_cdev;
    }
    hello_class = class_create( THIS_MODULE, "Hello" );
    if (IS_ERR( hello_class )) {
        pr_err( "hello: no udev support\n");
        errorType=EIO;
        goto free_cdev;
    }
    hello_dev = device_create( hello_class, NULL, hello_dev_number, NULL, "%s", "hello" );
    if (IS_ERR( hello_dev )) {
        pr_err( "hello: device_create failed\n");
        errorType=EIO;
        goto free_class;
    }
    hello_dev->dma_mask = &dma_mask;
    debug = dma_set_mask_and_coherent(hello_dev, DMA_BIT_MASK(32));
    printk("dma mask returns: %d", debug);
#if 1
    virtAddr = dma_alloc_coherent(hello_dev, size, &physAddr, flag);
    printk("I'm actually alive");
    if(virtAddr==NULL){
        printk("virtual address null, dma failed!\n");
        errorType=ENOMEM;
        goto free_dev;
    } else {
        printk("phys =%pad va =%p\n",&physAddr,virtAddr);
        printk("virt_addr_valid %d\n",virt_addr_valid(virtAddr));
        memset(virtAddr, 0x55, PAGE_SIZE);
        printk("success\n");
    }
#else
    page = alloc_page(GFP_KERNEL | GFP_ATOMIC);
        if (!page) {
                return -1;
        }
    virtAddr = page_to_virt(page);
    physAddr = page_to_phys(page);
    if(virtAddr==NULL){
        printk("virtual address null, dma failed!\n");
        errorType=ENOMEM;
        goto free_dev;
    } else {
        printk("phys =%pad va =%p\n",&physAddr,virtAddr);
        printk("virt_addr_valid %d\n",virt_addr_valid(virtAddr));
        memset(virtAddr, 0x55, PAGE_SIZE);
        printk("success\n");
    }
#endif
#if 0
        printk("exporter_map_dma_buf %p\n",page);
        table = kmalloc(sizeof(*table), GFP_KERNEL);
        if (!table)
                return ERR_PTR(-ENOMEM);

        err = sg_alloc_table(table, 1, GFP_KERNEL);
        if (err) {
                kfree(table);
                printk("sg_alloc_table error\n");
                return ERR_PTR(err);
        }

        printk("calling sg_set_page\n");

        sg_set_page(table->sgl, page, PAGE_SIZE, 0);

#endif
    return 0;
free_dev:
    device_destroy( hello_class, hello_dev_number );
free_class:
    class_destroy( hello_class );
free_cdev:
    kobject_put( &driver_object->kobj );
free_device_number:
    unregister_chrdev_region( hello_dev_number, 1 );
    return -errorType;
}

static void __exit mod_exit( void )
{
if(virtAddr)
#if 1
    dma_free_coherent(hello_dev, size, virtAddr, physAddr);
#else
        if(page)
        __free_page(page);

#endif
    device_destroy( hello_class, hello_dev_number );
    class_destroy( hello_class );
    cdev_del( driver_object );
    unregister_chrdev_region( hello_dev_number, 1 );
    return;
}

module_init( mod_init );
module_exit( mod_exit );

MODULE_AUTHOR("ME");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Testing DMA.");