/*
 * Copyright (C) 2009 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/** @file
 *
 * Login UI
 *
 */

#include <string.h>
#include <errno.h>
#include <curses.h>
#include <ipxe/console.h>
#include <ipxe/settings.h>
#include <ipxe/editbox.h>
#include <ipxe/keys.h>
#include <ipxe/ansicol.h>
#include <ipxe/login_ui.h>

#if 1
#include <config/colour.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <byteswap.h>
#include <ipxe/timer.h>
#include <ipxe/process.h>
#include <usr/autoboot.h>
/* Colour pairs */
#define CPAIR_NORMAL		1
#define CPAIR_EDIT_NEW		2
#endif

/* Screen layout */
#define USERNAME_LABEL_ROW	( ( LINES / 2U ) - 4U )
#define USERNAME_ROW		( ( LINES / 2U ) - 2U )
#define PASSWORD_LABEL_ROW	( ( LINES / 2U ) + 2U )
#define PASSWORD_ROW		( ( LINES / 2U ) + 4U )
#define LABEL_COL		( ( COLS / 2U ) - 4U )
#define EDITBOX_COL		( ( COLS / 2U ) - 10U )
#define EDITBOX_WIDTH		20U

#if 1
#define CPAIR_SELECT	2
/** A PXE boot menu item */
struct multiboot_menu_item {
	/** Description */
	char desc[32];
};

/**
 * A PXE boot menu
 *
 * This structure encapsulates the menu information provided via DHCP
 * options.
 */
struct multiboot_menu {
	/** Prompt string (optional) */
	char prompt[32];
	/** Timeout (in seconds)
	 *
	 * Negative indicates no timeout (i.e. wait indefinitely)
	 */
	int timeout;
	/** Number of menu items */
	unsigned int num_items;
	/** Selected menu item */
	unsigned int selection;
	/** Menu items */
	struct multiboot_menu_item items[8];
};


/**
 * Draw PXE boot menu item
 *
 * @v menu		PXE boot menu
 * @v index		Index of item to draw
 * @v selected		Item is selected
 */
static void multiboot_menu_draw_item ( struct multiboot_menu *menu,
				 unsigned int index, int selected ) {
	char buf[COLS+1];
	size_t len;
	unsigned int row;

	/* Prepare space-padded row content */
	len = snprintf ( buf, sizeof ( buf ), " %c. %s",
			 ( 'A' + index ), menu->items[index].desc );
	while ( len < ( sizeof ( buf ) - 1 ) )
		buf[len++] = ' ';
	buf[ sizeof ( buf ) - 1 ] = '\0';

	/* Draw row */
	row = ( LINES - menu->num_items + index - 1);
	color_set ( ( selected ? CPAIR_SELECT : CPAIR_NORMAL ), NULL );
	mvprintw ( row, 0, "%s", buf );
	move ( row, 1 );
}

/**
 * Make selection from PXE boot menu
 *
 * @v menu		PXE boot menu
 * @ret rc		Return status code
 */
