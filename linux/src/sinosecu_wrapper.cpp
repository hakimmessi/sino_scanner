#include "runner/my_application.h"


#include <flutter_linux/flutter_linux.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "flutter/generated_plugin_registrant.h"
#include "src/sinosecu_wrapper.h"
#include <memory>
#include <iostream>
#include <map>

// Global instance of our scanner wrapper.
static std::unique_ptr<SinosecuScanner> global_scanner_instance;


const char APPLICATION_ID[] = "com.example.sino_scanner";


struct _MyApplication {
    GtkApplication parent_instance;
    char** dart_entrypoint_arguments;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION)

// Platform Channel Method Call Handler
static FlMethodResponse* handle_platform_method_call(FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
    const gchar* method_name = fl_method_call_get_name(method_call);
    FlValue* args = fl_method_call_get_args(method_call);

    // Ensure our scanner instance exists for most calls.
    // For "initializeScanner", we create it if it doesn't exist.
    if (strcmp(method_name, "initializeScanner") == 0) {
        if (!global_scanner_instance) {
            global_scanner_instance = std::make_unique<SinosecuScanner>();
        }
    } else {
        // For other methods, if the instance doesn't exist or scanner isn't ready, return error.
        if (!global_scanner_instance) {
            std::cerr << "C++ Error: global_scanner_instance is null for method " << method_name << std::endl;
            return FL_METHOD_RESPONSE(fl_method_error_response_new("SCANNER_NOT_READY", "Scanner instance not available.", nullptr));
        }
    }

    FlMethodResponse* response = nullptr;

    if (strcmp(method_name, "initializeScanner") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            std::cerr << "C++ Error: initializeScanner arguments are not a map." << std::endl;
            return FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for initializeScanner", nullptr));
        }

        FlValue* user_id_value = fl_value_lookup_string(args, "userId");
        FlValue* n_type_value = fl_value_lookup_string(args, "nType");
        FlValue* sdk_dir_value = fl_value_lookup_string(args, "sdkDirectory");

        if (!user_id_value || fl_value_get_type(user_id_value) != FL_VALUE_TYPE_STRING ||
            !n_type_value || fl_value_get_type(n_type_value) != FL_VALUE_TYPE_INT || // Check for INT
            !sdk_dir_value || fl_value_get_type(sdk_dir_value) != FL_VALUE_TYPE_STRING) {
            std::cerr << "C++ Error: Missing or incorrect argument types for initializeScanner." << std::endl;
            // For debugging:
            // if (user_id_value) std::cerr << "  userId type: " << fl_value_get_type(user_id_value) << std::endl; else std::cerr << "  userId missing" << std::endl;
            // if (n_type_value) std::cerr << "  nType type: " << fl_value_get_type(n_type_value) << std::endl; else std::cerr << "  nType missing" << std::endl;
            // if (sdk_dir_value) std::cerr << "  sdkDir type: " << fl_value_get_type(sdk_dir_value) << std::endl; else std::cerr << "  sdkDir missing" << std::endl;
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid arguments for initializeScanner", nullptr));
        } else {
            const char* userId_cstr = fl_value_get_string(user_id_value);
            int nType = fl_value_get_int(n_type_value); // Correctly get int
            const char* sdkDirectory_cstr = fl_value_get_string(sdk_dir_value);

            std::cout << "C++ Platform: Calling initializeScanner with UserID: " << (userId_cstr ? userId_cstr : "NULL")
                      << ", nType: " << nType
                      << ", Directory: " << (sdkDirectory_cstr ? sdkDirectory_cstr : "NULL") << std::endl;

            int result = global_scanner_instance->initializeScanner(std::string(userId_cstr ? userId_cstr : ""), nType, std::string(sdkDirectory_cstr ? sdkDirectory_cstr : ""));
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
        }
    } else if (strcmp(method_name, "releaseScanner") == 0) {
        std::cout << "C++ Platform: Calling releaseScanner." << std::endl;
        if (global_scanner_instance) { // Check ensures it exists before calling
            global_scanner_instance->releaseScanner();
        }
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
    } else if (strcmp(method_name, "detectDocument") == 0) {
        std::cout << "C++ Platform: Calling detectDocumentOnScanner." << std::endl;
        int result = global_scanner_instance->detectDocumentOnScanner();
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
    } else if (strcmp(method_name, "autoProcessDocument") == 0) {
        std::cout << "C++ Platform: Calling autoProcessDocumentInScanner." << std::endl;
        std::map<std::string, int> cpp_result_map = global_scanner_instance->autoProcessDocumentInScanner();

        // Convert std::map<std::string, int> to FlValue (map type) for Dart
        g_autoptr(FlValue) return_value_map = fl_value_new_map();
        for (const auto& pair : cpp_result_map) {
            // Key is const char*, Value is FlValue*
            fl_value_set_string_take(return_value_map, pair.first.c_str(), fl_value_new_int(pair.second));
        }
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(return_value_map));
    }
    else {
        std::cout << "C++ Platform: Method not implemented: " << method_name << std::endl;
        response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
    }
    return response;
}


