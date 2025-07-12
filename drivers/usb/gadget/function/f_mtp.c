<<<<<<< HEAD
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
=======
/*
 * Gadget Function Driver for MTP
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
>>>>>>> target/16.0
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
<<<<<<< HEAD
#include <linux/printk.h>

=======

#include <linux/seq_file.h>
#include <linux/debugfs.h>
>>>>>>> target/16.0
#include <linux/types.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
<<<<<<< HEAD
=======
#include <linux/ipc_logging.h>
>>>>>>> target/16.0

#include <linux/usb.h>
#include <linux/usb_usual.h>
#include <linux/usb/ch9.h>
<<<<<<< HEAD
#include "f_mtp.h"
=======
#include <linux/usb/f_mtp.h>
>>>>>>> target/16.0
#include <linux/configfs.h>
#include <linux/usb/composite.h>

#include "configfs.h"
<<<<<<< HEAD
#include "usb_boost.h"


=======

#define NUM_PAGES	10 /* # of pages for ipc logging */

#ifdef CONFIG_DYNAMIC_DEBUG
#define mtp_log(fmt, ...) do { \
	ipc_log_string(_mtp_ipc_log, "%s: " fmt,  __func__, ##__VA_ARGS__); \
	dynamic_pr_debug("%s: " fmt, __func__, ##__VA_ARGS__); \
} while (0)
#else
#define mtp_log(fmt, ...) \
	ipc_log_string(_mtp_ipc_log, "%s: " fmt,  __func__, ##__VA_ARGS__)
#endif

#define MTP_RX_BUFFER_INIT_SIZE    1048576
#define MTP_TX_BUFFER_INIT_SIZE    1048576
>>>>>>> target/16.0
#define MTP_BULK_BUFFER_SIZE       16384
#define INTR_BUFFER_SIZE           28
#define MAX_INST_NAME_LEN          40
#define MTP_MAX_FILE_SIZE          0xFFFFFFFFL

/* String IDs */
#define INTERFACE_STRING_INDEX	0

/* values for mtp_dev.state */
#define STATE_OFFLINE               0   /* initial state, disconnected */
#define STATE_READY                 1   /* ready for userspace calls */
#define STATE_BUSY                  2   /* processing userspace calls */
#define STATE_CANCELED              3   /* transaction canceled by host */
#define STATE_ERROR                 4   /* error from completion routine */

/* number of tx and rx requests to allocate */
<<<<<<< HEAD
#define TX_REQ_MAX 4
#define RX_REQ_MAX 4
=======
#define MTP_TX_REQ_MAX 8
#define RX_REQ_MAX 2
>>>>>>> target/16.0
#define INTR_REQ_MAX 5

/* ID for Microsoft MTP OS String */
#define MTP_OS_STRING_ID   0xEE

/* MTP class reqeusts */
#define MTP_REQ_CANCEL              0x64
#define MTP_REQ_GET_EXT_EVENT_DATA  0x65
#define MTP_REQ_RESET               0x66
#define MTP_REQ_GET_DEVICE_STATUS   0x67

/* constants for device status */
#define MTP_RESPONSE_OK             0x2001
#define MTP_RESPONSE_DEVICE_BUSY    0x2019
#define DRIVER_NAME "mtp"

<<<<<<< HEAD
#define MTP_CONTAINER_LENGTH_OFFSET             0
#define MTP_CONTAINER_TYPE_OFFSET               4
#define MTP_CONTAINER_CODE_OFFSET               6
#define MTP_CONTAINER_TRANSACTION_ID_OFFSET     8
#define MTP_CONTAINER_PARAMETER_OFFSET          12
#define MTP_CONTAINER_HEADER_SIZE               12
#define MTP_DBG(fmt, args...) \
	pr_notice("MTP, <%s(), %d> " fmt, __func__, __LINE__, ## args)
#define MTP_DBG_LIMIT(FREQ, fmt, args...) do {\
	static DEFINE_RATELIMIT_STATE(ratelimit, HZ, FREQ);\
	static int skip_cnt;\
	\
	{ \
		if (__ratelimit(&ratelimit)) {\
			pr_notice("MTP, <%s(), %d> " fmt ", skip<%d>\n",\
					__func__, __LINE__, ## args, skip_cnt);\
			skip_cnt = 0;\
		} else\
			skip_cnt++;\
	} \
} while (0)

static bool cust_dump;
static int cust_dump_read = MTP_CONTAINER_HEADER_SIZE;
static int cust_dump_write = MTP_CONTAINER_HEADER_SIZE;
static int cust_dump_ioctl = MTP_CONTAINER_HEADER_SIZE;
static int monitor_work_interval_ms = 1000;
static bool monitor_time;
module_param(cust_dump, bool, 0644);
module_param(cust_dump_read, int, 0644);
module_param(cust_dump_write, int, 0644);
module_param(cust_dump_ioctl, int, 0644);
module_param(monitor_work_interval_ms, int, 0644);
module_param(monitor_time, bool, 0644);

static struct delayed_work monitor_work;
static void do_monitor_work(struct work_struct *work);
static void protocol_dump(char *data, int buf_len, int limit)
{
	u32 *len;
	u32 *type_code;
	u32 *id;

	if (buf_len < MTP_CONTAINER_HEADER_SIZE) {
		MTP_DBG("buf_len too small<%d>\n", buf_len);
		return;
	}

	len = (u32 *)(data + MTP_CONTAINER_LENGTH_OFFSET);
	type_code = (u32 *)(data + MTP_CONTAINER_TYPE_OFFSET);
	id = (u32 *)(data +  MTP_CONTAINER_TRANSACTION_ID_OFFSET);

	MTP_DBG("H<%x %x %x>\n", *len, *type_code, *id);

	/* dump the rest data */
	if (limit) {
		int i = MTP_CONTAINER_PARAMETER_OFFSET;
		int bound = min(limit, buf_len);

		while (i++ < bound)
			MTP_DBG("D[%d]:%x\n", i, data[i]);
	}
}

enum {
	MTP_READ = 0,
	MTP_WRITE,
	MTP_IOCTL,
	MTP_IOCTL_WORK,
	MTP_WAIT_EVENT_R1,
	MTP_WAIT_EVENT_R2,
	MTP_WAIT_EVENT,
	MTP_VFS_R,
	MTP_VFS_W,
	MTP_MAX_MONITOR_TYPE
};
static unsigned int monitor_in_cnt[MTP_MAX_MONITOR_TYPE];
static unsigned int monitor_out_cnt[MTP_MAX_MONITOR_TYPE];
static s64 ktime_ns[MTP_MAX_MONITOR_TYPE];
static ktime_t ktime_in[MTP_MAX_MONITOR_TYPE];
static ktime_t ktime_out[MTP_MAX_MONITOR_TYPE];
static void monitor_in(int id)
{
	monitor_in_cnt[id]++;

	if (likely(!monitor_time))
		return;

	/* TIME PROFILING */
	ktime_in[id] = ktime_get();
}
static void monitor_out(int id)
{
	monitor_out_cnt[id]++;

	if (likely(!monitor_time))
		return;

	/* TIME PROFILING */
	ktime_out[id] = ktime_get();
	ktime_ns[id] += ktime_to_ns(ktime_sub(ktime_out[id], ktime_in[id]));
}
static char *ioctl_string(unsigned int code)
{
	switch (code) {
	case MTP_SEND_FILE:
		return "MTP_SEND_FILE";
	case MTP_RECEIVE_FILE:
		return "MTP_RECEIVE_FILE";
	case MTP_SEND_FILE_WITH_HEADER:
		return "MTP_SEND_FILE_WITH_HEADER";
	case MTP_SEND_EVENT:
		return "MTP_SEND_EVENT";
	default:
		return "UNDEFINED";
	}
};


#define MTP_SEND_EVENT_TIMEOUT_CNT 5
static int mtp_send_event_timeout_cnt;

static bool mtp_skip_vfs_read;
static bool mtp_skip_vfs_write;
module_param(mtp_skip_vfs_read, bool, 0644);
module_param(mtp_skip_vfs_write, bool, 0644);

static const char mtp_shortname[] = DRIVER_NAME "_usb";

unsigned int mtp_rx_req_len = MTP_BULK_BUFFER_SIZE;
unsigned int mtp_tx_req_len = MTP_BULK_BUFFER_SIZE;

=======
#define MAX_ITERATION		100

unsigned int mtp_rx_req_len = MTP_RX_BUFFER_INIT_SIZE;
module_param(mtp_rx_req_len, uint, 0644);

unsigned int mtp_tx_req_len = MTP_TX_BUFFER_INIT_SIZE;
module_param(mtp_tx_req_len, uint, 0644);

unsigned int mtp_tx_reqs = MTP_TX_REQ_MAX;
module_param(mtp_tx_reqs, uint, 0644);

static const char mtp_shortname[] = DRIVER_NAME "_usb";

>>>>>>> target/16.0
struct mtp_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;
	struct usb_ep *ep_intr;

	int state;

	/* synchronize access to our device file */
	atomic_t open_excl;
	/* to enforce only one ioctl at a time */
	atomic_t ioctl_excl;

	struct list_head tx_idle;
	struct list_head intr_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	wait_queue_head_t intr_wq;
	struct usb_request *rx_req[RX_REQ_MAX];
	int rx_done;

	/* for processing MTP_SEND_FILE, MTP_RECEIVE_FILE and
	 * MTP_SEND_FILE_WITH_HEADER ioctls on a work queue
	 */
	struct workqueue_struct *wq;
	struct work_struct send_file_work;
	struct work_struct receive_file_work;
	struct file *xfer_file;
	loff_t xfer_file_offset;
	int64_t xfer_file_length;
<<<<<<< HEAD
	unsigned int xfer_send_header;
	uint16_t xfer_command;
	uint32_t xfer_transaction_id;
	int xfer_result;
	int is_boost;
	struct cpumask cpu_mask;
};

=======
	unsigned xfer_send_header;
	uint16_t xfer_command;
	uint32_t xfer_transaction_id;
	int xfer_result;
	struct {
		unsigned long vfs_rbytes;
		unsigned long vfs_wbytes;
		unsigned int vfs_rtime;
		unsigned int vfs_wtime;
	} perf[MAX_ITERATION];
	unsigned int dbg_read_index;
	unsigned int dbg_write_index;
	struct mutex  read_mutex;
};

static void *_mtp_ipc_log;

>>>>>>> target/16.0
static struct usb_interface_descriptor mtp_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 3,
<<<<<<< HEAD
	.bInterfaceClass        = USB_CLASS_STILL_IMAGE,
	.bInterfaceSubClass     = 1,
	.bInterfaceProtocol     = 1,
=======
	.bInterfaceClass        = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass     = USB_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol     = 0,
>>>>>>> target/16.0
};

static struct usb_interface_descriptor ptp_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,
	.bDescriptorType        = USB_DT_INTERFACE,
	.bInterfaceNumber       = 0,
	.bNumEndpoints          = 3,
	.bInterfaceClass        = USB_CLASS_STILL_IMAGE,
	.bInterfaceSubClass     = 1,
	.bInterfaceProtocol     = 1,
};

static struct usb_endpoint_descriptor mtp_ss_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
<<<<<<< HEAD
	.wMaxPacketSize         = cpu_to_le16(1024),
=======
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
>>>>>>> target/16.0
};

static struct usb_ss_ep_comp_descriptor mtp_ss_in_comp_desc = {
	.bLength                = sizeof(mtp_ss_in_comp_desc),
	.bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,
	/* .bMaxBurst           = DYNAMIC, */
};

static struct usb_endpoint_descriptor mtp_ss_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
<<<<<<< HEAD
	.wMaxPacketSize         = cpu_to_le16(1024),
=======
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
>>>>>>> target/16.0
};

static struct usb_ss_ep_comp_descriptor mtp_ss_out_comp_desc = {
	.bLength                = sizeof(mtp_ss_out_comp_desc),
	.bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,
	/* .bMaxBurst           = DYNAMIC, */
};

static struct usb_endpoint_descriptor mtp_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
<<<<<<< HEAD
	.wMaxPacketSize         = cpu_to_le16(512),
=======
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
>>>>>>> target/16.0
};

static struct usb_endpoint_descriptor mtp_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
<<<<<<< HEAD
	.wMaxPacketSize         = cpu_to_le16(512),
=======
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
>>>>>>> target/16.0
};

static struct usb_endpoint_descriptor mtp_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor mtp_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor mtp_intr_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_INT,
<<<<<<< HEAD
	.wMaxPacketSize         = cpu_to_le16(INTR_BUFFER_SIZE),
