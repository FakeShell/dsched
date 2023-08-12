// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include <gio/gio.h>
#include <glib.h>
#include <unistd.h>
#include <stdio.h>

static void call_add_pid() {
    GDBusConnection *connection;
    GError *error = NULL;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (connection == NULL) {
        g_printerr("Failed to connect to the D-Bus system bus: %s\n", error->message);
        g_error_free(error);
        return;
    }

    gint pid = getpid();
    printf("Sending PID to D-Bus: %d\n", pid);

    GVariant *result = g_dbus_connection_call_sync(
        connection,
        "org.droidian.dsched",
        "/org/droidian/dsched",
        "org.droidian.dsched",
        "AddPid",
        g_variant_new("(i)", pid),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (result == NULL) {
        g_printerr("Failed to call method: %s\n", error->message);
        g_error_free(error);
    } else {
        g_variant_unref(result);
    }

    g_object_unref(connection);
}

int main(int argc, char **argv) {
    call_add_pid();

    while (1) {
        sleep(5);
    }

    return 0;
}
