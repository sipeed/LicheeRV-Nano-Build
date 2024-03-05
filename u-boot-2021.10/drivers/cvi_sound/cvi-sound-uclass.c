// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 bitmain
 */

#include <dm/uclass.h>
#include <sound.h>

UCLASS_DRIVER(sound) = {
	.id		= UCLASS_SOUND,
	.name		= "sound",
	.per_device_auto	= sizeof(struct sound_uc_priv),
};