=======
	.wMaxPacketSize         = __constant_cpu_to_le16(INTR_BUFFER_SIZE),
>>>>>>> target/16.0
	.bInterval              = 6,
};

static struct usb_ss_ep_comp_descriptor mtp_intr_ss_comp_desc = {
	.bLength                = sizeof(mtp_intr_ss_comp_desc),
	.bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,
	.wBytesPerInterval      = cpu_to_le16(INTR_BUFFER_SIZE),
};

static struct usb_descriptor_header *fs_mtp_descs[] = {
	(struct usb_descriptor_header *) &mtp_interface_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_descriptor_header *hs_mtp_descs[] = {
	(struct usb_descriptor_header *) &mtp_interface_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_descriptor_header *ss_mtp_descs[] = {
	(struct usb_descriptor_header *) &mtp_interface_desc,
	(struct usb_descriptor_header *) &mtp_ss_in_desc,
	(struct usb_descriptor_header *) &mtp_ss_in_comp_desc,
	(struct usb_descriptor_header *) &mtp_ss_out_desc,
	(struct usb_descriptor_header *) &mtp_ss_out_comp_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	(struct usb_descriptor_header *) &mtp_intr_ss_comp_desc,
	NULL,
};

static struct usb_descriptor_header *fs_ptp_descs[] = {
	(struct usb_descriptor_header *) &ptp_interface_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_fullspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_descriptor_header *hs_ptp_descs[] = {
	(struct usb_descriptor_header *) &ptp_interface_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_in_desc,
	(struct usb_descriptor_header *) &mtp_highspeed_out_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	NULL,
};

static struct usb_descriptor_header *ss_ptp_descs[] = {
	(struct usb_descriptor_header *) &ptp_interface_desc,
	(struct usb_descriptor_header *) &mtp_ss_in_desc,
	(struct usb_descriptor_header *) &mtp_ss_in_comp_desc,
	(struct usb_descriptor_header *) &mtp_ss_out_desc,
	(struct usb_descriptor_header *) &mtp_ss_out_comp_desc,
	(struct usb_descriptor_header *) &mtp_intr_desc,
	(struct usb_descriptor_header *) &mtp_intr_ss_comp_desc,
	NULL,
};

static struct usb_string mtp_string_defs[] = {
	/* Naming interface "MTP" so libmtp will recognize us */
	[INTERFACE_STRING_INDEX].s	= "MTP",
	{  },	/* end of list */
};

static struct usb_gadget_strings mtp_string_table = {
	.language		= 0x0409,	/* en-US */
	.strings		= mtp_string_defs,
};

static struct usb_gadget_strings *mtp_strings[] = {
	&mtp_string_table,
	NULL,
};

/* Microsoft MTP OS String */
static u8 mtp_os_string[] = {
	18, /* sizeof(mtp_os_string) */
	USB_DT_STRING,
	/* Signature field: "MSFT100" */
	'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0,
	/* vendor code */
	1,
	/* padding */
	0
};

/* Microsoft Extended Configuration Descriptor Header Section */
struct mtp_ext_config_desc_header {
	__le32	dwLength;
	__u16	bcdVersion;
	__le16	wIndex;
	__u8	bCount;
	__u8	reserved[7];
};

/* Microsoft Extended Configuration Descriptor Function Section */
struct mtp_ext_config_desc_function {
	__u8	bFirstInterfaceNumber;
	__u8	bInterfaceCount;
	__u8	compatibleID[8];
	__u8	subCompatibleID[8];
	__u8	reserved[6];
};

/* MTP Extended Configuration Descriptor */
<<<<<<< HEAD
struct {
	struct mtp_ext_config_desc_header	header;
	struct mtp_ext_config_desc_function    function;
} mtp_ext_config_desc = {
	.header = {
		.dwLength = cpu_to_le32(sizeof(mtp_ext_config_desc)),
		.bcdVersion = cpu_to_le16(0x0100),
		.wIndex = cpu_to_le16(4),
=======
struct mtp_ext_config_desc {
	struct mtp_ext_config_desc_header	header;
	struct mtp_ext_config_desc_function    function;
};

static struct mtp_ext_config_desc mtp_ext_config_desc = {
	.header = {
		.dwLength = __constant_cpu_to_le32(sizeof(mtp_ext_config_desc)),
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(4),
>>>>>>> target/16.0
		.bCount = 1,
	},
	.function = {
		.bFirstInterfaceNumber = 0,
		.bInterfaceCount = 1,
		.compatibleID = { 'M', 'T', 'P' },
	},
};

struct mtp_device_status {
	__le16	wLength;
	__le16	wCode;
};

struct mtp_data_header {
	/* length of packet, including this header */
	__le32	length;
	/* container type (2 for data packet) */
	__le16	type;
	/* MTP command code */
	__le16	command;
	/* MTP transaction ID */
	__le32	transaction_id;
};

struct mtp_instance {
	struct usb_function_instance func_inst;
	const char *name;
	struct mtp_dev *dev;
	char mtp_ext_compat_id[16];
	struct usb_os_desc mtp_os_desc;
<<<<<<< HEAD
#ifdef CONFIG_USB_CONFIGFS_UEVENT
	struct device *mtp_device;
#endif
=======
>>>>>>> target/16.0
};

/* temporary variable used between mtp_open() and mtp_gadget_bind() */
static struct mtp_dev *_mtp_dev;

static inline struct mtp_dev *func_to_mtp(struct usb_function *f)
{
	return container_of(f, struct mtp_dev, function);
}

static struct usb_request *mtp_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);

	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
<<<<<<< HEAD
#if defined(CONFIG_64BIT) && defined(CONFIG_MTK_LM_MODE)
	req->buf = kmalloc(buffer_size, GFP_KERNEL | GFP_DMA);
#else
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
#endif
=======
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
>>>>>>> target/16.0
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void mtp_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int mtp_lock(atomic_t *excl)
{
<<<<<<< HEAD
	if (atomic_inc_return(excl) == 1)
		return 0;

	atomic_dec(excl);
	return -1;
=======
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
>>>>>>> target/16.0
}

static inline void mtp_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
static void mtp_req_put(struct mtp_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
<<<<<<< HEAD
	if (unlikely(head == &dev->intr_idle))
		mtp_send_event_timeout_cnt = 0;
=======
>>>>>>> target/16.0
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
static struct usb_request
*mtp_req_get(struct mtp_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;

	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static void mtp_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct mtp_dev *dev = _mtp_dev;

<<<<<<< HEAD
	if (req->status != 0)
=======
	if (req->status != 0 && dev->state != STATE_OFFLINE)
>>>>>>> target/16.0
		dev->state = STATE_ERROR;

	mtp_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

<<<<<<< HEAD
static struct completion usb_read_completion;
static atomic_t usb_rdone;
static int64_t usb_rcnt;
static int64_t vfs_wcnt;
static bool rx_cont_abort;
static bool mtp_rx_cont = true;
module_param(mtp_rx_cont, bool, 0644);
static bool mtp_rx_boost = true;
module_param(mtp_rx_boost, bool, 0644);
=======
>>>>>>> target/16.0
static void mtp_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct mtp_dev *dev = _mtp_dev;

	dev->rx_done = 1;
<<<<<<< HEAD
	if (req->status != 0) {
		dev->state = STATE_ERROR;
		rx_cont_abort = true;
	}

	if (mtp_rx_boost)
		usb_boost();

	wake_up(&dev->read_wq);
	atomic_inc(&usb_rdone);
	complete(&usb_read_completion);
=======
	if (req->status != 0 && dev->state != STATE_OFFLINE)
		dev->state = STATE_ERROR;

	wake_up(&dev->read_wq);
>>>>>>> target/16.0
}

static void mtp_complete_intr(struct usb_ep *ep, struct usb_request *req)
{
	struct mtp_dev *dev = _mtp_dev;

<<<<<<< HEAD
	if (req->status != 0)
=======
	if (req->status != 0 && dev->state != STATE_OFFLINE)
>>>>>>> target/16.0
		dev->state = STATE_ERROR;

	mtp_req_put(dev, &dev->intr_idle, req);

	wake_up(&dev->intr_wq);
}

static int mtp_create_bulk_endpoints(struct mtp_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc,
				struct usb_endpoint_descriptor *intr_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
<<<<<<< HEAD
	const unsigned int mtp_req_len[2] = { (MTP_BULK_BUFFER_SIZE*3),
		MTP_BULK_BUFFER_SIZE};
	int len_idx;
	int i;

	DBG(cdev, "create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_in got %s\n", ep->name);
=======
	int i;

	mtp_log("dev: %pK\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		mtp_log("usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	mtp_log("usb_ep_autoconfig for ep_in got %s\n", ep->name);
>>>>>>> target/16.0
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
<<<<<<< HEAD
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for mtp ep_out got %s\n", ep->name);
=======
		mtp_log("usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	mtp_log("usb_ep_autoconfig for mtp ep_out got %s\n", ep->name);
>>>>>>> target/16.0
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

	ep = usb_ep_autoconfig(cdev->gadget, intr_desc);
	if (!ep) {
<<<<<<< HEAD
		DBG(cdev, "usb_ep_autoconfig for ep_intr failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for mtp ep_intr got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_intr = ep;

	len_idx = 0;
retry_tx_alloc:
	/* now allocate requests for our endpoints */
	for (i = 0; i < TX_REQ_MAX; i++) {
		req = mtp_request_new(dev->ep_in, mtp_req_len[len_idx]);
		if (!req) {
			if (len_idx >= (ARRAY_SIZE(mtp_req_len)-1))
				goto fail;
			while ((req = mtp_req_get(dev, &dev->tx_idle)))
				mtp_request_free(req, dev->ep_in);
			len_idx++;
			pr_info("allocate TX fail. try %d\n",
				mtp_req_len[len_idx]);
=======
		mtp_log("usb_ep_autoconfig for ep_intr failed\n");
		return -ENODEV;
	}
	mtp_log("usb_ep_autoconfig for mtp ep_intr got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_intr = ep;

retry_tx_alloc:
	/* now allocate requests for our endpoints */
	for (i = 0; i < mtp_tx_reqs; i++) {
		req = mtp_request_new(dev->ep_in, mtp_tx_req_len);
		if (!req) {
			if (mtp_tx_req_len <= MTP_BULK_BUFFER_SIZE)
				goto fail;
			while ((req = mtp_req_get(dev, &dev->tx_idle)))
				mtp_request_free(req, dev->ep_in);
			mtp_tx_req_len = MTP_BULK_BUFFER_SIZE;
			mtp_tx_reqs = MTP_TX_REQ_MAX;
>>>>>>> target/16.0
			goto retry_tx_alloc;
		}
		req->complete = mtp_complete_in;
		mtp_req_put(dev, &dev->tx_idle, req);
	}
<<<<<<< HEAD
	mtp_tx_req_len = mtp_req_len[len_idx];

	len_idx = 0;
retry_rx_alloc:
	for (i = 0; i < RX_REQ_MAX; i++) {
		req = mtp_request_new(dev->ep_out, mtp_req_len[len_idx]);
		if (!req) {
			if (len_idx >= (ARRAY_SIZE(mtp_req_len)-1))
				goto fail;
			for (--i; i >= 0; i--)
				mtp_request_free(dev->rx_req[i], dev->ep_out);
			len_idx++;
			pr_info("allocate RX fail. try %d\n",
				mtp_req_len[len_idx]);
=======

	/*
	 * The RX buffer should be aligned to EP max packet for
	 * some controllers.  At bind time, we don't know the
	 * operational speed.  Hence assuming super speed max
	 * packet size.
	 */
	if (mtp_rx_req_len % 1024)
		mtp_rx_req_len = MTP_BULK_BUFFER_SIZE;

retry_rx_alloc:
	for (i = 0; i < RX_REQ_MAX; i++) {
		req = mtp_request_new(dev->ep_out, mtp_rx_req_len);
		if (!req) {
			if (mtp_rx_req_len <= MTP_BULK_BUFFER_SIZE)
				goto fail;
			for (--i; i >= 0; i--)
				mtp_request_free(dev->rx_req[i], dev->ep_out);
			mtp_rx_req_len = MTP_BULK_BUFFER_SIZE;
>>>>>>> target/16.0
			goto retry_rx_alloc;
		}
		req->complete = mtp_complete_out;
		dev->rx_req[i] = req;
	}
<<<<<<< HEAD
	mtp_rx_req_len = mtp_req_len[len_idx];

=======
>>>>>>> target/16.0
	for (i = 0; i < INTR_REQ_MAX; i++) {
		req = mtp_request_new(dev->ep_intr, INTR_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = mtp_complete_intr;
		mtp_req_put(dev, &dev->intr_idle, req);
	}

<<<<<<< HEAD
	pr_info("allocate RX=%d Tx=%d\n", mtp_rx_req_len, mtp_tx_req_len);

	return 0;

fail:
	pr_info("mtp_bind() could not allocate requests\n");
	return -1;
}

struct cpumask *mtp_get_cpu_mask(void)
{
	struct mtp_dev *dev = _mtp_dev;

	if (dev)
		return &(dev->cpu_mask);
	else
		return NULL;
}

void mtp_set_cpu_mask(unsigned int mask)
{
	struct mtp_dev *dev = _mtp_dev;
	int i = 1, idx = 0;

	cpumask_clear(&(dev->cpu_mask));

	while (i <= mask) {
		if (i & mask) {
			cpumask_set_cpu(idx, &(dev->cpu_mask));
			pr_info("Set CPU[%d] On\n", idx);
		}
		idx++;
		i = i << 1;
	}
	dev->is_boost = 0;
}

int mtp_get_mtp_server(void)
{
	struct mtp_dev *dev = _mtp_dev;

	if (dev)
		return dev->is_boost;
	else
		return -ENODEV;
}

#define MTP_QUEUE_DBG(fmt, args...)		\
	pr_info("MTP_QUEUE_DBG, <%s(), %d> " fmt, __func__, __LINE__, ## args)
#define MTP_QUEUE_DBG_STR_SZ 128

void mtp_dbg_dump(void)
{
	static char string[MTP_QUEUE_DBG_STR_SZ];
	int ret;

	ret = sprintf(string, "NOT MtpServer, task info<%d,%s>\n", current->pid,
			 current->comm);
	if (ret < 0)
		MTP_QUEUE_DBG("%s-%d, sprintf fail\n", __func__, __LINE__);
	MTP_QUEUE_DBG("%s\n", string);

#ifdef CONFIG_MEDIATEK_SOLUTION
	/* aee_kernel_warning_api(__FILE__, __LINE__,
	 *	DB_OPT_DEFAULT|DB_OPT_NATIVE_BACKTRACE, string, string);
	 */
#else
	{
		char *ptr = NULL;
		*ptr = 0;
	}
#endif
}

=======
	return 0;

fail:
	pr_err("mtp_bind() could not allocate requests\n");
	return -1;
}

>>>>>>> target/16.0
static ssize_t mtp_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	struct mtp_dev *dev = fp->private_data;
<<<<<<< HEAD
	struct usb_composite_dev *cdev;
	struct usb_request *req;
	ssize_t r = count;
	unsigned int xfer;
	int ret = 0;
	size_t len = 0;

	pr_debug("%s(%zu)\n", __func__, count);

	MTP_DBG_LIMIT(5, "in\n");

	if (true) {
		set_cpus_allowed_ptr(current, &dev->cpu_mask);
		dev->is_boost = current->pid;
	}

	if (count > MTP_BULK_BUFFER_SIZE)
		return -EINVAL;

	/* we will block until we're online */
	pr_debug("%s: waiting for online state\n", __func__);
	monitor_in(MTP_WAIT_EVENT_R1);
	ret = wait_event_interruptible(dev->read_wq,
		dev->state != STATE_OFFLINE);
	monitor_out(MTP_WAIT_EVENT_R1);
	if (ret < 0) {
		r = ret;
		goto done;
	}

	spin_lock_irq(&dev->lock);

	/* update cdev after online */
	cdev = dev->cdev;

=======
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	ssize_t r = count, xfer, len;
	int ret = 0;

	mtp_log("(%zu) state:%d\n", count, dev->state);

	/* we will block until we're online */
	ret = wait_event_interruptible(dev->read_wq,
		dev->state != STATE_OFFLINE);
	if (ret < 0) {
		r = ret;
		goto wait_err;
	}

	len = ALIGN(count, dev->ep_out->maxpacket);
	if (len > mtp_rx_req_len)
		return -EINVAL;

	spin_lock_irq(&dev->lock);
>>>>>>> target/16.0
	if (dev->state == STATE_OFFLINE) {
		spin_unlock_irq(&dev->lock);
		return -ENODEV;
	}
<<<<<<< HEAD
=======

>>>>>>> target/16.0
	if (dev->ep_out->desc) {
		if (!cdev) {
			spin_unlock_irq(&dev->lock);
			return -ENODEV;
		}

		len = usb_ep_align_maybe(cdev->gadget, dev->ep_out, count);
		if (len > MTP_BULK_BUFFER_SIZE) {
			spin_unlock_irq(&dev->lock);
			return -EINVAL;
		}
	}

	if (dev->state == STATE_CANCELED) {
<<<<<<< HEAD
		/* report cancellation to userspace */
=======
		/* report cancelation to userspace */
>>>>>>> target/16.0
		dev->state = STATE_READY;
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
	dev->state = STATE_BUSY;
	spin_unlock_irq(&dev->lock);

<<<<<<< HEAD
=======
	mutex_lock(&dev->read_mutex);
	if (dev->state == STATE_OFFLINE) {
		r = -EIO;
		goto done;
	}
>>>>>>> target/16.0
requeue_req:
	/* queue a request */
	req = dev->rx_req[0];
	req->length = len;
	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_KERNEL);
	if (ret < 0) {
		r = -EIO;
		goto done;
	} else {
<<<<<<< HEAD
		DBG(cdev, "rx %p queue\n", req);
	}

	/* wait for a request to complete */
	monitor_in(MTP_WAIT_EVENT_R2);
	ret = wait_event_interruptible(dev->read_wq,
				dev->rx_done || dev->state != STATE_BUSY);
	monitor_out(MTP_WAIT_EVENT_R2);
=======
		mtp_log("rx %pK queue\n", req);
	}

	/* wait for a request to complete */
	ret = wait_event_interruptible(dev->read_wq,
				dev->rx_done || dev->state != STATE_BUSY);
>>>>>>> target/16.0
	if (dev->state == STATE_CANCELED) {
		r = -ECANCELED;
		if (!dev->rx_done)
			usb_ep_dequeue(dev->ep_out, req);
		spin_lock_irq(&dev->lock);
		dev->state = STATE_CANCELED;
		spin_unlock_irq(&dev->lock);
		goto done;
	}
<<<<<<< HEAD

=======
>>>>>>> target/16.0
	if (ret < 0) {
		r = ret;
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}
	if (dev->state == STATE_BUSY) {
		/* If we got a 0-len packet, throw it back and try again. */
		if (req->actual == 0)
			goto requeue_req;

<<<<<<< HEAD
		if (likely(!cust_dump)) {
			static DEFINE_RATELIMIT_STATE(rlimit, 1 * HZ, 5);

			if (__ratelimit(&rlimit))
				protocol_dump((char *)req->buf,
						(int)req->actual, 0);
		} else
			protocol_dump((char *)req->buf,
					(int)req->actual, cust_dump_read);

		DBG(cdev, "rx %p %d\n", req, req->actual);
=======
		mtp_log("rx %pK %d\n", req, req->actual);
>>>>>>> target/16.0
		xfer = (req->actual < count) ? req->actual : count;
		r = xfer;
		if (copy_to_user(buf, req->buf, xfer))
			r = -EFAULT;
	} else
		r = -EIO;

done:
<<<<<<< HEAD
=======
	mutex_unlock(&dev->read_mutex);
wait_err:
>>>>>>> target/16.0
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		r = -ECANCELED;
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);

<<<<<<< HEAD
	pr_debug("%s %zd\n", __func__, r);
=======
	mtp_log("returning %zd state:%d\n", r, dev->state);
>>>>>>> target/16.0
	return r;
}

static ssize_t mtp_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	struct mtp_dev *dev = fp->private_data;
<<<<<<< HEAD
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req = 0;
	ssize_t r = count;
	unsigned int xfer;
	int sendZLP = 0;
	int ret;

	DBG(cdev, "%s(%zu)\n", __func__, count);

	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED) {
		/* report cancellation to userspace */
=======
	struct usb_request *req = 0;
	ssize_t r = count;
	unsigned xfer;
	int sendZLP = 0;
	int ret;

	mtp_log("(%zu) state:%d\n", count, dev->state);

	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED) {
		/* report cancelation to userspace */
>>>>>>> target/16.0
		dev->state = STATE_READY;
		spin_unlock_irq(&dev->lock);
		return -ECANCELED;
	}
	if (dev->state == STATE_OFFLINE) {
		spin_unlock_irq(&dev->lock);
		return -ENODEV;
	}
	dev->state = STATE_BUSY;
	spin_unlock_irq(&dev->lock);

	/* we need to send a zero length packet to signal the end of transfer
	 * if the transfer size is aligned to a packet boundary.
	 */
	if ((count & (dev->ep_in->maxpacket - 1)) == 0)
		sendZLP = 1;

	while (count > 0 || sendZLP) {
		/* so we exit after sending ZLP */
		if (count == 0)
			sendZLP = 0;

		if (dev->state != STATE_BUSY) {
<<<<<<< HEAD
			DBG(cdev, "%s dev->error\n", __func__);
=======
			mtp_log("dev->error\n");
>>>>>>> target/16.0
			r = -EIO;
			break;
		}

		/* get an idle tx request to use */
		req = 0;
<<<<<<< HEAD
		monitor_in(MTP_WAIT_EVENT);
		ret = wait_event_interruptible(dev->write_wq,
			((req = mtp_req_get(dev, &dev->tx_idle))
				|| dev->state != STATE_BUSY));
		monitor_out(MTP_WAIT_EVENT);
		if (!req) {
=======
		ret = wait_event_interruptible(dev->write_wq,
			((req = mtp_req_get(dev, &dev->tx_idle))
				|| dev->state != STATE_BUSY));
		if (!req) {
			mtp_log("request NULL ret:%d state:%d\n",
				ret, dev->state);
>>>>>>> target/16.0
			r = ret;
			break;
		}

<<<<<<< HEAD
		if (count > MTP_BULK_BUFFER_SIZE)
			xfer = MTP_BULK_BUFFER_SIZE;
=======
		if (count > mtp_tx_req_len)
			xfer = mtp_tx_req_len;
>>>>>>> target/16.0
		else
			xfer = count;
		if (xfer && copy_from_user(req->buf, buf, xfer)) {
			r = -EFAULT;
			break;
		}

<<<<<<< HEAD
		if (count == r) {
			static DEFINE_RATELIMIT_STATE(rlimit, 1 * HZ, 5);

			if (likely(!cust_dump)) {

				if (__ratelimit(&rlimit))
					protocol_dump((char *)req->buf,
							(int)xfer, 0);
			} else
				protocol_dump((char *)req->buf,
						(int)xfer, cust_dump_write);
		}

		req->length = xfer;
		ret = usb_ep_queue(dev->ep_in, req, GFP_KERNEL);
		if (ret < 0) {
			DBG(cdev, "%s: xfer error %d\n", __func__, ret);
=======
		req->length = xfer;
		ret = usb_ep_queue(dev->ep_in, req, GFP_KERNEL);
		if (ret < 0) {
			mtp_log("xfer error %d\n", ret);
>>>>>>> target/16.0
			r = -EIO;
			break;
		}

		buf += xfer;
		count -= xfer;

		/* zero this so we don't try to free it on error exit */
		req = 0;
	}

	if (req)
		mtp_req_put(dev, &dev->tx_idle, req);

	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		r = -ECANCELED;
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);

<<<<<<< HEAD
	DBG(cdev, "%s returning %zd\n", __func__, r);
=======
	mtp_log("returning %zd state:%d\n", r, dev->state);
>>>>>>> target/16.0
	return r;
}

/* read from a local file and write to USB */
static void send_file_work(struct work_struct *data)
{
	struct mtp_dev *dev = container_of(data, struct mtp_dev,
						send_file_work);
<<<<<<< HEAD
	struct usb_composite_dev *cdev = dev->cdev;
=======
>>>>>>> target/16.0
	struct usb_request *req = 0;
	struct mtp_data_header *header;
	struct file *filp;
	loff_t offset;
	int64_t count;
	int xfer, ret, hdr_size;
	int r = 0;
	int sendZLP = 0;
<<<<<<< HEAD
=======
	ktime_t start_time;
>>>>>>> target/16.0

	/* read our parameters */
	smp_rmb();
	filp = dev->xfer_file;
	offset = dev->xfer_file_offset;
	count = dev->xfer_file_length;

	if (count < 0) {
		dev->xfer_result = -EINVAL;
		return;
	}

<<<<<<< HEAD
	DBG(cdev, "%s(%lld %lld)\n", __func__, offset, count);
=======
	mtp_log("(%lld %lld)\n", offset, count);
>>>>>>> target/16.0

	if (dev->xfer_send_header) {
		hdr_size = sizeof(struct mtp_data_header);
		count += hdr_size;
	} else {
		hdr_size = 0;
	}

	/* we need to send a zero length packet to signal the end of transfer
	 * if the transfer size is aligned to a packet boundary.
	 */
	if ((count & (dev->ep_in->maxpacket - 1)) == 0)
		sendZLP = 1;

	while (count > 0 || sendZLP) {
		/* so we exit after sending ZLP */
		if (count == 0)
			sendZLP = 0;

		/* get an idle tx request to use */
		req = 0;
<<<<<<< HEAD
		monitor_in(MTP_WAIT_EVENT);
		ret = wait_event_interruptible(dev->write_wq,
			(req = mtp_req_get(dev, &dev->tx_idle))
			|| dev->state != STATE_BUSY);
		monitor_out(MTP_WAIT_EVENT);
=======
		ret = wait_event_interruptible(dev->write_wq,
			(req = mtp_req_get(dev, &dev->tx_idle))
			|| dev->state != STATE_BUSY);
>>>>>>> target/16.0
		if (dev->state == STATE_CANCELED) {
			r = -ECANCELED;
			break;
		}
		if (!req) {
<<<<<<< HEAD
=======
			mtp_log("request NULL ret:%d state:%d\n",
				ret, dev->state);
>>>>>>> target/16.0
			r = ret;
			break;
		}

		if (count > mtp_tx_req_len)
			xfer = mtp_tx_req_len;
		else
			xfer = count;

		if (hdr_size) {
			/* prepend MTP data header */
			header = (struct mtp_data_header *)req->buf;
			/*
<<<<<<< HEAD
			 * set file size with header according to
			 * MTP Specification v1.0
			 */
=======
                         * set file size with header according to
                         * MTP Specification v1.0
                         */
>>>>>>> target/16.0
			header->length = (count > MTP_MAX_FILE_SIZE) ?
				MTP_MAX_FILE_SIZE : __cpu_to_le32(count);
			header->type = __cpu_to_le16(2); /* data packet */
			header->command = __cpu_to_le16(dev->xfer_command);
			header->transaction_id =
					__cpu_to_le32(dev->xfer_transaction_id);
		}
<<<<<<< HEAD
		usb_boost();

		monitor_in(MTP_VFS_R);
		if (mtp_skip_vfs_read) {
			ret = (xfer - hdr_size);
			offset += ret;
		} else
			ret = vfs_read(filp,
					req->buf + hdr_size, xfer - hdr_size,
					&offset);
		monitor_out(MTP_VFS_R);

=======
		start_time = ktime_get();
		ret = vfs_read(filp, req->buf + hdr_size, xfer - hdr_size,
								&offset);
>>>>>>> target/16.0
		if (ret < 0) {
			r = ret;
			break;
		}
<<<<<<< HEAD
		xfer = ret + hdr_size;
=======

		xfer = ret + hdr_size;
		dev->perf[dev->dbg_read_index].vfs_rtime =
			ktime_to_us(ktime_sub(ktime_get(), start_time));
		dev->perf[dev->dbg_read_index].vfs_rbytes = xfer;
		dev->dbg_read_index = (dev->dbg_read_index + 1) % MAX_ITERATION;
>>>>>>> target/16.0
		hdr_size = 0;

		req->length = xfer;
		ret = usb_ep_queue(dev->ep_in, req, GFP_KERNEL);
		if (ret < 0) {
<<<<<<< HEAD
			DBG(cdev, "%s: xfer error %d\n", __func__, ret);
			dev->state = STATE_ERROR;
=======
			mtp_log("xfer error %d\n", ret);
			if (dev->state != STATE_OFFLINE)
				dev->state = STATE_ERROR;
>>>>>>> target/16.0
			r = -EIO;
			break;
		}

		count -= xfer;

		/* zero this so we don't try to free it on error exit */
		req = 0;
	}

	if (req)
		mtp_req_put(dev, &dev->tx_idle, req);

<<<<<<< HEAD
	DBG(cdev, "%s %d\n", __func__, r);
	/* write the result */
	dev->xfer_result = r;
	smp_wmb(); /* avoid context switch and race condiction */
=======
	mtp_log("returning %d state:%d\n", r, dev->state);
	/* write the result */
	dev->xfer_result = r;
	smp_wmb();
>>>>>>> target/16.0
}

/* read from USB and write to a local file */
static void receive_file_work(struct work_struct *data)
{
	struct mtp_dev *dev = container_of(data, struct mtp_dev,
						receive_file_work);
<<<<<<< HEAD
	struct usb_composite_dev *cdev = dev->cdev;
=======
>>>>>>> target/16.0
	struct usb_request *read_req = NULL, *write_req = NULL;
	struct file *filp;
	loff_t offset;
	int64_t count;
	int ret, cur_buf = 0;
	int r = 0;
<<<<<<< HEAD
	/* use this to avoid 4G copy issue */
	int64_t total_size = 0;
=======
	ktime_t start_time;
>>>>>>> target/16.0

	/* read our parameters */
	smp_rmb();
	filp = dev->xfer_file;
	offset = dev->xfer_file_offset;
	count = dev->xfer_file_length;

	if (count < 0) {
		dev->xfer_result = -EINVAL;
		return;
	}

<<<<<<< HEAD
	DBG(cdev, "%s(%lld)\n", __func__, count);

=======
	mtp_log("(%lld)\n", count);
	if (!IS_ALIGNED(count, dev->ep_out->maxpacket))
		mtp_log("- count(%lld) not multiple of mtu(%d)\n",
						count, dev->ep_out->maxpacket);
	mutex_lock(&dev->read_mutex);
	if (dev->state == STATE_OFFLINE) {
		r = -EIO;
		goto fail;
	}
>>>>>>> target/16.0
	while (count > 0 || write_req) {
		if (count > 0) {
			/* queue a request */
			read_req = dev->rx_req[cur_buf];
			cur_buf = (cur_buf + 1) % RX_REQ_MAX;

<<<<<<< HEAD
			read_req->length = (count > mtp_rx_req_len
					? mtp_rx_req_len : count);

			if (total_size >= 0xFFFFFFFF)
				read_req->short_not_ok = 0;
			else {
				if (0 == (read_req->length %
						dev->ep_out->maxpacket))
					read_req->short_not_ok = 1;
				else
					read_req->short_not_ok = 0;
			}
=======
			/* some h/w expects size to be aligned to ep's MTU */
			read_req->length = mtp_rx_req_len;
>>>>>>> target/16.0

			dev->rx_done = 0;
			ret = usb_ep_queue(dev->ep_out, read_req, GFP_KERNEL);
			if (ret < 0) {
				r = -EIO;
<<<<<<< HEAD
				dev->state = STATE_ERROR;
=======
				if (dev->state != STATE_OFFLINE)
					dev->state = STATE_ERROR;
>>>>>>> target/16.0
				break;
			}
		}

		if (write_req) {
<<<<<<< HEAD
			usb_boost();

			DBG(cdev, "rx %p %d\n", write_req, write_req->actual);
			monitor_in(MTP_VFS_W);
			if (mtp_skip_vfs_write) {
				ret = write_req->actual;
				offset += ret;
			} else
				ret = vfs_write(filp,
						write_req->buf,
						write_req->actual,
						&offset);
			monitor_out(MTP_VFS_W);
			DBG(cdev, "vfs_write %d\n", ret);
			if (ret != write_req->actual) {
				r = -EIO;
				dev->state = STATE_ERROR;
				break;
			}
=======
			mtp_log("rx %pK %d\n", write_req, write_req->actual);
			start_time = ktime_get();
			ret = vfs_write(filp, write_req->buf, write_req->actual,
				&offset);
			mtp_log("vfs_write %d\n", ret);
			if (ret != write_req->actual) {
				r = -EIO;
				if (dev->state != STATE_OFFLINE)
					dev->state = STATE_ERROR;
				if (read_req && !dev->rx_done)
					usb_ep_dequeue(dev->ep_out, read_req);
				break;
			}
			dev->perf[dev->dbg_write_index].vfs_wtime =
				ktime_to_us(ktime_sub(ktime_get(), start_time));
			dev->perf[dev->dbg_write_index].vfs_wbytes = ret;
			dev->dbg_write_index =
				(dev->dbg_write_index + 1) % MAX_ITERATION;
>>>>>>> target/16.0
			write_req = NULL;
		}

		if (read_req) {
			/* wait for our last read to complete */
<<<<<<< HEAD
			monitor_in(MTP_WAIT_EVENT);
			ret = wait_event_interruptible(dev->read_wq,
				dev->rx_done || dev->state != STATE_BUSY);
			monitor_out(MTP_WAIT_EVENT);
			if (dev->state == STATE_CANCELED) {
				r = -ECANCELED;
=======
			ret = wait_event_interruptible(dev->read_wq,
				dev->rx_done || dev->state != STATE_BUSY);
			if (dev->state == STATE_CANCELED
					|| dev->state == STATE_OFFLINE) {
				if (dev->state == STATE_OFFLINE)
					r = -EIO;
				else
					r = -ECANCELED;
>>>>>>> target/16.0
				if (!dev->rx_done)
					usb_ep_dequeue(dev->ep_out, read_req);
				break;
			}
			if (read_req->status) {
				r = read_req->status;
				break;
			}
<<<<<<< HEAD
=======

			/* Check if we aligned the size due to MTU constraint */
			if (count < read_req->length)
				read_req->actual = (read_req->actual > count ?
						count : read_req->actual);
>>>>>>> target/16.0
			/* if xfer_file_length is 0xFFFFFFFF, then we read until
			 * we get a zero length packet
			 */
			if (count != 0xFFFFFFFF)
				count -= read_req->actual;
<<<<<<< HEAD

			total_size += read_req->actual;

=======
>>>>>>> target/16.0
			if (read_req->actual < read_req->length) {
				/*
				 * short packet is used to signal EOF for
				 * sizes > 4 gig
				 */
<<<<<<< HEAD
				DBG(cdev, "got short packet\n");
				count = 0;
			}

			/* Add for RX mode 1 */
			read_req->short_not_ok = 0;

=======
				mtp_log("got short packet\n");
				count = 0;
			}

>>>>>>> target/16.0
			write_req = read_req;
			read_req = NULL;
		}
	}
<<<<<<< HEAD

	if (dev->state == STATE_ERROR || dev->state == STATE_OFFLINE) {
		DBG(dev->cdev, "%s, line %d: read_req = %p\n", __func__,
			 __LINE__, read_req);
		if (read_req)
			read_req->short_not_ok = 0;
	}

	DBG(cdev, "%s returning %d\n", __func__, r);
	/* write the result */
	dev->xfer_result = r;
	smp_wmb(); /* avoid context switch and race condiction */
}

/* #define MTP_RX_DBG_ON */
#ifdef MTP_RX_DBG_ON
#define MTP_RX_DBG(fmt, args...) \
pr_notice("MTP_RX_DBG, <%s(), %d> " fmt, __func__, __LINE__, ## args)
#else
#define MTP_RX_DBG(fmt, args...) do {} while (0)
#endif
static void vfs_write_work(struct work_struct *data)
{
	struct mtp_dev *dev = _mtp_dev;
	struct usb_request *write_req;
	struct file *filp = dev->xfer_file;
	loff_t offset = dev->xfer_file_offset;
	int64_t req_cnt = dev->xfer_file_length;
	int index = 0;

	MTP_RX_DBG("write_cnt<%lld>, read_cnt<%lld>, req_cnt<%lld>\n",
			vfs_wcnt, usb_rcnt, req_cnt);

	for (;;) {
		wait_for_completion_interruptible(&usb_read_completion);

		/* unbind case */
		if (rx_cont_abort)
			goto exit;

		/* cancel case */
		if (dev->state != STATE_BUSY) {
			rx_cont_abort = true;
			MTP_RX_DBG("state<%d>\n", dev->state);
			goto exit;
		}
		/* deal with what we have */
		while (atomic_read(&usb_rdone) > 0) {
			int rc;

			write_req = dev->rx_req[index];
			index = (index + 1) % RX_REQ_MAX;
			atomic_dec(&usb_rdone);
			MTP_RX_DBG("write_req<%p>, len<%d>, usb_rdone<%d>\n",
					write_req, write_req->actual,
					atomic_read(&usb_rdone));

			usb_boost();
			monitor_in(MTP_VFS_W);
			if (mtp_skip_vfs_write) {
				rc = write_req->actual;
				offset += rc;
			} else
				rc = vfs_write(filp, write_req->buf,
					write_req->actual, &offset);
			monitor_out(MTP_VFS_W);

			if (rc != write_req->actual)
				MTP_RX_DBG("rc<%d> != actual<%d>\n",
					rc, write_req->actual);
			vfs_wcnt += write_req->actual;
			MTP_RX_DBG("vfs_wcnt%lld,usb_rcnt%lld,req_cnt%lld\n",
					vfs_wcnt, usb_rcnt,
					req_cnt);


			/* check next round existence */
			if (usb_rcnt != req_cnt) {
				int64_t count = (req_cnt - usb_rcnt);
				struct usb_request *read_req = write_req;

				read_req->length = (count > mtp_rx_req_len
						? mtp_rx_req_len : count);
				MTP_RX_DBG("read_req<%p>, len<%d>\n",
					read_req, read_req->length);
				rc = usb_ep_queue(dev->ep_out, read_req,
					GFP_KERNEL);
				if (unlikely(rc)) {
					rx_cont_abort = true;
					MTP_RX_DBG("rc<%d>\n", rc);
					goto exit;
				}
				usb_rcnt += read_req->length;
				MTP_RX_DBG("v_wcnt%lld,u_rcnt%lld,req%lld\n",
						vfs_wcnt, usb_rcnt,
						req_cnt);
			}
		}
		/* check done or not */
		if (vfs_wcnt == req_cnt)
			goto exit;

	} /* for (;;) */
exit:
	MTP_RX_DBG("write_cnt<%lld>, read_cnt<%lld>, req_cnt<%lld>\n",
			vfs_wcnt, usb_rcnt, req_cnt);
}
void trigger_rx_cont(void)
{
	int i;
	struct usb_request *read_req;
	struct mtp_dev *dev = _mtp_dev;
	int64_t count = dev->xfer_file_length;
	static struct work_struct work;
	static int work_inited;

	if (count <= 0) {
		MTP_RX_DBG("count<%d> invalid\n", (int)count);
		return;
	}

	/* reset condition */
	atomic_set(&usb_rdone, 0);
	usb_rcnt = vfs_wcnt = 0;
	rx_cont_abort = false;
	init_completion(&usb_read_completion);

	mb(); /* make all related status reset and sync */

	MTP_RX_DBG("count<%d>\n", (int)count);

	/* USB related */
	for (i = 0; i < RX_REQ_MAX; i++) {
		int rc;

		read_req = dev->rx_req[i];
		read_req->length = (count > mtp_rx_req_len
				? mtp_rx_req_len : count);
		MTP_RX_DBG("i<%d>, read_req<%p>, len<%d>\n",
			i, read_req, read_req->length);
		rc = usb_ep_queue(dev->ep_out, read_req, GFP_KERNEL);
		if (unlikely(rc)) {
			rx_cont_abort = true;
			MTP_RX_DBG("rc<%d>\n", rc);
			break;
		}

		usb_rcnt += read_req->length;
		count -= read_req->length;
		MTP_RX_DBG("count<%lld>, usb_rcnt<%lld>\n",
			count, usb_rcnt);
		if (!count)
			break;
	}

	/* VFS related */
	if (!work_inited) {
		INIT_WORK(&work, vfs_write_work);
		work_inited = 1;
	}
	queue_work(dev->wq, &work);
	flush_workqueue(dev->wq);

	/* check status */
	if (unlikely(rx_cont_abort)) {
		if (dev->state == STATE_CANCELED) {
			dev->xfer_result = -ECANCELED;
			/* recycle request */
			for (i = 0; i < RX_REQ_MAX; i++)
				usb_ep_dequeue(dev->ep_out,  dev->rx_req[i]);
		} else
			dev->xfer_result = -EIO;
	} else
		dev->xfer_result = 0;
	MTP_RX_DBG("xfer_result<%d>\n", dev->xfer_result);
=======
fail:
	mutex_unlock(&dev->read_mutex);
	mtp_log("returning %d\n", r);
	/* write the result */
	dev->xfer_result = r;
	smp_wmb();
>>>>>>> target/16.0
}

static int mtp_send_event(struct mtp_dev *dev, struct mtp_event *event)
{
	struct usb_request *req = NULL;
	int ret;
	int length = event->length;

<<<<<<< HEAD
	DBG(dev->cdev, "%s(%zu)\n", __func__, event->length);
=======
	mtp_log("(%zu)\n", event->length);
>>>>>>> target/16.0

	if (length < 0 || length > INTR_BUFFER_SIZE)
		return -EINVAL;
	if (dev->state == STATE_OFFLINE)
		return -ENODEV;

<<<<<<< HEAD
	if (mtp_send_event_timeout_cnt > MTP_SEND_EVENT_TIMEOUT_CNT) {
		pr_info("%s, timeout count<%d> exceed %d, directly return\n",
			__func__, mtp_send_event_timeout_cnt,
			MTP_SEND_EVENT_TIMEOUT_CNT);
		return -ETIME;
	}

	ret = wait_event_interruptible_timeout(dev->intr_wq,
			(req = mtp_req_get(dev, &dev->intr_idle)),
			msecs_to_jiffies(1000));
	if (!req) {
		mtp_send_event_timeout_cnt++;
		pr_info("%s, timeout count<%d>\n", __func__,
			mtp_send_event_timeout_cnt);
		return -ETIME;
	}
=======
	ret = wait_event_interruptible_timeout(dev->intr_wq,
			(req = mtp_req_get(dev, &dev->intr_idle)),
			msecs_to_jiffies(1000));
	if (!req)
		return -ETIME;
>>>>>>> target/16.0

	if (copy_from_user(req->buf, (void __user *)event->data, length)) {
		mtp_req_put(dev, &dev->intr_idle, req);
		return -EFAULT;
	}
<<<<<<< HEAD

	if (likely(!cust_dump) && !strstr(current->comm, "process.media")) {
		static DEFINE_RATELIMIT_STATE(rlimit, 1 * HZ, 5);

		if (__ratelimit(&rlimit))
			protocol_dump((char *)req->buf,
					length, 0);
	} else
		protocol_dump((char *)req->buf,
				length, cust_dump_ioctl);

=======
>>>>>>> target/16.0
	req->length = length;
	ret = usb_ep_queue(dev->ep_intr, req, GFP_KERNEL);
	if (ret)
		mtp_req_put(dev, &dev->intr_idle, req);

	return ret;
}

<<<<<<< HEAD
static long mtp_ioctl(struct file *fp, unsigned int code, unsigned long value)
{
	struct mtp_dev *dev = fp->private_data;
	struct file *filp = NULL;
	int ret = -EINVAL;

	if (mtp_lock(&dev->ioctl_excl)) {
		MTP_DBG("BUSY, action<%s>\n", ioctl_string(code));
		return -EBUSY;
	}

=======
static long mtp_send_receive_ioctl(struct file *fp, unsigned int code,
	struct mtp_file_range *mfr)
{
	struct mtp_dev *dev = fp->private_data;
	struct file *filp = NULL;
	struct work_struct *work;
	int ret = -EINVAL;

	if (mtp_lock(&dev->ioctl_excl)) {
		mtp_log("ioctl returning EBUSY state:%d\n", dev->state);
		return -EBUSY;
	}

	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED) {
		/* report cancellation to userspace */
		dev->state = STATE_READY;
		spin_unlock_irq(&dev->lock);
		ret = -ECANCELED;
		goto out;
	}
	if (dev->state == STATE_OFFLINE) {
		spin_unlock_irq(&dev->lock);
		ret = -ENODEV;
		goto out;
	}
	dev->state = STATE_BUSY;
	spin_unlock_irq(&dev->lock);

	/* hold a reference to the file while we are working with it */
	filp = fget(mfr->fd);
	if (!filp) {
		ret = -EBADF;
		goto fail;
	}

	/* write the parameters */
	dev->xfer_file = filp;
	dev->xfer_file_offset = mfr->offset;
	dev->xfer_file_length = mfr->length;
	/* make sure write is done before parameters are read */
	smp_wmb();

	if (code == MTP_SEND_FILE_WITH_HEADER) {
		work = &dev->send_file_work;
		dev->xfer_send_header = 1;
		dev->xfer_command = mfr->command;
		dev->xfer_transaction_id = mfr->transaction_id;
	} else if (code == MTP_SEND_FILE) {
		work = &dev->send_file_work;
		dev->xfer_send_header = 0;
	} else {
		work = &dev->receive_file_work;
	}

	/* We do the file transfer on a work queue so it will run
	 * in kernel context, which is necessary for vfs_read and
	 * vfs_write to use our buffers in the kernel address space.
	 */
	queue_work(dev->wq, work);
	/* wait for operation to complete */
	flush_workqueue(dev->wq);
	fput(filp);

	/* read the result */
	smp_rmb();
	ret = dev->xfer_result;

fail:
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		ret = -ECANCELED;
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);
out:
	mtp_unlock(&dev->ioctl_excl);
	mtp_log("ioctl returning %d\n", ret);
	return ret;
}

static long mtp_ioctl(struct file *fp, unsigned int code, unsigned long value)
{
	struct mtp_dev *dev = fp->private_data;
	struct mtp_file_range	mfr;
	struct mtp_event	event;
	int ret = -EINVAL;

>>>>>>> target/16.0
	switch (code) {
	case MTP_SEND_FILE:
	case MTP_RECEIVE_FILE:
	case MTP_SEND_FILE_WITH_HEADER:
<<<<<<< HEAD
	{
		struct mtp_file_range	mfr;
		struct work_struct *work;

		spin_lock_irq(&dev->lock);
		if (dev->state == STATE_CANCELED) {
			/* report cancellation to userspace */
			dev->state = STATE_READY;
			spin_unlock_irq(&dev->lock);
			ret = -ECANCELED;
			goto out;
		}
		if (dev->state == STATE_OFFLINE) {
			spin_unlock_irq(&dev->lock);
			ret = -ENODEV;
			goto out;
		}
		dev->state = STATE_BUSY;
		spin_unlock_irq(&dev->lock);

=======
>>>>>>> target/16.0
		if (copy_from_user(&mfr, (void __user *)value, sizeof(mfr))) {
			ret = -EFAULT;
			goto fail;
		}
<<<<<<< HEAD
		/* hold a reference to the file while we are working with it */
		filp = fget(mfr.fd);
		if (!filp) {
			ret = -EBADF;
			goto fail;
		}

		/* write the parameters */
		dev->xfer_file = filp;
		dev->xfer_file_offset = mfr.offset;
		dev->xfer_file_length = mfr.length;
		smp_wmb(); /* avoid context switch and race condiction */

		if (unlikely(cust_dump))
			MTP_DBG("action<%s>, len<%lld>\n",
					ioctl_string(code),
					dev->xfer_file_length);

		if (code == MTP_SEND_FILE_WITH_HEADER) {
			work = &dev->send_file_work;
			dev->xfer_send_header = 1;
			dev->xfer_command = mfr.command;
			dev->xfer_transaction_id = mfr.transaction_id;
		} else if (code == MTP_SEND_FILE) {
			work = &dev->send_file_work;
			dev->xfer_send_header = 0;
		} else {
			work = &dev->receive_file_work;
		}

		/* We do the file transfer on a work queue so it will run
		 * in kernel context, which is necessary for vfs_read and
		 * vfs_write to use our buffers in the kernel address space.
		 */
		monitor_in(MTP_IOCTL_WORK);
		if (code != MTP_RECEIVE_FILE) {
			queue_work(dev->wq, work);
			/* wait for operation to complete */
			flush_workqueue(dev->wq);
		} else {
			bool rx_cont = mtp_rx_cont;

			/* deal with (512K + 1) ~ (0xFFFFFFFE) */
			if (rx_cont && (dev->xfer_file_length <= 524288
				|| dev->xfer_file_length == 0xFFFFFFFF))
				rx_cont = false;

			if (rx_cont)
				trigger_rx_cont();
			else {
				queue_work(dev->wq, work);
				/* wait for operation to complete */
				flush_workqueue(dev->wq);
			}
		}
		monitor_out(MTP_IOCTL_WORK);
		fput(filp);

		/* read the result */
		smp_rmb();
		ret = dev->xfer_result;
		break;
	}
	case MTP_SEND_EVENT:
	{
		struct mtp_event	event;
=======
		ret = mtp_send_receive_ioctl(fp, code, &mfr);
	break;
	case MTP_SEND_EVENT:
		if (mtp_lock(&dev->ioctl_excl))
			return -EBUSY;
>>>>>>> target/16.0
		/* return here so we don't change dev->state below,
		 * which would interfere with bulk transfer state.
		 */
		if (copy_from_user(&event, (void __user *)value, sizeof(event)))
			ret = -EFAULT;
		else
			ret = mtp_send_event(dev, &event);
<<<<<<< HEAD
		goto out;
	}
	}

fail:
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		ret = -ECANCELED;
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);
out:
	mtp_unlock(&dev->ioctl_excl);
	DBG(dev->cdev, "ioctl returning %d\n", ret);
	return ret;
}

#ifdef CONFIG_COMPAT
static long compat_mtp_ioctl(struct file *fp, unsigned int code,
				unsigned long value)
{
	struct mtp_dev *dev = fp->private_data;
	struct file *filp = NULL;
	int ret = -EINVAL;

	if (mtp_lock(&dev->ioctl_excl))
		return -EBUSY;

	switch (code) {
	case MTP_SEND_FILE:
	case MTP_RECEIVE_FILE:
	case MTP_SEND_FILE_WITH_HEADER:
	{
		struct mtp_file_range	mfr;
		struct work_struct *work;

		spin_lock_irq(&dev->lock);
		if (dev->state == STATE_CANCELED) {
			/* report cancellation to userspace */
			dev->state = STATE_READY;
			spin_unlock_irq(&dev->lock);
			ret = -ECANCELED;
			goto out;
		}
		if (dev->state == STATE_OFFLINE) {
			spin_unlock_irq(&dev->lock);
			ret = -ENODEV;
			goto out;
		}
		dev->state = STATE_BUSY;
		spin_unlock_irq(&dev->lock);

		if (copy_from_user(&mfr, (void __user *)value, sizeof(mfr))) {
			ret = -EFAULT;
			goto fail;
		}
		/* hold a reference to the file while we are working with it */
		filp = fget(mfr.fd);
		if (!filp) {
			ret = -EBADF;
			goto fail;
		}

		/* write the parameters */
		dev->xfer_file = filp;
		dev->xfer_file_offset = mfr.offset;
		dev->xfer_file_length = mfr.length;
		smp_wmb(); /* avoid context switch and race condiction */

		if (code == MTP_SEND_FILE_WITH_HEADER) {
			work = &dev->send_file_work;
			dev->xfer_send_header = 1;
			dev->xfer_command = mfr.command;
			dev->xfer_transaction_id = mfr.transaction_id;
		} else if (code == MTP_SEND_FILE) {
			work = &dev->send_file_work;
			dev->xfer_send_header = 0;
		} else {
			work = &dev->receive_file_work;
		}

		/* We do the file transfer on a work queue so it will run
		 * in kernel context, which is necessary for vfs_read and
		 * vfs_write to use our buffers in the kernel address space.
		 */
		queue_work(dev->wq, work);
		/* wait for operation to complete */
		flush_workqueue(dev->wq);
		fput(filp);

		/* read the result */
		smp_rmb();
		ret = dev->xfer_result;
		break;
	}
	case COMPAT_MTP_SEND_EVENT:
	{
		struct mtp_event	event;
		struct __compat_mtp_event cevent;
=======
		mtp_unlock(&dev->ioctl_excl);
	break;
	default:
		mtp_log("unknown ioctl code: %d\n", code);
	}
fail:
	return ret;
}

/*
 * 32 bit userspace calling into 64 bit kernel. handle ioctl code
 * and userspace pointer
 */
#ifdef CONFIG_COMPAT
static long compat_mtp_ioctl(struct file *fp, unsigned int code,
	unsigned long value)
{
	struct mtp_dev *dev = fp->private_data;
	struct mtp_file_range	mfr;
	struct __compat_mtp_file_range	cmfr;
	struct mtp_event	event;
	struct __compat_mtp_event cevent;
	unsigned int cmd;
	bool send_file = false;
	int ret = -EINVAL;

	switch (code) {
	case COMPAT_MTP_SEND_FILE:
		cmd = MTP_SEND_FILE;
		send_file = true;
		break;
	case COMPAT_MTP_RECEIVE_FILE:
		cmd = MTP_RECEIVE_FILE;
		send_file = true;
		break;
	case COMPAT_MTP_SEND_FILE_WITH_HEADER:
		cmd = MTP_SEND_FILE_WITH_HEADER;
		send_file = true;
		break;
	case COMPAT_MTP_SEND_EVENT:
		cmd = MTP_SEND_EVENT;
		break;
	default:
		mtp_log("unknown compat_ioctl code: %d\n", code);
		ret = -ENOIOCTLCMD;
		goto fail;
	}

	if (send_file) {
		if (copy_from_user(&cmfr, (void __user *)value, sizeof(cmfr))) {
			ret = -EFAULT;
			goto fail;
		}
		mfr.fd = cmfr.fd;
		mfr.offset = cmfr.offset;
		mfr.length = cmfr.length;
		mfr.command = cmfr.command;
		mfr.transaction_id = cmfr.transaction_id;
		ret = mtp_send_receive_ioctl(fp, cmd, &mfr);
	} else {
		if (mtp_lock(&dev->ioctl_excl))
			return -EBUSY;
>>>>>>> target/16.0
		/* return here so we don't change dev->state below,
		 * which would interfere with bulk transfer state.
		 */
		if (copy_from_user(&cevent, (void __user *)value,
<<<<<<< HEAD
					sizeof(cevent)))
			ret = -EFAULT;
		else {
			event.length = cevent.length;
			event.data = compat_ptr(cevent.data);
			ret = mtp_send_event(dev, &event);
		}
		goto out;
	}
	}

fail:
	spin_lock_irq(&dev->lock);
	if (dev->state == STATE_CANCELED)
		ret = -ECANCELED;
	else if (dev->state != STATE_OFFLINE)
		dev->state = STATE_READY;
	spin_unlock_irq(&dev->lock);
out:
	mtp_unlock(&dev->ioctl_excl);
	DBG(dev->cdev, "ioctl returning %d\n", ret);
=======
			sizeof(cevent))) {
			ret = -EFAULT;
			goto fail;
		}
		event.length = cevent.length;
		event.data = compat_ptr(cevent.data);
		ret = mtp_send_event(dev, &event);
		mtp_unlock(&dev->ioctl_excl);
	}
fail:
>>>>>>> target/16.0
	return ret;
}
#endif

static int mtp_open(struct inode *ip, struct file *fp)
{
<<<<<<< HEAD
	static bool inited;

	pr_info("%s\n", __func__);
	if (mtp_lock(&_mtp_dev->open_excl)) {
		MTP_DBG("BUSY\n");
		return -EBUSY;
	}

	if (!inited) {
		inited = true;
		INIT_DELAYED_WORK(&monitor_work, do_monitor_work);
		schedule_delayed_work(&monitor_work, 0);
	} else
		schedule_delayed_work(&monitor_work, 0);

=======
	printk(KERN_INFO "mtp_open\n");
	if (mtp_lock(&_mtp_dev->open_excl)) {
		pr_err("%s mtp_release not called returning EBUSY\n", __func__);
		return -EBUSY;
	}

>>>>>>> target/16.0
	/* clear any error condition */
	if (_mtp_dev->state != STATE_OFFLINE)
		_mtp_dev->state = STATE_READY;

	fp->private_data = _mtp_dev;
	return 0;
}

static int mtp_release(struct inode *ip, struct file *fp)
{
<<<<<<< HEAD
	pr_info("%s\n", __func__);

	cancel_delayed_work(&monitor_work);

	_mtp_dev->is_boost = 0;
=======
	printk(KERN_INFO "mtp_release\n");
>>>>>>> target/16.0

	mtp_unlock(&_mtp_dev->open_excl);
	return 0;
}

<<<<<<< HEAD
static ssize_t monitor_mtp_read(struct file *fp, char __user *buf,
	size_t count, loff_t *pos)
{
	ssize_t r;

	monitor_in(MTP_READ);

	r = mtp_read(fp, buf, count, pos);

	monitor_out(MTP_READ);

	return r;
}
static ssize_t monitor_mtp_write(struct file *fp, const char __user *buf,
	size_t count, loff_t *pos)
{
	ssize_t r;

	monitor_in(MTP_WRITE);

	r = mtp_write(fp, buf, count, pos);

	monitor_out(MTP_WRITE);

	return r;
}
static long monitor_mtp_ioctl(struct file *fp,
		unsigned int code, unsigned long value)
{
	long r;

	if (code != MTP_SEND_EVENT)
		monitor_in(MTP_IOCTL);

	r = mtp_ioctl(fp, code, value);

	if (code != MTP_SEND_EVENT)
		monitor_out(MTP_IOCTL);

	return r;
}
static void do_monitor_work(struct work_struct *work)
{
	int i, r;
	char string_container[128];

	r = sprintf(string_container, "IN <");
	if (r >= 0 && r < ARRAY_SIZE(string_container))
		for (i = 0; i < MTP_MAX_MONITOR_TYPE; i++)
			r += sprintf(string_container + r, "%d ",
				monitor_in_cnt[i]);
	MTP_DBG("%s>\n", string_container);

	r = sprintf(string_container, "OUT <");
	if (r >= 0 && r < ARRAY_SIZE(string_container))
		for (i = 0; i < MTP_MAX_MONITOR_TYPE; i++)
			r += sprintf(string_container + r, "%d ",
				monitor_out_cnt[i]);
	MTP_DBG("%s>\n", string_container);

	if (likely(!monitor_time))
		goto monitor_work_exit;

	/* TIME PROFILING */
	r = sprintf(string_container, "TIME <");
	if (r >= 0 && r < ARRAY_SIZE(string_container))
		for (i = 0; i < MTP_MAX_MONITOR_TYPE; i++)
			r += sprintf(string_container + r, "%lld ",
				ktime_ns[i]);
	MTP_DBG("%s>\n", string_container);

monitor_work_exit:
	schedule_delayed_work(&monitor_work,
			msecs_to_jiffies(monitor_work_interval_ms));
}

/* file operations for /dev/mtp_usb */
static const struct file_operations mtp_fops = {
	.owner = THIS_MODULE,
	.read = monitor_mtp_read,
	.write = monitor_mtp_write,
	.unlocked_ioctl = monitor_mtp_ioctl,
=======
/* file operations for /dev/mtp_usb */
static const struct file_operations mtp_fops = {
	.owner = THIS_MODULE,
	.read = mtp_read,
	.write = mtp_write,
	.unlocked_ioctl = mtp_ioctl,
>>>>>>> target/16.0
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_mtp_ioctl,
#endif
	.open = mtp_open,
	.release = mtp_release,
};

static struct miscdevice mtp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = mtp_shortname,
	.fops = &mtp_fops,
};

static int mtp_ctrlrequest(struct usb_composite_dev *cdev,
				const struct usb_ctrlrequest *ctrl)
{
	struct mtp_dev *dev = _mtp_dev;
	int	value = -EOPNOTSUPP;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	unsigned long	flags;

<<<<<<< HEAD
	VDBG(cdev, "%s %x.%x v%x i%x l%u\n",
			__func__, ctrl->bRequestType, ctrl->bRequest,
=======
	mtp_log("%02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
>>>>>>> target/16.0
			w_value, w_index, w_length);

	/* Handle MTP OS string */
	if (ctrl->bRequestType ==
			(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)
			&& ctrl->bRequest == USB_REQ_GET_DESCRIPTOR
			&& (w_value >> 8) == USB_DT_STRING
			&& (w_value & 0xFF) == MTP_OS_STRING_ID) {
		value = (w_length < sizeof(mtp_os_string)
				? w_length : sizeof(mtp_os_string));
		memcpy(cdev->req->buf, mtp_os_string, value);
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		/* Handle MTP OS descriptor */
<<<<<<< HEAD
		DBG(cdev, "vendor request: %d index: %d value: %d length: %d\n",
=======
		mtp_log("vendor request: %d index: %d value: %d length: %d\n",
>>>>>>> target/16.0
			ctrl->bRequest, w_index, w_value, w_length);

		if (ctrl->bRequest == 1
				&& (ctrl->bRequestType & USB_DIR_IN)
				&& (w_index == 4 || w_index == 5)) {
			value = (w_length < sizeof(mtp_ext_config_desc) ?
					w_length : sizeof(mtp_ext_config_desc));
			memcpy(cdev->req->buf, &mtp_ext_config_desc, value);
<<<<<<< HEAD
		}
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		DBG(cdev, "class request: %d index: %d value: %d length: %d\n",
=======

			/* update compatibleID if PTP */
			if (dev->function.fs_descriptors == fs_ptp_descs) {
				struct mtp_ext_config_desc *d = cdev->req->buf;

				d->function.compatibleID[0] = 'P';
			}
		}
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		mtp_log("class request: %d index: %d value: %d length: %d\n",
>>>>>>> target/16.0
			ctrl->bRequest, w_index, w_value, w_length);

		if (ctrl->bRequest == MTP_REQ_CANCEL && w_index == 0
				&& w_value == 0) {
<<<<<<< HEAD
			ERROR(cdev, "MTP_REQ_CANCEL\n");
=======
			mtp_log("MTP_REQ_CANCEL\n");
>>>>>>> target/16.0

			spin_lock_irqsave(&dev->lock, flags);
			if (dev->state == STATE_BUSY) {
				dev->state = STATE_CANCELED;
				wake_up(&dev->read_wq);
				wake_up(&dev->write_wq);
<<<<<<< HEAD
				complete(&usb_read_completion);
=======
>>>>>>> target/16.0
			}
			spin_unlock_irqrestore(&dev->lock, flags);

			/* We need to queue a request to read the remaining
			 *  bytes, but we don't actually need to look at
			 * the contents.
			 */
			value = w_length;
		} else if (ctrl->bRequest == MTP_REQ_GET_DEVICE_STATUS
				&& w_index == 0 && w_value == 0) {
			struct mtp_device_status *status = cdev->req->buf;

			status->wLength =
<<<<<<< HEAD
				cpu_to_le16(sizeof(*status));

			DBG(cdev, "MTP_REQ_GET_DEVICE_STATUS\n");
			spin_lock_irqsave(&dev->lock, flags);
			/* device status is "busy" until we report
			 * the cancellation to userspace
=======
				__constant_cpu_to_le16(sizeof(*status));

			mtp_log("MTP_REQ_GET_DEVICE_STATUS\n");
			spin_lock_irqsave(&dev->lock, flags);
			/* device status is "busy" until we report
			 * the cancelation to userspace
>>>>>>> target/16.0
			 */
			if (dev->state == STATE_CANCELED)
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_DEVICE_BUSY);
			else
				status->wCode =
					__cpu_to_le16(MTP_RESPONSE_OK);
			spin_unlock_irqrestore(&dev->lock, flags);
			value = sizeof(*status);
		}
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		int rc;

		cdev->req->zero = value < w_length;
		cdev->req->length = value;
		rc = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (rc < 0)
<<<<<<< HEAD
			ERROR(cdev, "%s: response queue error\n", __func__);
=======
			pr_err("%s: response queue error\n", __func__);
>>>>>>> target/16.0
	}
	return value;
}

static int
mtp_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct mtp_dev	*dev = func_to_mtp(f);
	int			id;
	int			ret;
	struct mtp_instance *fi_mtp;

	dev->cdev = cdev;
<<<<<<< HEAD
	DBG(cdev, "%s dev: %p\n", __func__, dev);
=======
	mtp_log("dev: %pK\n", dev);

	/* ChipIdea controller supports 16K request length for IN endpoint */
	if (cdev->gadget->is_chipidea && mtp_tx_req_len > 16384) {
		mtp_log("Truncating Tx Req length to 16K for ChipIdea\n");
		mtp_tx_req_len = 16384;
	}
>>>>>>> target/16.0

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	mtp_interface_desc.bInterfaceNumber = id;

<<<<<<< HEAD
	ptp_interface_desc.bInterfaceNumber = id;

=======
>>>>>>> target/16.0
	if (mtp_string_defs[INTERFACE_STRING_INDEX].id == 0) {
		ret = usb_string_id(c->cdev);
		if (ret < 0)
			return ret;
		mtp_string_defs[INTERFACE_STRING_INDEX].id = ret;
		mtp_interface_desc.iInterface = ret;
	}

	fi_mtp = container_of(f->fi, struct mtp_instance, func_inst);

	if (cdev->use_os_string) {
		f->os_desc_table = kzalloc(sizeof(*f->os_desc_table),
					GFP_KERNEL);
		if (!f->os_desc_table)
			return -ENOMEM;
		f->os_desc_n = 1;
		f->os_desc_table[0].os_desc = &fi_mtp->mtp_os_desc;
	}

	/* allocate endpoints */
	ret = mtp_create_bulk_endpoints(dev, &mtp_fullspeed_in_desc,
			&mtp_fullspeed_out_desc, &mtp_intr_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		mtp_highspeed_in_desc.bEndpointAddress =
			mtp_fullspeed_in_desc.bEndpointAddress;
		mtp_highspeed_out_desc.bEndpointAddress =
			mtp_fullspeed_out_desc.bEndpointAddress;
	}
	/* support super speed hardware */
	if (gadget_is_superspeed(c->cdev->gadget)) {
<<<<<<< HEAD
		unsigned int max_burst;

		/* Calculate bMaxBurst, we know packet size is 1024 */
		max_burst = min_t(unsigned int,
				MTP_BULK_BUFFER_SIZE / 1024, 15);
=======
		unsigned max_burst;

		/* Calculate bMaxBurst, we know packet size is 1024 */
		max_burst = min_t(unsigned, MTP_BULK_BUFFER_SIZE / 1024, 15);
>>>>>>> target/16.0
		mtp_ss_in_desc.bEndpointAddress =
			mtp_fullspeed_in_desc.bEndpointAddress;
		mtp_ss_in_comp_desc.bMaxBurst = max_burst;
		mtp_ss_out_desc.bEndpointAddress =
			mtp_fullspeed_out_desc.bEndpointAddress;
		mtp_ss_out_comp_desc.bMaxBurst = max_burst;
	}

<<<<<<< HEAD
	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
=======
	fi_mtp->func_inst.f = &dev->function;
	mtp_log("%s speed %s: IN/%s, OUT/%s\n",
>>>>>>> target/16.0
		gadget_is_superspeed(c->cdev->gadget) ? "super" :
		(gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full"),
		f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
mtp_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct mtp_dev	*dev = func_to_mtp(f);
<<<<<<< HEAD
	struct usb_request *req;
	int i;

	pr_info("%s before flush\n", __func__);
	flush_workqueue(dev->wq);
	pr_info("%s after flush\n", __func__);

	mtp_string_defs[INTERFACE_STRING_INDEX].id = 0;
=======
	struct mtp_instance *fi_mtp;
	struct usb_request *req;
	int i;
	fi_mtp = container_of(f->fi, struct mtp_instance, func_inst);
	mtp_string_defs[INTERFACE_STRING_INDEX].id = 0;
	mtp_log("dev: %pK\n", dev);

	mutex_lock(&dev->read_mutex);
>>>>>>> target/16.0
	while ((req = mtp_req_get(dev, &dev->tx_idle)))
		mtp_request_free(req, dev->ep_in);
	for (i = 0; i < RX_REQ_MAX; i++)
		mtp_request_free(dev->rx_req[i], dev->ep_out);
	while ((req = mtp_req_get(dev, &dev->intr_idle)))
		mtp_request_free(req, dev->ep_intr);
<<<<<<< HEAD

=======
>>>>>>> target/16.0
	spin_lock_irq(&dev->lock);
	dev->state = STATE_OFFLINE;
	dev->cdev = NULL;
	spin_unlock_irq(&dev->lock);
<<<<<<< HEAD
	kfree(f->os_desc_table);
	f->os_desc_n = 0;
}

static int mtp_function_set_alt(struct usb_function *f,
		unsigned int intf, unsigned int alt)
=======
	mutex_unlock(&dev->read_mutex);

	kfree(f->os_desc_table);
	f->os_desc_n = 0;
	fi_mtp->func_inst.f = NULL;
}

static int mtp_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
>>>>>>> target/16.0
{
	struct mtp_dev	*dev = func_to_mtp(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

<<<<<<< HEAD
	DBG(cdev, "%s: %d alt: %d\n", __func__, intf, alt);
=======
	mtp_log("%d alt: %d\n", intf, alt);
>>>>>>> target/16.0

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_in);
	if (ret)
		return ret;

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_out);
	if (ret) {
		usb_ep_disable(dev->ep_in);
		return ret;
	}

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_intr);
	if (ret)
		return ret;

	ret = usb_ep_enable(dev->ep_intr);
	if (ret) {
		usb_ep_disable(dev->ep_out);
		usb_ep_disable(dev->ep_in);
		return ret;
	}
	dev->state = STATE_READY;

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	return 0;
}

static void mtp_function_disable(struct usb_function *f)
{
	struct mtp_dev	*dev = func_to_mtp(f);
<<<<<<< HEAD
	struct usb_composite_dev	*cdev = dev->cdev;

	DBG(cdev, "%s\n", __func__);
	dev->state = STATE_OFFLINE;
=======

	mtp_log("\n");
	spin_lock_irq(&dev->lock);
	dev->state = STATE_OFFLINE;
	spin_unlock_irq(&dev->lock);
>>>>>>> target/16.0
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);
	usb_ep_disable(dev->ep_intr);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

<<<<<<< HEAD
	VDBG(cdev, "%s disabled\n", dev->function.name);
=======
	mtp_log("%s disabled\n", dev->function.name);
}

static int debug_mtp_read_stats(struct seq_file *s, void *unused)
{
	struct mtp_dev *dev = _mtp_dev;
	int i;
	unsigned long flags;
	unsigned int min, max = 0, sum = 0, iteration = 0;

	seq_puts(s, "\n=======================\n");
	seq_puts(s, "MTP Write Stats:\n");
	seq_puts(s, "\n=======================\n");
	spin_lock_irqsave(&dev->lock, flags);
	min = dev->perf[0].vfs_wtime;
	for (i = 0; i < MAX_ITERATION; i++) {
		seq_printf(s, "vfs write: bytes:%ld\t\t time:%d\n",
				dev->perf[i].vfs_wbytes,
				dev->perf[i].vfs_wtime);
		if (dev->perf[i].vfs_wbytes == mtp_rx_req_len) {
			sum += dev->perf[i].vfs_wtime;
			if (min > dev->perf[i].vfs_wtime)
				min = dev->perf[i].vfs_wtime;
			if (max < dev->perf[i].vfs_wtime)
				max = dev->perf[i].vfs_wtime;
			iteration++;
		}
	}

	seq_printf(s, "vfs_write(time in usec) min:%d\t max:%d\t avg:%d\n",
				min, max, (iteration ? (sum / iteration) : 0));
	min = max = sum = iteration = 0;
	seq_puts(s, "\n=======================\n");
	seq_puts(s, "MTP Read Stats:\n");
	seq_puts(s, "\n=======================\n");

	min = dev->perf[0].vfs_rtime;
	for (i = 0; i < MAX_ITERATION; i++) {
		seq_printf(s, "vfs read: bytes:%ld\t\t time:%d\n",
				dev->perf[i].vfs_rbytes,
				dev->perf[i].vfs_rtime);
		if (dev->perf[i].vfs_rbytes == mtp_tx_req_len) {
			sum += dev->perf[i].vfs_rtime;
			if (min > dev->perf[i].vfs_rtime)
				min = dev->perf[i].vfs_rtime;
			if (max < dev->perf[i].vfs_rtime)
				max = dev->perf[i].vfs_rtime;
			iteration++;
		}
	}

	seq_printf(s, "vfs_read(time in usec) min:%d\t max:%d\t avg:%d\n",
				min, max, (iteration ? (sum / iteration) : 0));
	spin_unlock_irqrestore(&dev->lock, flags);
	return 0;
}

static ssize_t debug_mtp_reset_stats(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	int clear_stats;
	unsigned long flags;
	struct mtp_dev *dev = _mtp_dev;

	if (buf == NULL) {
		pr_err("[%s] EINVAL\n", __func__);
		goto done;
	}

	if (kstrtoint(buf, 0, &clear_stats) || clear_stats != 0) {
		pr_err("Wrong value. To clear stats, enter value as 0.\n");
		goto done;
	}

	spin_lock_irqsave(&dev->lock, flags);
	memset(&dev->perf[0], 0, MAX_ITERATION * sizeof(dev->perf[0]));
	dev->dbg_read_index = 0;
	dev->dbg_write_index = 0;
	spin_unlock_irqrestore(&dev->lock, flags);
done:
	return count;
}

static int debug_mtp_open(struct inode *inode, struct file *file)
{
	return single_open(file, debug_mtp_read_stats, inode->i_private);
}

static const struct file_operations debug_mtp_ops = {
	.open = debug_mtp_open,
	.read = seq_read,
	.write = debug_mtp_reset_stats,
};

struct dentry *dent_mtp;
static void mtp_debugfs_init(void)
{
	struct dentry *dent_mtp_status;

	dent_mtp = debugfs_create_dir("usb_mtp", 0);
	if (!dent_mtp || IS_ERR(dent_mtp))
		return;

	dent_mtp_status = debugfs_create_file("status", 0644,
					dent_mtp, 0, &debug_mtp_ops);
	if (!dent_mtp_status || IS_ERR(dent_mtp_status)) {
		debugfs_remove(dent_mtp);
		dent_mtp = NULL;
		return;
	}
}

static void mtp_debugfs_remove(void)
{
	debugfs_remove_recursive(dent_mtp);
>>>>>>> target/16.0
}

static int __mtp_setup(struct mtp_instance *fi_mtp)
{
	struct mtp_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);

	if (fi_mtp != NULL)
		fi_mtp->dev = dev;

	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->lock);
	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);
	init_waitqueue_head(&dev->intr_wq);
	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->ioctl_excl, 0);
	INIT_LIST_HEAD(&dev->tx_idle);
	INIT_LIST_HEAD(&dev->intr_idle);

<<<<<<< HEAD
	init_completion(&usb_read_completion);

=======
>>>>>>> target/16.0
	dev->wq = create_singlethread_workqueue("f_mtp");
	if (!dev->wq) {
		ret = -ENOMEM;
		goto err1;
	}
	INIT_WORK(&dev->send_file_work, send_file_work);
	INIT_WORK(&dev->receive_file_work, receive_file_work);

<<<<<<< HEAD
	dev->is_boost = 0;
	cpumask_setall(&(dev->cpu_mask));

=======
>>>>>>> target/16.0
	_mtp_dev = dev;