// Implements GApplication::activate.
static void my_application_activate(GApplication* application) {
    MyApplication* self = MY_APPLICATION(application);
    GtkWindow* window =
            GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));


        // Use a header bar when running in GNOME as this is the common style used
        // by applications and is the setup most users will be using (e.g. Ubuntu
        // desktop).
        // If running on X and not using GNOME then just use a traditional title bar
        // in case the window manager does more exotic layout, e.g. tiling.
        // If running on Wayland assume the header bar will work (may need changing
        // if future cases occur).
    gboolean use_header_bar = TRUE;
    #ifdef GDK_WINDOWING_X11
    GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(window));
    if (GDK_IS_X11_SCREEN(screen)) {
        const gchar* wm_name = gdk_x11_screen_get_window_manager_name(screen);
        if (g_strcmp0(wm_name, "GNOME Shell") != 0) {
            use_header_bar = FALSE;
        }
    }
    #endif
    if (use_header_bar) {
        GtkHeaderBar* header_bar = GTK_HEADER_BAR(gtk_header_bar_new());
        gtk_widget_show(GTK_WIDGET(header_bar));
        gtk_header_bar_set_title(header_bar, "SinoScanner App"); // Updated title
        gtk_header_bar_set_show_close_button(header_bar, TRUE);
        gtk_window_set_titlebar(GTK_WINDOW(window), GTK_WIDGET(header_bar));
    } else {
        gtk_window_set_title(GTK_WINDOW(window), "SinoScanner App"); // Updated title
    }

    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);
    gtk_widget_show(GTK_WIDGET(window));


    g_autoptr(FlDartProject) project = fl_dart_project_new();
    fl_dart_project_set_dart_entrypoint_arguments(project, self->dart_entrypoint_arguments);

    FlView* view = fl_view_new(project);
    gtk_widget_show(GTK_WIDGET(view));
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));

    // Register Flutter plugins
    fl_register_plugins(FL_PLUGIN_REGISTRY(view));

    // Register our custom platform channel
    // FlPluginRegistrar* registrar = fl_plugin_registry_get_registrar_for_plugin(FL_PLUGIN_REGISTRY(view), "SinoScannerChannel");
    // The above line might be problematic if "SinoScannerChannel" isn't a formally registered plugin name.
    // A more direct way to get a messenger for a simple, non-plugin channel:
    FlMessageChannel* messenger = fl_plugin_registry_get_messenger(FL_PLUGIN_REGISTRY(view));


    if (messenger == nullptr) { // Check if messenger is valid
        std::cerr << "C++ Error: Could not get plugin messenger." << std::endl;
    } else {
        FlMethodChannel* channel = fl_method_channel_new(
                messenger, // Use the direct messenger
                APPLICATION_ID,
                FL_STANDARD_METHOD_CODEC_GET_INSTANCE);

        fl_method_channel_set_method_call_handler(channel, handle_platform_method_call,
                                                  nullptr, // user_data for the handler
                                                  nullptr  // destroy_notify for user_data
        );
        std::cout << "C++ Platform: SinoScanner platform channel registered successfully with name 'com.example.sino_scanner'." << std::endl;
    }

    gtk_widget_grab_focus(GTK_WIDGET(view));
}

// Implements GApplication::local_command_line.
static gboolean my_application_local_command_line(GApplication* application, gchar*** arguments, int* exit_status) {
    MyApplication* self = MY_APPLICATION(application);
    self->dart_entrypoint_arguments = g_strdupv(*arguments + 1); // Keep only args for Dart

    g_autoptr(GError) error = nullptr;
    if (!g_application_register(application, nullptr, &error)) { // Register the application
        g_warning("Failed to register GApplication: %s", error->message);
        *exit_status = 1;
        return TRUE; // Stop further processing by returning TRUE
    }

    g_application_activate(application); // Activate the application
    *exit_status = 0;

    return TRUE; // We handled the command line
}

// Implements GApplication::startup.
static void my_application_startup(GApplication* application) {
    // Any initial setup for the application itself (not per-window)
    G_APPLICATION_CLASS(my_application_parent_class)->startup(application);
}

// Implements GApplication::shutdown.
static void my_application_shutdown(GApplication* application) {
    if (global_scanner_instance) {
        std::cout << "C++ Platform: Releasing scanner on application shutdown." << std::endl;
        global_scanner_instance->releaseScanner();
        global_scanner_instance.reset();
    }
    G_APPLICATION_CLASS(my_application_parent_class)->shutdown(application);
}

// Implements GObject::dispose.
static void my_application_dispose(GObject* object) {
    MyApplication* self = MY_APPLICATION(object);
    g_clear_pointer(&self->dart_entrypoint_arguments, g_strfreev);
    G_OBJECT_CLASS(my_application_parent_class)->dispose(object);
}

static void my_application_class_init(MyApplicationClass* klass) {
    G_APPLICATION_CLASS(klass)->activate = my_application_activate;
    G_APPLICATION_CLASS(klass)->local_command_line = my_application_local_command_line;
    G_APPLICATION_CLASS(klass)->startup = my_application_startup;
    G_APPLICATION_CLASS(klass)->shutdown = my_application_shutdown;
    G_OBJECT_CLASS(klass)->dispose = my_application_dispose;
}

static void my_application_init(MyApplication* self) {
    // Initialization for each instance of MyApplication, if any.
}

MyApplication* my_application_new() {
    g_set_prgname(APPLICATION_ID);
    return MY_APPLICATION(g_object_new(my_application_get_type(),
                                       "application-id", APPLICATION_ID,
                                       "flags", G_APPLICATION_NON_UNIQUE,
                                       nullptr));
}