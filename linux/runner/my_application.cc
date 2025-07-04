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

struct _MyApplication {
    GtkApplication parent_instance;
    char** dart_entrypoint_arguments;
};

G_DEFINE_TYPE(MyApplication, my_application, GTK_TYPE_APPLICATION);

// Platform Channel Method Call Handler
static void method_call_handler(FlMethodChannel* channel,
                                FlMethodCall* method_call,
                                gpointer user_data) {
    const gchar* method_name = fl_method_call_get_name(method_call);
    FlValue* args = fl_method_call_get_args(method_call);

    if (strcmp(method_name, "initializeScanner") == 0) {
        if (!global_scanner_instance) {
            global_scanner_instance = std::make_unique<SinosecuScanner>();
        }
    } else {
        if (!global_scanner_instance) {
            std::cerr << "Linux side: global_scanner_instance is null for method " << method_name << std::endl;
            fl_method_call_respond(method_call, FL_METHOD_RESPONSE(fl_method_error_response_new("SCANNER_NOT_READY", "Scanner instance not available.", nullptr)), nullptr);
            return;
        }
    }

    FlMethodResponse* response = nullptr;

    if (strcmp(method_name, "initializeScanner") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            std::cerr << "Linux side: initializeScanner arguments are not a map." << std::endl;
            fl_method_call_respond(method_call, FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for initializeScanner", nullptr)), nullptr);
            return;
        }

        FlValue* user_id_value = fl_value_lookup_string(args, "userId");
        FlValue* n_type_value = fl_value_lookup_string(args, "nType");
        FlValue* sdk_dir_value = fl_value_lookup_string(args, "sdkDirectory");

        if (!user_id_value || fl_value_get_type(user_id_value) != FL_VALUE_TYPE_STRING ||
            !n_type_value || fl_value_get_type(n_type_value) != FL_VALUE_TYPE_INT ||
            !sdk_dir_value || fl_value_get_type(sdk_dir_value) != FL_VALUE_TYPE_STRING) {
            std::cerr << "Linux side: Missing or incorrect argument types for initializeScanner." << std::endl;
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid arguments for initializeScanner", nullptr));
        } else {
            const char* userId_cstr = fl_value_get_string(user_id_value);
            int nType = fl_value_get_int(n_type_value);
            const char* sdkDirectory_cstr = fl_value_get_string(sdk_dir_value);

            std::cout << "Linux side: Calling initializeScanner with UserID: " << (userId_cstr ? userId_cstr : "NULL")
                      << ", nType: " << nType
                      << ", Directory: " << (sdkDirectory_cstr ? sdkDirectory_cstr : "NULL") << std::endl;

            int result = global_scanner_instance->initializeScanner(std::string(userId_cstr ? userId_cstr : ""), nType, std::string(sdkDirectory_cstr ? sdkDirectory_cstr : ""));
            response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
        }
    }
    else if (strcmp(method_name, "releaseScanner") == 0) {
        std::cout << "Linux side: Calling releaseScanner." << std::endl;
        if (global_scanner_instance) {
            global_scanner_instance->releaseScanner();
        }
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
    }
    else if (strcmp(method_name, "detectDocument") == 0) {
        std::cout << "Linux side: Calling detectDocumentOnScanner." << std::endl;
        int result = global_scanner_instance->detectDocumentOnScanner();
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
    }
    else if (strcmp(method_name, "waitForDocumentDetection") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for waitForDocumentDetection", nullptr));
        } else {
            FlValue* timeout_value = fl_value_lookup_string(args, "timeoutSeconds");
            if (!timeout_value || fl_value_get_type(timeout_value) != FL_VALUE_TYPE_INT) {
                response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid timeoutSeconds argument", nullptr));
            } else {
                int timeoutSeconds = fl_value_get_int(timeout_value);
                std::cout << "Linux side: Waiting for document detection (timeout: " << timeoutSeconds << "s)" << std::endl;
                int result = global_scanner_instance->waitForDocumentDetection(timeoutSeconds);
                response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
            }
        }
    }
    else if (strcmp(method_name, "autoProcessDocument") == 0) {
        std::cout << "Linux side: Calling autoProcessDocument." << std::endl;
        std::map<std::string, int> cpp_result_map = global_scanner_instance->autoProcessDocument();

        g_autoptr(FlValue) return_value_map = fl_value_new_map();
        for (const auto& pair : cpp_result_map) {
            fl_value_set_string_take(return_value_map, pair.first.c_str(), fl_value_new_int(pair.second));
        }
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(return_value_map));
    }
    else if (strcmp(method_name, "getDocumentFields") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for getDocumentFields", nullptr));
        } else {
            FlValue* attribute_value = fl_value_lookup_string(args, "attribute");
            if (!attribute_value || fl_value_get_type(attribute_value) != FL_VALUE_TYPE_INT) {
                response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid attribute argument", nullptr));
            } else {
                int attribute = fl_value_get_int(attribute_value);
                std::cout << "Linux side: Getting document fields for attribute: " << attribute << std::endl;
                std::map<std::string, std::string> fields = global_scanner_instance->getDocumentFields(attribute);

                g_autoptr(FlValue) return_value_map = fl_value_new_map();
                for (const auto& pair : fields) {
                    fl_value_set_string_take(return_value_map, pair.first.c_str(), fl_value_new_string(pair.second.c_str()));
                }
                response = FL_METHOD_RESPONSE(fl_method_success_response_new(return_value_map));
            }
        }
    }
    else if (strcmp(method_name, "checkDeviceStatus") == 0) {
        std::cout << "Linux side: Checking device status." << std::endl;
        int result = global_scanner_instance->checkDeviceStatus();
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
    }
    else if (strcmp(method_name, "scanDocumentComplete") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for scanDocumentComplete", nullptr));
        } else {
            FlValue* timeout_value = fl_value_lookup_string(args, "timeoutSeconds");
            if (!timeout_value || fl_value_get_type(timeout_value) != FL_VALUE_TYPE_INT) {
                response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid timeoutSeconds argument", nullptr));
            } else {
                int timeoutSeconds = fl_value_get_int(timeout_value);
                std::cout << "Linux side: Starting complete document scan (timeout: " << timeoutSeconds << "s)" << std::endl;
                std::map<std::string, std::string> scanResult = global_scanner_instance->scanDocumentComplete(timeoutSeconds);

                g_autoptr(FlValue) return_value_map = fl_value_new_map();
                for (const auto& pair : scanResult) {
                    fl_value_set_string_take(return_value_map, pair.first.c_str(), fl_value_new_string(pair.second.c_str()));
                }
                response = FL_METHOD_RESPONSE(fl_method_success_response_new(return_value_map));
            }
        }
    }
    else if (strcmp(method_name, "getDocumentName") == 0) {
        std::cout << "Linux side: Getting document name." << std::endl;
        std::string docName = global_scanner_instance->getDocumentName();
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_string(docName.c_str())));
    }
    else if (strcmp(method_name, "saveImages") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for saveImages", nullptr));
        } else {
            FlValue* base_path_value = fl_value_lookup_string(args, "basePath");
            FlValue* image_types_value = fl_value_lookup_string(args, "imageTypes");

            if (!base_path_value || fl_value_get_type(base_path_value) != FL_VALUE_TYPE_STRING ||
                !image_types_value || fl_value_get_type(image_types_value) != FL_VALUE_TYPE_INT) {
                response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid arguments for saveImages", nullptr));
            } else {
                const char* basePath_cstr = fl_value_get_string(base_path_value);
                int imageTypes = fl_value_get_int(image_types_value);
                std::cout << "Linux side: Saving images to: " << basePath_cstr << std::endl;
                bool result = global_scanner_instance->saveImages(std::string(basePath_cstr), imageTypes);
                response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_bool(result)));
            }
        }
    }
    else if (strcmp(method_name, "loadConfiguration") == 0) {
        if (fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
            response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Expected map argument for loadConfiguration", nullptr));
        } else {
            FlValue* config_path_value = fl_value_lookup_string(args, "configPath");
            if (!config_path_value || fl_value_get_type(config_path_value) != FL_VALUE_TYPE_STRING) {
                response = FL_METHOD_RESPONSE(fl_method_error_response_new("ARGUMENT_ERROR", "Invalid configPath argument", nullptr));
            } else {
                const char* configPath_cstr = fl_value_get_string(config_path_value);
                std::cout << "Linux side: Loading configuration from: " << configPath_cstr << std::endl;
                int result = global_scanner_instance->loadConfiguration(std::string(configPath_cstr));
                response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_int(result)));
            }
        }
    }
    else if (strcmp(method_name, "getLastError") == 0) {
        std::cout << "Linux side: Getting last error." << std::endl;
        std::string error = global_scanner_instance->getLastError();
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(fl_value_new_string(error.c_str())));
    }
    else {
        std::cout << "Linux side: Method not implemented: " << method_name << std::endl;
        response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
    }

    // Return the response
    if (response) {
        fl_method_call_respond(method_call, response, nullptr);
    }
}