	ret = misc_register(&mtp_device);
	if (ret)
		goto err2;

<<<<<<< HEAD
=======
	mtp_debugfs_init();
>>>>>>> target/16.0
	return 0;

err2:
	destroy_workqueue(dev->wq);
err1:
	_mtp_dev = NULL;
	kfree(dev);
<<<<<<< HEAD
	pr_info("mtp gadget driver failed to initialize\n");
=======
	printk(KERN_ERR "mtp gadget driver failed to initialize\n");
>>>>>>> target/16.0
	return ret;
}

static int mtp_setup_configfs(struct mtp_instance *fi_mtp)
{
	return __mtp_setup(fi_mtp);
}


static void mtp_cleanup(void)
{
	struct mtp_dev *dev = _mtp_dev;

	if (!dev)
		return;

<<<<<<< HEAD
=======
	mtp_debugfs_remove();
>>>>>>> target/16.0
	misc_deregister(&mtp_device);
	destroy_workqueue(dev->wq);
	_mtp_dev = NULL;
	kfree(dev);
}

static struct mtp_instance *to_mtp_instance(struct config_item *item)
{
	return container_of(to_config_group(item), struct mtp_instance,
		func_inst.group);
}

static void mtp_attr_release(struct config_item *item)
{
	struct mtp_instance *fi_mtp = to_mtp_instance(item);

	usb_put_function_instance(&fi_mtp->func_inst);
}

