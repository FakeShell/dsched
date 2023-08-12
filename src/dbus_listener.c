// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include "dbus_listener.h"
#include "generated-code.h"

#define MAX_RECEIVED_PIDS 32768

static gint received_pids[MAX_RECEIVED_PIDS];
static int received_pids_count = 0;

extern void add_set_pid(int pid, const char *program_name);

static void on_handle_add_pid(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data) {

    gint pid;
    g_variant_get(parameters, "(i)", &pid);

    if (received_pids_count < MAX_RECEIVED_PIDS) {
        received_pids[received_pids_count++] = pid;
    }

    g_dbus_method_invocation_return_value(invocation, NULL);
}

static const GDBusInterfaceVTable interface_vtable = {
    .method_call = on_handle_add_pid,
};

void start_dbus_listener() {
    GMainLoop *loop;
    GDBusConnection *connection;
    guint owner_id;
    guint registration_id;

    loop = g_main_loop_new(NULL, FALSE);

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    if (connection == NULL) {
        g_printerr("Failed to connect to the D-Bus system bus\n");
        return;
    }

    GDBusInterfaceInfo *interface_info = org_droidian_dsched_interface_info();
    registration_id = g_dbus_connection_register_object(
        connection,
        "/org/droidian/dsched",
        interface_info,
        &interface_vtable,
        NULL,
        NULL,
        NULL);

    if (registration_id == 0) {
        g_printerr("Failed to register object\n");
        return;
    }

    owner_id = g_bus_own_name_on_connection(
        connection,
        "org.droidian.dsched",
        G_BUS_NAME_OWNER_FLAGS_NONE,
        NULL,
        NULL,
        NULL,
        NULL);

    g_main_loop_run(loop);

    g_bus_unown_name(owner_id);
    g_dbus_connection_unregister_object(connection, registration_id);
    g_object_unref(connection);
    g_main_loop_unref(loop);
}

void get_received_pids(gint **pids, int *count) {
    *pids = received_pids;
    *count = received_pids_count;
}