// Implements GApplication::activate.
static void my_application_activate(GApplication* application) {
    MyApplication* self = MY_APPLICATION(application);
    GtkWindow* window =
            GTK_WINDOW(gtk_application_window_new(GTK_APPLICATION(application)));

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
        gtk_header_bar_set_title(header_bar, "SinoScanner App");
        gtk_header_bar_set_show_close_button(header_bar, TRUE);
        gtk_window_set_titlebar(GTK_WINDOW(window), GTK_WIDGET(header_bar));
    } else {
        gtk_window_set_title(GTK_WINDOW(window), "SinoScanner App");
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

    // Register custom platform channel
    FlEngine* engine = fl_view_get_engine(view);
    if (engine == nullptr) {
        std::cerr << "Linux side: Could not get Flutter engine." << std::endl;
        return;
    }

    g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
    if (codec == nullptr) {
        std::cerr << "Linux side: Could not create standard method codec." << std::endl;
        return;
    }

    FlBinaryMessenger* messenger = fl_engine_get_binary_messenger(engine);
    if (messenger == nullptr) {
        std::cerr << "Linux side: Could not get binary messenger from Flutter engine." << std::endl;
        return;
    }

    FlMethodChannel* channel = fl_method_channel_new(
            messenger,
            "com.example.sino_scanner",
            FL_METHOD_CODEC(codec)
    );

    if (channel == nullptr) {
        std::cerr << "Linux side: Failed to create method channel." << std::endl;
        return;
    }

    // Update how we set the method call handler
    fl_method_channel_set_method_call_handler(channel,
                                              method_call_handler,
                                              g_object_ref(self),
                                              g_object_unref);

    std::cout << "Linux side: SinoScanner platform channel registered successfully." << std::endl;

    gtk_widget_grab_focus(GTK_WIDGET(view));
}

// Implements GApplication::local_command_line.
static gboolean my_application_local_command_line(GApplication* application, gchar*** arguments, int* exit_status) {
    MyApplication* self = MY_APPLICATION(application);
    self->dart_entrypoint_arguments = g_strdupv(*arguments + 1);

    g_autoptr(GError) error = nullptr;
    if (!g_application_register(application, nullptr, &error)) {
        g_warning("Failed to register GApplication: %s", error->message);
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

// Implements GApplication::shutdown.
static void my_application_shutdown(GApplication* application) {
    if (global_scanner_instance) {
        std::cout << "Linux side: Releasing scanner on application shutdown." << std::endl;
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
    g_set_prgname("com.example.sino_scanner");
    return MY_APPLICATION(g_object_new(my_application_get_type(),
                                       "application-id", "com.example.sino_scanner",
                                       "flags", G_APPLICATION_NON_UNIQUE,
                                       nullptr));
}