static struct configfs_item_operations mtp_item_ops = {
	.release        = mtp_attr_release,
};

static struct config_item_type mtp_func_type = {
	.ct_item_ops    = &mtp_item_ops,
	.ct_owner       = THIS_MODULE,
};

<<<<<<< HEAD
#ifdef CONFIG_USB_CONFIGFS_UEVENT
static int cpumask_to_int(const struct cpumask *cpu_mask)
{
	int mask = 0;
	int cpu;

	for_each_cpu(cpu, cpu_mask) {
		pr_debug("[USB]%d\n", cpu);
		mask |= (1 << cpu);
	}

	return mask;
}

static ssize_t cpu_mask_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cpumask *cpu_mask = mtp_get_cpu_mask();

	return sprintf(buf, "0x%X\n",
		(cpu_mask?cpumask_to_int(cpu_mask):0xFFFFFFFF));
}

static ssize_t cpu_mask_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned int mask;

	if (kstrtouint(buf, 16, &mask) != 0)
		return -EINVAL;

	pr_info("Store => 0x%x\n", mask);

	mtp_set_cpu_mask(mask);

	return size;
}

static DEVICE_ATTR_RW(cpu_mask);

static ssize_t mtp_server_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", mtp_get_mtp_server());
}

static DEVICE_ATTR_RO(mtp_server);

