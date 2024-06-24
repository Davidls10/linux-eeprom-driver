#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0x6870aba1, "i2c_unregister_device" },
	{ 0xd59c188f, "i2c_del_driver" },
	{ 0x719441dd, "i2c_transfer" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x8b7dd98f, "i2c_get_adapter" },
	{ 0x7672df9f, "i2c_new_client_device" },
	{ 0xbd341b00, "i2c_register_driver" },
	{ 0x5615cc33, "i2c_put_adapter" },
	{ 0xdcb764ad, "memset" },
	{ 0xbeb6cb13, "i2c_transfer_buffer_flags" },
	{ 0x3a3b9f5d, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("i2c:AT24C256");

MODULE_INFO(srcversion, "E530C72387C9486B5CB05DC");