static int multiboot_menu_select ( struct multiboot_menu *menu ) {
	int key;
	unsigned int key_selection;
	unsigned int i;
	int rc = 0;
	
	unsigned long start = currticks();
	unsigned long now;
	unsigned long elapsed;
	size_t len = 0;
	
	/* Initialise UI */
	initscr();
	start_color();
	init_pair ( CPAIR_NORMAL, COLOR_WHITE, COLOR_BLACK );
	init_pair ( CPAIR_SELECT, COLOR_BLACK, COLOR_WHITE );
	color_set ( CPAIR_NORMAL, NULL );

	/* Draw initial menu */
	for ( i = 0 ; i < menu->num_items ; i++ )
		printf ( "\n" );

	printf ("\n\n");

	/* Display prompt, if specified */
	move ( LINES - menu->num_items - 3, 0 );
	if ( menu->prompt )
		printf ( "%s", menu->prompt );

	for ( i = 0 ; i < menu->num_items - 1; i++ )
		multiboot_menu_draw_item ( menu, ( menu->num_items - i - 1 ), 0 );

	 multiboot_menu_draw_item ( menu, ( menu->num_items - i - 1 ), 1 );

	/* Wait for timeout, if specified */
	while ( menu->timeout > 0 ) {
		if ( ! len )
		{
			move ( LINES - menu->num_items - 1, strlen(menu->items[0].desc) + 4);
			len = printf ( " (%d)", menu->timeout );
			move ( LINES - menu->num_items - 1, 1);
		}
		if ( iskey() ) {
			move ( LINES - menu->num_items - 1, strlen(menu->items[0].desc) + 4 + len);
			do {
				printf ( "\b \b" );
			} while ( --len );
			goto _select_menu;
		}
		now = currticks();
		elapsed = ( now - start );
		if ( elapsed >= TICKS_PER_SEC ) {
			menu->timeout -= 1;
			move ( LINES - menu->num_items - 1, strlen(menu->items[0].desc) + 4 + len);
			do {
				printf ( "\b \b" );
			} while ( --len );
			len = 0;
			start = now;
		}
	}

	endwin();

	return rc;
	
_select_menu:

	while ( 1 ) {

		/* Highlight currently selected item */
		multiboot_menu_draw_item ( menu, menu->selection, 1 );

		/* Wait for keyboard input */
		while ( ! iskey() )
			step();
		key = getkey( 0 );

		/* Unhighlight currently selected item */
		multiboot_menu_draw_item ( menu, menu->selection, 0 );

		/* Act upon key */
		if ( ( key == CR ) || ( key == LF ) ) {
			multiboot_menu_draw_item ( menu, menu->selection, 1 );
			break;
		} /*else if ( ( key == CTRL_C ) || ( key == ESC ) ) {
			rc = -ECANCELED;
			break;
		}*/ else if ( key == KEY_UP ) {
			if ( menu->selection > 0 )
				menu->selection--;
		} else if ( key == KEY_DOWN ) {
			if ( menu->selection < ( menu->num_items - 1 ) )
				menu->selection++;
		} else if ( ( key < KEY_MIN ) &&
			    ( ( key_selection = ( toupper ( key ) - 'A' ) )
			      < menu->num_items ) ) {
			menu->selection = key_selection;
			multiboot_menu_draw_item ( menu, menu->selection, 1 );
			break;
		}
	}

	/* Shut down UI */
	endwin();

	return rc;
}

/**
 * Prompt for (and make selection from) PXE boot menu
 *
 * @v menu		PXE boot menu
 * @ret rc		Return status code
 */
static int multiboot_menu_prompt_and_select ( struct multiboot_menu *menu ) {
	unsigned long start = currticks();
	unsigned long now;
	unsigned long elapsed;
	size_t len = 0;
	int key;
	int rc = 0;

	/* Display menu immediately, if specified to do so */
	if ( menu->timeout < 0 ) {
		if ( menu->prompt )
			printf ( "%s\n", menu->prompt );
		return multiboot_menu_select ( menu );
	}

	if ( menu->timeout > 0)
	{
		return multiboot_menu_select (menu);
	}

	/* Display prompt, if specified */
	if ( menu->prompt )
		printf ( "%s", menu->prompt );

	/* Wait for timeout, if specified */
	while ( menu->timeout > 0 ) {
		if ( ! len )
			len = printf ( " (%d)", menu->timeout );
		if ( iskey() ) {
			key = getkey( 0 );
			if ( key == KEY_F8 ) {
				/* Display menu */
				printf ( "\n" );
				return multiboot_menu_select ( menu );
			} else if ( ( key == CTRL_C ) || ( key == ESC ) ) {
				/* Abort */
				rc = -ECANCELED;
				break;
			} else {
				/* Stop waiting */
				break;
			}
		}
		now = currticks();
		elapsed = ( now - start );
		if ( elapsed >= TICKS_PER_SEC ) {
			menu->timeout -= 1;
			do {
				printf ( "\b \b" );
			} while ( --len );
			start = now;
		}
	}

	/* Return with default option selected */
	printf ( "\n" );
	return rc;
}