static struct device_attribute *mtp_function_attributes[] = {
	&dev_attr_cpu_mask,
	&dev_attr_mtp_server,
	NULL
};
#endif
=======
>>>>>>> target/16.0

static struct mtp_instance *to_fi_mtp(struct usb_function_instance *fi)
{
	return container_of(fi, struct mtp_instance, func_inst);
}

static int mtp_set_inst_name(struct usb_function_instance *fi, const char *name)
{
	struct mtp_instance *fi_mtp;
	char *ptr;
	int name_len;

	name_len = strlen(name) + 1;
	if (name_len > MAX_INST_NAME_LEN)
		return -ENAMETOOLONG;

	ptr = kstrndup(name, name_len, GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	fi_mtp = to_fi_mtp(fi);
	fi_mtp->name = ptr;

	return 0;
}

static void mtp_free_inst(struct usb_function_instance *fi)
{
	struct mtp_instance *fi_mtp;

	fi_mtp = to_fi_mtp(fi);
	kfree(fi_mtp->name);
<<<<<<< HEAD
#ifdef CONFIG_USB_CONFIGFS_UEVENT
	device_destroy(fi_mtp->mtp_device->class,
			fi_mtp->mtp_device->devt);
#endif
	mtp_cleanup();
	//kfree(fi_mtp->mtp_os_desc.group.default_groups);
	kfree(fi_mtp);
}
#ifdef CONFIG_USB_CONFIGFS_UEVENT
extern struct device *create_function_device(char *name);
#endif
=======
	mtp_cleanup();
	kfree(fi_mtp);
}

