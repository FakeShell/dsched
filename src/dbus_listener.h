// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef DBUS_LISTENER_H
#define DBUS_LISTENER_H

#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>

void start_dbus_listener();
void get_received_pids(gint **pids, int *count);

#endif // DBUS_LISTENER_H
