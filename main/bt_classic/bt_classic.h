#ifndef __BT_CLASSIC_HEADER__
#define __BT_CLASSIC_HEADER__

bool bt_classic_init();
char *bda2str(esp_bd_addr_t bda, char *str, size_t size);

#endif