>>>>>>> target/16.0
struct usb_function_instance *alloc_inst_mtp_ptp(bool mtp_config)
{
	struct mtp_instance *fi_mtp;
	int ret = 0;
	struct usb_os_desc *descs[1];
	char *names[1];
<<<<<<< HEAD
#ifdef CONFIG_USB_CONFIGFS_UEVENT
	struct device_attribute **attrs;
	struct device_attribute *attr;
	struct device *dev;
	int err = 0;
#endif
=======
>>>>>>> target/16.0

	fi_mtp = kzalloc(sizeof(*fi_mtp), GFP_KERNEL);
	if (!fi_mtp)
		return ERR_PTR(-ENOMEM);
	fi_mtp->func_inst.set_inst_name = mtp_set_inst_name;
	fi_mtp->func_inst.free_func_inst = mtp_free_inst;

	fi_mtp->mtp_os_desc.ext_compat_id = fi_mtp->mtp_ext_compat_id;
	INIT_LIST_HEAD(&fi_mtp->mtp_os_desc.ext_prop);
	descs[0] = &fi_mtp->mtp_os_desc;
	names[0] = "MTP";

	if (mtp_config) {
		ret = mtp_setup_configfs(fi_mtp);
		if (ret) {
			kfree(fi_mtp);
<<<<<<< HEAD
			pr_info("Error setting MTP\n");
=======
			pr_err("Error setting MTP\n");
>>>>>>> target/16.0
			return ERR_PTR(ret);
		}
	} else
		fi_mtp->dev = _mtp_dev;

