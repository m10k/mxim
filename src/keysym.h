/*
 * keysym.h - This file is part of mxim
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

#ifndef KEYSYM_H
#define KEYSYM_H

#include "ximproto.h"

typedef enum {
	KEY_INVALID = 0,
	KEY_ESC,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_LALT,
	KEY_RALT,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LCTRL,
	KEY_RCTRL,
	KEY_SUPER,
	KEY_CAPSLOCK,
	KEY_TAB,
	KEY_ZENKAKU,
	KEY_KANA,
	KEY_HENKAN,
	KEY_MUHENKAN,
	KEY_INSERT,
	KEY_DELETE,
	KEY_BACKSPACE,
	KEY_RETURN,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_HOME,
	KEY_END,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_MINUS,
	KEY_COLON,
	KEY_SEMICOLON,
	KEY_CIRCUMFLEX,
	KEY_YEN,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_AT,
	KEY_SPACE,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_MUTE,
	KEY_VOLUP,
	KEY_VOLDOWN,
	KEY_PAUSE,
	KEY_SLEEP,
	KEY_FN,
	KEY_SUSPEND,
	KEY_HEADPHONE,
	KEY_BRIGHTNESS_UP,
	KEY_BRIGHTNESS_DOWN,
	KEY_MONITOR,
	KEY_BATTERY,
	KEY_RFLOCK,
	KEY_MEDIA_FORWARD,
	KEY_MEDIA_PAUSE,
	KEY_MEDIA_REVERSE,
	KEY_MEDIA_STOP,
	KEY_SCRLK,
	KEY_MENU,
	KEY_PREV,
	KEY_NEXT,
	KEY_LOCK
} keycode_t;

typedef enum {
	MOD_NONE  = 0,
	MOD_SHIFT = (1 << 0),
	MOD_CTRL  = (1 << 1),
	MOD_ALT   = (1 << 2),
	MOD_SUPER = (1 << 3)
} modmask_t;

typedef struct {
	keycode_t key;
	modmask_t mod;
} keysym_t;

int keysym_from_event(keysym_t *keysym, struct XCoreKeyEvent *event);

#endif /* KEYSYM_H */
