/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2012 Sébastien Hinderer <Sebastien.Hinderer@ens-lyon.org>
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_VO_BRLDEFS
#define BRLTTY_INCLUDED_VO_BRLDEFS

typedef enum {
  /* The top round keys behind the routing keys, named as if they
   * are to be used for braille input.
   */
  VO_KEY_Dot1 = 0,
  VO_KEY_Dot2 = 1,
  VO_KEY_Dot3 = 2,
  VO_KEY_Dot4 = 3,
  VO_KEY_Dot5 = 4,
  VO_KEY_Dot6 = 5,
  VO_KEY_Dot7 = 6,
  VO_KEY_Dot8 = 7,

  /* The front keys */
  VO_KEY_Thumb1 =  8, /* Leftmost */
  VO_KEY_Thumb2 =  9, /* Second from left */
  VO_KEY_Left   = 10, /* Round key to the left of the central pad */
  VO_KEY_Up     = 11, /* Up position of central pad */
  VO_KEY_Down   = 12, /* Down position of central pad */
  VO_KEY_Right  = 13, /* Round key to the right of the central pad */
  VO_KEY_Thumb3 = 14, /* Second from right */
  VO_KEY_Thumb4 = 15  /* Rightmost */
} VO_NavigationKey;

typedef enum {
  BP_KEY_Dot1    = 0,
  BP_KEY_Dot2    = 1,
  BP_KEY_Dot3    = 2,
  BP_KEY_Dot4    = 3,
  BP_KEY_Dot5    = 4,
  BP_KEY_Dot6    = 5,
  BP_KEY_Shift   = 6,
  BP_KEY_Control = 7,

  BP_KEY_ScrollLeft     =  8,
  BP_KEY_JoystickEnter  =  9,
  BP_KEY_JoystickLeft   = 10,
  BP_KEY_JoystickUp     = 11,
  BP_KEY_JoystickDown   = 12,
  BP_KEY_JoystickRight  = 13,
  BP_KEY_Space          = 14,
  BP_KEY_ScrollRight    = 15
} BP_NavigationKey;

typedef enum {
  VO_SET_NavigationKeys = 0,
  VO_SET_RoutingKeys
} VO_KeySet;

#endif /* BRLTTY_INCLUDED_VO_BRLDEFS */ 