	config_group_init_type_name(&fi_mtp->func_inst.group,
					"", &mtp_func_type);
<<<<<<< HEAD

	usb_os_desc_prepare_interf_dir(&fi_mtp->func_inst.group, 1,
					descs, names, THIS_MODULE);
#ifdef CONFIG_USB_CONFIGFS_UEVENT
	if (mtp_config) {
		dev = create_function_device("f_mtp");

		if (IS_ERR(dev)) {
			kfree(fi_mtp);
			pr_info("Error create_function_device\n");
			return (void *)dev;
		}

		fi_mtp->mtp_device = dev;

		attrs = mtp_function_attributes;
		if (attrs) {
			while ((attr = *attrs++) && !err)
				err = device_create_file(dev, attr);
			if (err) {
				device_destroy(dev->class, dev->devt);
				kfree(fi_mtp);
				pr_info("Error device_create_file\n");
				return ERR_PTR(-EINVAL);
			}
		}
	}
#endif
=======
	usb_os_desc_prepare_interf_dir(&fi_mtp->func_inst.group, 1,
					descs, names, THIS_MODULE);

	mutex_init(&fi_mtp->dev->read_mutex);
>>>>>>> target/16.0

	return  &fi_mtp->func_inst;
}
EXPORT_SYMBOL_GPL(alloc_inst_mtp_ptp);

