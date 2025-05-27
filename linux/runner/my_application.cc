#include "my_application.h"

#include <flutter_linux/flutter_linux.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include "flutter/generated_plugin_registrant.h"
#include "src/sinosecu_wrapper.h"
#include <memory>
#include <iostream>

static std::unique_ptr<SinosecuScanner> global_scanner_instance;

struct _MyApplication {
  GtkApplication parent_instance;
  char** dart_entrypoint_arguments;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION)

// *** Platform Channel Method Call Handler ***
static FlMethodResponse* handle_platform_method_call(FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
    const gchar* method_name = fl_method_call_get_name(method_call);
    FlValue* args = fl_method_call_get_args(method_call);

    // Ensure our scanner instance exists
    if (!global_scanner_instance) {
        global_scanner_instance = std::make_unique<SinosecuScanner>();
    }

    FlMethodResponse* response = nullptr;

    if (strcmp(method_name, "initializeScanner") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            std::cerr << "Error: initializeScanner arguments are not a map." << std::endl;
            return FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for initializeScanner", nullptr));
        }

        FlValue* user_id_value = fl_value_lookup_string(args, "userId");
        FlValue* n_type_value = fl_value_lookup_string(args, "nType");
        FlValue* sdk_dir_value = fl_value_lookup_string(args, "sdkDirectory");

        if (!user_id_value || fl_value_get_type(user_id_value) != FL_VALUE_TYPE_STRING ||
            !n_type_value || fl_value_get_type(n_type_value) != FL_VALUE_TYPE_INT ||
            !sdk_dir_value || fl_value_get_type(sdk_dir_value) != FL_VALUE_TYPE_STRING) {
            std::cerr << "Error: Missing or incorrect argument types for initializeScanner." << std::endl;
            std::cerr << "  userId type: " << (user_id_value ? fl_value_get_type(user_id_value) : -1) << std::endl;
            std::cerr << "  nType type: " << (n_type_value ? fl_value_get_type(n_type_value) : -1) << std::endl;
            std::cerr << "  sdkDir type: " << (sdk_dir_value ? fl_value_get_type(sdk_dir_value) : -1) << std::endl;
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid arguments for initializeScanner", nullptr));
        } else {
            const char* userId_cstr = fl_value_get_string(user_id_value);
            int nType = fl_value_get_int(n_type_value);
            const char* sdkDirectory_cstr = fl_value_get_string(sdk_dir_value);

            std::cout << "C++: Calling initializeScanner with UserID: " << userId_cstr
                      << ", nType: " << nType
                      << ", Directory: " << sdkDirectory_cstr << std::endl;

            int result = global_scanner_instance->initializeScanner(std::string(userId_cstr), nType, std::string(sdkDirectory_cstr));
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
        }
    } else if (strcmp(method_name, "releaseScanner") == 0) {
        std::cout << "C++: Calling releaseScanner." << std::endl;
        if (global_scanner_instance) {
            global_scanner_instance->releaseScanner();
        }
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr)); // Success with no specific return value
    } else {
        std::cout << "C++: Method not implemented: " << method_name << std::endl;
        response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
    }
    return response;
}


// Implements GApplication::activate.
static void my_application_activate(GApplication* application) {
  MyApplication* self = MY_APPLICATION(application);
  GtkWindow* window =
      GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));


  gboolean use_header_bar = TRUE;
#ifdef GDK_WINDOWING_X11
  GdkScreen* screen = gtk_window_get_screen(window);
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
    gtk_header_bar_set_title(header_bar, "sino_scanner");
    gtk_header_bar_set_show_close_button(header_bar, TRUE);
    gtk_window_set_titlebar(window, GTK_WIDGET(header_bar));
  } else {
    gtk_window_set_title(window, "sino_scanner");
  }

  gtk_window_set_default_size(window, 1280, 720);
  gtk_widget_show(GTK_WIDGET(window));

  g_autoptr(FlDartProject) project = fl_dart_project_new();
  fl_dart_project_set_dart_entrypoint_arguments(project, self->dart_entrypoint_arguments);

  FlView* view = fl_view_new(project);
  gtk_widget_show(GTK_WIDGET(view));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));

  fl_register_plugins(FL_PLUGIN_REGISTRY(view));


    FlPluginRegistrar* registrar = fl_plugin_registry_get_registrar_for_plugin(FL_PLUGIN_REGISTRY(view), "SinoScannerChannel");

    if (registrar == nullptr) {
        std::cerr << "Error: Could not get plugin registrar for SinoScannerChannel." << std::endl;
    } else {
        FlMethodChannel* channel = fl_method_channel_new(
                fl_plugin_registrar_get_messenger(registrar),
                "com.example.scanner/sinosecu",
                FL_STANDARD_METHOD_CODEC_GET_INSTANCE);

        fl_method_channel_set_method_call_handler(channel, handle_platform_method_call,
                                                  nullptr, // user_data for the handler
                                                  nullptr  // destroy_notify for user_data
        );
        std::cout << "SinoScanner platform channel registered." << std::endl;
        // The channel is owned by the registrar, so we don't unref it here typically
        // unless we were managing it manually with g_object_ref.
    }

    gtk_widget_grab_focus(GTK_WIDGET(view));
}

// Implements GApplication::local_command_line.
static gboolean my_application_local_command_line(GApplication* application, gchar*** arguments, int* exit_status) {
  MyApplication* self = MY_APPLICATION(application);
  // Strip out the first argument as it is the binary name.
  self->dart_entrypoint_arguments = g_strdupv(*arguments + 1);

  g_autoptr(GError) error = nullptr;
  if (!g_application_register(application, nullptr, &error)) {
     g_warning("Failed to register: %s", error->message);
     *exit_status = 1;
     return TRUE;
  }

  g_application_activate(application);
  *exit_status = 0;

  return TRUE;
}

// Implements GApplication::startup.
static void my_application_startup(GApplication* application) {
    G_APPLICATION_CLASS(my_application_parent_class)->startup(application);
}

static void my_application_shutdown(GApplication* application) {
    // Clean up scanner instance on shutdown
    if (global_scanner_instance) {
        std::cout << "C++: Releasing scanner on application shutdown." << std::endl;
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

static void my_application_init(MyApplication* self) {}

MyApplication* my_application_new() {
  // Set the program name to the application ID, which helps various systems
  // like GTK and desktop environments map this running application to its
  // corresponding .desktop file. This ensures better integration by allowing
  // the application to be recognized beyond its binary name.
  g_set_prgname(APPLICATION_ID);

  return MY_APPLICATION(g_object_new(my_application_get_type(),
                                     "application-id", APPLICATION_ID,
                                     "flags", G_APPLICATION_NON_UNIQUE,
                                     nullptr));
}
