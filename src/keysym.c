/*
 * keysym.c - This file is part of mxim
 * Copyright (C) 2024 Matthias Kruk
 *
 * Mxim is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3, or (at your
 * option) any later version.
 *
 * Mxim is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Mxim; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "keysym.h"
#include <errno.h>
#include <X11/X.h>

static const keycode_t _keymap[] = {
	[0]   = KEY_INVALID,

	[9]   = KEY_ESC,
	[10]  = KEY_1,
	[11]  = KEY_2,
	[12]  = KEY_3,
	[13]  = KEY_4,
	[14]  = KEY_5,
	[15]  = KEY_6,
	[16]  = KEY_7,
	[17]  = KEY_8,
	[18]  = KEY_9,
	[19]  = KEY_0,
	[20]  = KEY_MINUS,
	[21]  = KEY_CIRCUMFLEX,
	[22]  = KEY_BACKSPACE,
	[23]  = KEY_TAB,
	[24]  = KEY_Q,
	[25]  = KEY_W,
	[26]  = KEY_E,
	[27]  = KEY_R,
	[28]  = KEY_T,
	[29]  = KEY_Y,
	[30]  = KEY_U,
	[31]  = KEY_I,
	[32]  = KEY_O,
	[33]  = KEY_P,
	[34]  = KEY_AT,
	[35]  = KEY_LBRACKET,
	[36]  = KEY_RETURN,
	[37]  = KEY_LCTRL,
	[38]  = KEY_A,
	[39]  = KEY_S,
	[40]  = KEY_D,
	[41]  = KEY_F,
	[42]  = KEY_G,
	[43]  = KEY_H,
	[44]  = KEY_J,
	[45]  = KEY_K,
	[46]  = KEY_L,
	[47]  = KEY_SEMICOLON,
	[48]  = KEY_COLON,
	[49]  = KEY_ZENKAKU,
	[50]  = KEY_LSHIFT,
	[51]  = KEY_RBRACKET,
	[52]  = KEY_Z,
	[53]  = KEY_X,
	[54]  = KEY_C,
	[55]  = KEY_V,
	[56]  = KEY_B,
	[57]  = KEY_N,
	[58]  = KEY_M,
	[59]  = KEY_COMMA,
	[60]  = KEY_PERIOD,
	[61]  = KEY_SLASH,
	[62]  = KEY_RSHIFT,

	[64]  = KEY_LALT,
	[65]  = KEY_SPACE,
	[66]  = KEY_CAPSLOCK,
	[67]  = KEY_F1,
	[68]  = KEY_F2,
	[69]  = KEY_F3,
	[70]  = KEY_F4,
	[71]  = KEY_F5,
	[72]  = KEY_F6,
	[73]  = KEY_F7,
	[74]  = KEY_F8,
	[75]  = KEY_F9,
	[76]  = KEY_F10,

	[78]  = KEY_SCRLK,

	[95]  = KEY_F11,
	[96]  = KEY_F12,
	[97]  = KEY_BACKSLASH,

	[100] = KEY_HENKAN,
	[101] = KEY_KANA,
	[102] = KEY_MUHENKAN,

	[105] = KEY_RCTRL,

	[108] = KEY_RALT,

	[110] = KEY_HOME,
	[111] = KEY_UP,
	[112] = KEY_PAGEUP,
	[113] = KEY_LEFT,
	[114] = KEY_RIGHT,
	[115] = KEY_END,
	[116] = KEY_DOWN,
	[117] = KEY_PAGEDOWN,
	[118] = KEY_INSERT,
	[119] = KEY_DELETE,

	[121] = KEY_MUTE,
	[122] = KEY_VOLDOWN,
	[123] = KEY_VOLUP,

	[127] = KEY_PAUSE,

	[132] = KEY_YEN,
	[133] = KEY_SUPER,

	[135] = KEY_MENU,

	[150] = KEY_SLEEP,
	[151] = KEY_FN,

	[160] = KEY_LOCK,

	[166] = KEY_PREV,
	[167] = KEY_NEXT,

	[171] = KEY_MEDIA_FORWARD,
	[172] = KEY_MEDIA_PAUSE,
	[173] = KEY_MEDIA_REVERSE,
	[174] = KEY_MEDIA_STOP,

	[213] = KEY_SUSPEND,

	[220] = KEY_HEADPHONE,

	[232] = KEY_BRIGHTNESS_DOWN,
	[233] = KEY_BRIGHTNESS_UP,

	[235] = KEY_MONITOR,

	[244] = KEY_BATTERY,

	[246] = KEY_RFLOCK,

	[255] = KEY_INVALID
};

static keycode_t keycode_from_detail(const int detail)
{
	int idx;

	if (detail < KEY_ESC || detail >= 255) {
		idx = 255;
	} else {
		idx = detail;
	}

	return _keymap[idx];
}

static modmask_t modmask_from_state(const int state)
{
	modmask_t mask;

	mask = 0;

	if (state & ShiftMask) {
		mask |= MOD_SHIFT;
	}
	if (state & ControlMask) {
		mask |= MOD_CTRL;
	}
	if (state & Mod1Mask) {
		mask |= MOD_ALT;
	}
	if (state & Mod4Mask) {
		mask |= MOD_SUPER;
	}

	return mask;
}

int keysym_from_event(keysym_t *keysym, struct XCoreKeyEvent *event)
{
	keycode_t key;

	if ((key = keycode_from_detail(event->detail)) == KEY_INVALID) {
		return -EBADMSG;
	}

	keysym->key = key;
	keysym->mod = modmask_from_state(event->state);

	return 0;
}
