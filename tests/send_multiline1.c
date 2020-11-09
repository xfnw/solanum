/*
 *  Copyright (C) 2020 Ed Kellett
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "tap/basic.h"

#include "ircd_util.h"
#include "client_util.h"

#include "send.h"
#include "s_serv.h"
#include "monitor.h"
#include "s_conf.h"

#define MSG "%s:%d (%s)", __FILE__, __LINE__, __FUNCTION__

static void sendto_multiline_basic(void)
{
	struct Client *user = make_local_person();
	const char *s;

	/* multiline with no items should do nothing */
	sendto_one_multiline_init(user, " ", "foo");
	sendto_one_multiline_fini(user, NULL);
	is_client_sendq_empty(user, MSG);

	/* 510 = 17 * 30. line the end of an item with the end of the 510-byte data exactly */
	sendto_one_multiline_init(user, " ", "prefix78901234567 ");
	for (size_t i = 0; i < 29; i++)
		sendto_one_multiline_item(user, "item567890123456");
	sendto_one_multiline_fini(user, NULL);

	s = get_client_sendq(user);
	is_int(512, strlen(s), MSG);
	is_string("item567890123456\r\n", &s[494], MSG);
	is_client_sendq_empty(user, MSG);

	/* just run exactly the same thing again, there's static state */
	sendto_one_multiline_init(user, " ", "prefix78901234567 ");
	for (size_t i = 0; i < 29; i++)
		sendto_one_multiline_item(user, "item567890123456");
	sendto_one_multiline_fini(user, NULL);

	s = get_client_sendq(user);
	is_int(512, strlen(s), MSG);
	is_string("item567890123456\r\n", &s[494], MSG);
	is_client_sendq_empty(user, MSG);

	/* the same thing again but with one extra character, so we have an item that just won't fit */
	sendto_one_multiline_init(user, " ", "prefix789012345678 ");
	for (size_t i = 0; i < 29; i++)
		sendto_one_multiline_item(user, "item567890123456");
	sendto_one_multiline_item(user, "bar");
	sendto_one_multiline_fini(user, "foo ");

	s = get_client_sendq(user);
	is_string("item567890123456\r\n", &s[478], MSG);
	is_client_sendq("foo item567890123456 bar\r\n", user, MSG);

	remove_local_person(user);
}

int main(int argc, char *argv[])
{
	plan_lazy();

	ircd_util_init(__FILE__);
	client_util_init();

	sendto_multiline_basic();

	client_util_free();
	ircd_util_free();
	return 0;
}
