#pragma once

#include <zephyr/bluetooth/uuid.h>

/* GATT service on the peripheral (right half) that receives udo packets. */
#define UDO_SVC_UUID_VAL BT_UUID_128_ENCODE(0x26931a33, 0x2592, 0x4c15, 0xa55b, 0xe029354b8347)
#define UDO_CHR_UUID_VAL BT_UUID_128_ENCODE(0xf2d74a4b, 0x73b0, 0x4dae, 0x91e3, 0x27ecbb34c1b5)

/* Compound literals: fine at file scope (static storage), but inside a function
 * they die on return. Use a static bt_uuid_128 for anything Zephyr keeps
 * (e.g. bt_gatt_discover_params.uuid). */
#define UDO_SVC_UUID BT_UUID_DECLARE_128(UDO_SVC_UUID_VAL)
#define UDO_CHR_UUID BT_UUID_DECLARE_128(UDO_CHR_UUID_VAL)