struct setting ccboot_multi_boot_timeout __setting ( SETTING_MISC, boot-timeout )= {
	.name = "ccboot-multi-boot-timeout",
	.description = "CCBoot multi boot timeout",
	.tag = 253,
	.type = &setting_type_uint8,
};

int multiboot(const char * targetname, const char * desc)
{
	// iscsi:192.168.1.201::3260::iqn.2008-12.com.ccboot.211:2
	// Image 01;Image 02;
	struct multiboot_menu menu;
	int rc;
	const char * find_number;
	int i = 0;
	char rootpath[256];

	memset(&menu, 0, sizeof(menu));

	menu.timeout = fetch_uintz_setting ( NULL, &ccboot_multi_boot_timeout);

	if(menu.timeout == 0)
		menu.timeout = 3;

	snprintf(menu.prompt, sizeof(menu.prompt), "%s", "CCBoot Multiple Boot System");

	find_number = strrchr(targetname, ':');
	if(find_number == NULL)
		return -1;

	find_number ++;

	menu.num_items = find_number[0] - '0';

	while(i < 8)
	{
		const char * p = strstr(desc, ";");
		if(p == NULL)
			break;
		strncpy(menu.items[i].desc, desc, ((long unsigned int)(p - desc)) > (sizeof(menu.items[i].desc) - 1) ? (sizeof(menu.items[i].desc) - 1) : ((long unsigned int)(p - desc)));
		i ++;
		desc = p + 1;
	}

	/* Make selection from boot menu */
	if ( ( rc = multiboot_menu_prompt_and_select ( &menu ) ) != 0 ) {
		return rc;
	}

	snprintf(rootpath, sizeof(rootpath), "%s", targetname);
	rootpath[find_number - targetname] = 0;
	snprintf(rootpath, sizeof(rootpath), "%s%03d", rootpath, menu.selection);
	
	store_setting ( NULL, &root_path_setting, rootpath,  strlen (rootpath) );
	//printf("===%s===\n", rootpath);
	return 0;
}
#endif