static struct usb_function_instance *mtp_alloc_inst(void)
{
		return alloc_inst_mtp_ptp(true);
}

static int mtp_ctrlreq_configfs(struct usb_function *f,
				const struct usb_ctrlrequest *ctrl)
{
	return mtp_ctrlrequest(f->config->cdev, ctrl);
}

static void mtp_free(struct usb_function *f)
{
	/*NO-OP: no function specific resource allocation in mtp_alloc*/
}

struct usb_function *function_alloc_mtp_ptp(struct usb_function_instance *fi,
					bool mtp_config)
{
	struct mtp_instance *fi_mtp = to_fi_mtp(fi);
	struct mtp_dev *dev;

	/*
	 * PTP piggybacks on MTP function so make sure we have
	 * created MTP function before we associate this PTP
	 * function with a gadget configuration.
	 */
	if (fi_mtp->dev == NULL) {
<<<<<<< HEAD
		pr_info("fi_mtp->dev == NULL\n");
=======
		pr_err("Error: Create MTP function before linking"
				" PTP function with a gadget configuration\n");
		pr_err("\t1: Delete existing PTP function if any\n");
		pr_err("\t2: Create MTP function\n");
		pr_err("\t3: Create and symlink PTP function"
				" with a gadget configuration\n");
>>>>>>> target/16.0
		return ERR_PTR(-EINVAL); /* Invalid Configuration */
	}

	dev = fi_mtp->dev;
	dev->function.name = DRIVER_NAME;
	dev->function.strings = mtp_strings;
	if (mtp_config) {
		dev->function.fs_descriptors = fs_mtp_descs;
		dev->function.hs_descriptors = hs_mtp_descs;
		dev->function.ss_descriptors = ss_mtp_descs;
<<<<<<< HEAD
=======
		dev->function.ssp_descriptors = ss_mtp_descs;
>>>>>>> target/16.0
	} else {
		dev->function.fs_descriptors = fs_ptp_descs;
		dev->function.hs_descriptors = hs_ptp_descs;
		dev->function.ss_descriptors = ss_ptp_descs;
<<<<<<< HEAD
=======
		dev->function.ssp_descriptors = ss_ptp_descs;
>>>>>>> target/16.0
	}
	dev->function.bind = mtp_function_bind;
	dev->function.unbind = mtp_function_unbind;
	dev->function.set_alt = mtp_function_set_alt;
	dev->function.disable = mtp_function_disable;
	dev->function.setup = mtp_ctrlreq_configfs;
	dev->function.free_func = mtp_free;
<<<<<<< HEAD
=======
	fi->f = &dev->function;
>>>>>>> target/16.0

	return &dev->function;
}
EXPORT_SYMBOL_GPL(function_alloc_mtp_ptp);

static struct usb_function *mtp_alloc(struct usb_function_instance *fi)
{
	return function_alloc_mtp_ptp(fi, true);
}

<<<<<<< HEAD
DECLARE_USB_FUNCTION_INIT(mtp, mtp_alloc_inst, mtp_alloc);
MODULE_LICENSE("GPL");
=======
DECLARE_USB_FUNCTION(mtp, mtp_alloc_inst, mtp_alloc);

static int mtp_init(void)
{
	_mtp_ipc_log = ipc_log_context_create(NUM_PAGES, "usb_mtp", 0);
	if (IS_ERR_OR_NULL(_mtp_ipc_log))
		_mtp_ipc_log =  NULL;

	return usb_function_register(&mtpusb_func);
}
module_init(mtp_init);

static void __exit mtp_exit(void)
{
	ipc_log_context_destroy(_mtp_ipc_log);
	usb_function_unregister(&mtpusb_func);
}
module_exit(mtp_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTP function driver");
>>>>>>> target/16.0