int login_ui ( void ) {
	char username[64];
	char password[64];
#if 1
	char buffer[128];
	char hostname[64];
	char * param;
#endif
	struct edit_box username_box;
	struct edit_box password_box;
	struct edit_box *current_box = &username_box;
	int key;
	int rc = -EINPROGRESS;
#if 1
	username[0] = 0;
#endif	
	/* Fetch current setting values */
	fetch_string_setting ( NULL, &username_setting, username,
			       sizeof ( username ) );
	fetch_string_setting ( NULL, &password_setting, password,
			       sizeof ( password ) );
#if 1

	if(username[0] == 0)
		return -1;
		
	//multiboot:iqn.2008-12.com.ccboot.211:2
	// iscsi:192.168.1.201::3260::iqn.2008-12.com.ccboot.201:1
	if(strncmp(username, "iscsi:", strlen("iscsi:")) == 0)
	{
		// 在multiboot里面，会修改root_path_setting
		rc = multiboot(username, password); 
		store_setting ( NULL, &username_setting, NULL, 0 );
		store_setting ( NULL, &password_setting, NULL, 0 );
		// 这个地方必须改回来，否则ms iscsi initiator会因为密码不足12位而失败
		store_setting ( NULL, &username_setting, "ccboot_multiboot",  strlen("ccboot_multiboot"));
		store_setting ( NULL, &password_setting, "123456789012", strlen("123456789012"));
		return rc;
	}

	// new ip
	// 这个时候username是机器名 password是ip
	fetch_string_setting (NULL, &hostname_setting, hostname, sizeof(hostname));
	param = strstr(hostname, "(");
#endif		
	/* Initialise UI */
	initscr();
	start_color();
	init_pair ( CPAIR_NORMAL, COLOR_NORMAL_FG, COLOR_NORMAL_BG );
	init_pair ( CPAIR_EDIT_NEW, COLOR_EDIT_FG, COLOR_EDIT_BG );
	init_editbox ( &username_box, username, sizeof ( username ), NULL,
		       USERNAME_ROW, EDITBOX_COL, EDITBOX_WIDTH, 0 );
#if 0
	init_editbox ( &password_box, password, sizeof ( password ), NULL,
		       PASSWORD_ROW, EDITBOX_COL, EDITBOX_WIDTH,
		       EDITBOX_STARS );
#else
	init_editbox ( &password_box, password, sizeof ( password ), NULL,
		       PASSWORD_ROW, EDITBOX_COL, EDITBOX_WIDTH,
		       0 );
#endif

	/* Draw initial UI */
	color_set ( CPAIR_NORMAL, NULL );
	erase();
	attron ( A_BOLD );
#if 0
	mvprintw ( USERNAME_LABEL_ROW, LABEL_COL, "Username:" );
	mvprintw ( PASSWORD_LABEL_ROW, LABEL_COL, "Password:" );
#else
	mvprintw ( USERNAME_LABEL_ROW, LABEL_COL, "Computer Name" );
	mvprintw ( PASSWORD_LABEL_ROW, LABEL_COL, "IP Address" );
#endif
	attroff ( A_BOLD );
	color_set ( CPAIR_EDIT_NEW, NULL );
	draw_editbox ( &username_box );
	draw_editbox ( &password_box );

	/* Main loop */
	while ( rc == -EINPROGRESS ) {

		draw_editbox ( current_box );

		key = getkey ( 0 );
		switch ( key ) {
		case KEY_DOWN:
			current_box = &password_box;
			break;
		case KEY_UP:
			current_box = &username_box;
			break;
		case TAB:
			current_box = ( ( current_box == &username_box ) ?
					&password_box : &username_box );
			break;
		case KEY_ENTER:
			if ( current_box == &username_box ) {
				current_box = &password_box;
			} else {
				rc = 0;
			}
			break;
		case CTRL_C:
		case ESC:
			rc = -ECANCELED;
			break;
		default:
			edit_editbox ( current_box, key );
			break;
		}
	}

	/* Terminate UI */
	color_set ( CPAIR_NORMAL, NULL );
	erase();
	endwin();

	if ( rc != 0 )
		return rc;
#if 0
	/* Store settings */
	if ( ( rc = store_setting ( NULL, &username_setting, username,
				    strlen ( username ) ) ) != 0 )
		return rc;
	if ( ( rc = store_setting ( NULL, &password_setting, password,
				    strlen ( password ) ) ) != 0 )
		return rc;
#else
	snprintf(buffer, sizeof(buffer), "%s:%s", username, password);

	// 把用户输入的机器名和IP格式化成username传回服务器
	/* Store settings */
	if ( ( rc = store_setting ( NULL, &username_setting, buffer,
				    strlen ( buffer ) ) ) != 0 )
		return rc;

	// 确保ms iscsi通过
	snprintf(buffer, sizeof(buffer), "123456789012");

	if ( ( rc = store_setting ( NULL, &password_setting, buffer,
				    strlen ( buffer ) ) ) != 0 )
		return rc;

	// 根据机器名修改hostname_settings
	if(param)
		snprintf(buffer, sizeof(buffer), "%s%s", username, param);
	else
		snprintf(buffer, sizeof(buffer), "%s", username);

	if ( ( rc = store_setting ( NULL, &hostname_setting, buffer,
				    strlen ( buffer ) ) ) != 0 )
		return rc;
#endif
	return 0;
}
