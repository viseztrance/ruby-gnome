
/* -*- c-file-style: "ruby"; indent-tabs-mode: nil -*- */
/************************************************

  rbgtkmain.c -

  $Author: mutoh $
  $Date: 2005/01/29 11:44:14 $

  Copyright (C) 2002,2003 Ruby-GNOME2 Project Team
  Copyright (C) 1998-2000 Yukihiro Matsumoto,
                          Daisuke Kanda,
                          Hiroshi Igarashi
************************************************/
#include "global.h"
#include <locale.h>

EXTERN VALUE rb_progname, rb_argv;

static VALUE rbgtk_main_threads = Qnil;

static gboolean
gtk_m_function(data)
    gpointer data;
{
    VALUE ret = rb_funcall((VALUE)data, id_call, 0);
    return RTEST(ret);
}

static VALUE
gtk_m_set_locale(self)
    VALUE self;
{
    return CSTR2RVAL(gtk_set_locale());
}

static VALUE
gtk_m_disable_setlocale(self)
    VALUE self;
{
    gtk_disable_setlocale();
    return Qnil;
}

static VALUE
gtk_m_get_default_language(self)
    VALUE self;
{
    return BOXED2RVAL(gtk_get_default_language(), PANGO_TYPE_LANGUAGE);
}

typedef void (*SignalFunc) (int);

static VALUE
gtk_m_init(argc, argv, self)
    int argc;
    VALUE *argv;
    VALUE self;
{
    gint i, gargc;
    VALUE argary;
    gchar** gargv;

    rb_scan_args(argc, argv, "01", &argary);

    if (NIL_P(argary)){
        gargc = RARRAY(rb_argv)->len;
        argary = rb_argv;
    } else {
        Check_Type(argary, T_ARRAY);
        gargc = RARRAY(argary)->len;
    }

    gargv = ALLOCA_N(char*,gargc + 1);
    gargv[0] = RVAL2CSTR(rb_progname);

    for (i = 0; i < gargc; i++) {
        if (TYPE(RARRAY(argary)->ptr[i]) == T_STRING) {
            gargv[i+1] = RVAL2CSTR(RARRAY(argary)->ptr[i]);
        }
        else {
            gargv[i+1] = "";
        }
    }
    gargc++;

    {
        gboolean is_initialized;

        /* Gdk modifies sighandlers, sigh */
#ifdef NT
        RETSIGTYPE (*sigfunc[3])();
#else
        RETSIGTYPE (*sigfunc[7])();
#endif

#ifdef NT
        sigfunc[0] = signal(SIGINT, SIG_IGN);
        sigfunc[1] = signal(SIGSEGV, SIG_IGN);
        sigfunc[2] = signal(SIGTERM, SIG_IGN);
#else
        sigfunc[0] = signal(SIGHUP, SIG_IGN);
        sigfunc[1] = signal(SIGINT, SIG_IGN);
        sigfunc[2] = signal(SIGQUIT, SIG_IGN);
        sigfunc[3] = signal(SIGBUS, SIG_IGN);
        sigfunc[4] = signal(SIGSEGV, SIG_IGN);
        sigfunc[5] = signal(SIGPIPE, SIG_IGN);
        sigfunc[6] = signal(SIGTERM, SIG_IGN);
#endif
        is_initialized = gtk_init_check(&gargc, &gargv);
        setlocale(LC_NUMERIC, "C");

#ifdef NT
        signal(SIGINT,  (SignalFunc)sigfunc[0]);
        signal(SIGSEGV, (SignalFunc)sigfunc[1]);
        signal(SIGTERM, (SignalFunc)sigfunc[2]);
#else
        signal(SIGHUP,  sigfunc[0]);
        signal(SIGINT,  sigfunc[1]);
        signal(SIGQUIT, sigfunc[2]);
        signal(SIGBUS,  sigfunc[3]);
        signal(SIGSEGV, sigfunc[4]);
        signal(SIGPIPE, sigfunc[5]);
        signal(SIGTERM, sigfunc[6]);
#endif
        
        if (!is_initialized)
            rb_raise(rb_eRuntimeError, "failed to initialize gtk+");
    }

    return self;
}

/* We don't need them.
gtk_init()
gtk_exit()
*/

static VALUE
gtk_m_events_pending(self)
    VALUE self;
{
    return gtk_events_pending() ? Qtrue : Qfalse;
}

static VALUE
gtk_m_main(self)
    VALUE self;
{
    rb_ary_push(rbgtk_main_threads, rb_thread_current());
    gtk_main();
    return Qnil;
}

static VALUE
gtk_m_main_level(self)
    VALUE self;
{
    return INT2FIX(gtk_main_level());
}

static VALUE
gtk_m_main_quit(self)
    VALUE self;
{
    VALUE thread = rb_ary_pop(rbgtk_main_threads);
    gtk_main_quit();
    if (NIL_P(thread)){
        rb_warning("Gtk.main_quit was called incorrectly.");
    } else {
        rb_thread_wakeup(thread);
    }
    return Qnil;
}

static VALUE
gtk_m_main_iteration(self)
    VALUE self;
{
    return gtk_main_iteration() ? Qtrue : Qfalse;
}

static VALUE
gtk_m_main_iteration_do(self, blocking)
    VALUE self, blocking;
{
    return gtk_main_iteration_do(RTEST(blocking)) ? Qtrue : Qfalse;
}

static VALUE
gtk_m_main_do_event(self, event)
    VALUE self, event;
{
    gtk_main_do_event(RVAL2GEV(event));
    return event;
}

/* We don't need them.
gtk_true()
gtk_false()
*/

static VALUE
gtk_m_grab_add(self, widget)
    VALUE self, widget;
{
    gtk_grab_add(GTK_WIDGET(RVAL2GOBJ(widget)));
    return Qnil;
}

static VALUE
gtk_m_get_current(self)
    VALUE self;
{
    return GOBJ2RVAL(gtk_grab_get_current());
}

static VALUE
gtk_m_grab_remove(self, widget)
    VALUE self, widget;
{
    gtk_grab_remove(GTK_WIDGET(RVAL2GOBJ(widget)));
    return Qnil;
}

static VALUE
gtk_m_init_add(self)
    VALUE self;
{
    volatile VALUE func = G_BLOCK_PROC();
    
    gtk_init_add((GtkFunction)gtk_m_function, (gpointer)func);
    G_RELATIVE(self, func);
    return Qnil;
}

static VALUE
gtk_m_quit_add(self, main_level)
    VALUE self, main_level;
{
    volatile VALUE func = G_BLOCK_PROC();
    VALUE id;

    id = INT2FIX(gtk_quit_add(NUM2UINT(main_level), 
                                (GtkFunction)gtk_m_function, (gpointer)func));
    G_RELATIVE2(self, func, id_relative_callbacks, id);
    return id;
}

static VALUE
gtk_m_quit_remove(self, quit_handler_id)
    VALUE self, quit_handler_id;
{
    gtk_quit_remove(NUM2UINT(quit_handler_id));
    G_REMOVE_RELATIVE(self, id_relative_callbacks, quit_handler_id);
    return quit_handler_id;
}

/* We don't need this.
gtk_quit_add_full ()
gtk_quit_add_destroy()
gtk_quit_remove_by_data()
gtk_timeout_add_full()
*/

static VALUE
timeout_add(self, interval)
    VALUE self, interval;
{
    VALUE id;
    VALUE func;

    func = G_BLOCK_PROC();
    id = INT2FIX(gtk_timeout_add(NUM2INT(interval),
                                 (GtkFunction)gtk_m_function,
                                 (gpointer)func));
    G_RELATIVE2(self, func, id_relative_callbacks, id);
    return id;
}

static VALUE
timeout_remove(self, id)
    VALUE self, id;
{
    gtk_timeout_remove(NUM2INT(id));
    G_REMOVE_RELATIVE(self, id_relative_callbacks, id);
    return Qnil;
}

static VALUE
idle_add(self)
    VALUE self;
{
    VALUE id;
    VALUE func;

    func = G_BLOCK_PROC();
    id = INT2FIX(gtk_idle_add((GtkFunction)gtk_m_function, (gpointer)func));
    G_RELATIVE2(self, func, id_relative_callbacks, id);
    return id;
}

static VALUE
idle_add_priority(self, priority)
    VALUE self;
{
    VALUE id;
    VALUE func;

    func = G_BLOCK_PROC();
    id = INT2FIX(gtk_idle_add_priority(NUM2INT(priority),
                                       (GtkFunction)gtk_m_function, 
                                       (gpointer)func));
    G_RELATIVE2(self, func, id_relative_callbacks, id);
    return id;
}

static VALUE
idle_remove(self, id)
    VALUE self, id;
{
    gtk_idle_remove(NUM2INT(id));
    G_REMOVE_RELATIVE(self, id_relative_callbacks, id);
    return Qnil;
}

/* We don't need this.
gtk_idle_remove_by_data()
gtk_idle_add_full()

Use Gdk::Input.add, remove
gtk_input_add_full()
gtk_input_remove()
*/

static gint
gtk_m_key_snoop_func(grab_widget, event, func)
    GtkWidget* grab_widget;
    GdkEventKey* event;
    gpointer func;
{
    VALUE ret = rb_funcall((VALUE)func, id_call, 2, 
                           GOBJ2RVAL(grab_widget), 
                           GEV2RVAL((GdkEvent*)event));
    return RTEST(ret);
}

static VALUE
gtk_m_key_snooper_install(self)
    VALUE self;
{
    VALUE func = G_BLOCK_PROC();
    VALUE id = INT2FIX(gtk_key_snooper_install(
                           (GtkKeySnoopFunc)gtk_m_key_snoop_func, 
                           (gpointer)func));
    G_RELATIVE2(self, func, id_relative_callbacks, id);
    return id;
}

static VALUE
gtk_m_key_snooper_remove(self, id)
    VALUE self, id;
{
    gtk_key_snooper_remove(NUM2UINT(id));
    G_REMOVE_RELATIVE(self, id_relative_callbacks, id);
    return Qnil;
}

static VALUE
gtk_m_get_current_event(self)
    VALUE self;
{
    return GEV2RVAL(gtk_get_current_event());
}

static VALUE
gtk_m_get_current_event_time(self)
    VALUE self;
{
    return INT2NUM(gtk_get_current_event_time());
}

static VALUE
gtk_m_get_current_event_state(self)
    VALUE self;
{
    GdkModifierType state;
    gboolean ret = gtk_get_current_event_state(&state);
    return ret ? GFLAGS2RVAL(state, GDK_TYPE_MODIFIER_TYPE) : Qnil;
}

static VALUE
gtk_m_get_event_widget(argc, argv, self)
    int argc;
    VALUE* argv;
    VALUE self;
{
    VALUE event;
    rb_scan_args(argc, argv, "01", &event);

    return GOBJ2RVAL(gtk_get_event_widget(NIL_P(event) ? NULL :RVAL2GEV(event)));
}

static VALUE
gtk_m_propagate_event(self, widget, event)
    VALUE self, widget, event;
{
    gtk_propagate_event(GTK_WIDGET(RVAL2GOBJ(widget)), RVAL2GEV(event));
    return Qnil;
}

/* From Version Information */
static VALUE
gtk_m_check_version(self, major, minor, micro)
    VALUE self, major, minor, micro;
{
    gchar * ret = NULL;
    ret = gtk_check_version(FIX2INT(major),
                            FIX2INT(minor), FIX2INT(micro));
    return ret ? CSTR2RVAL(ret) : Qnil;
}

static VALUE
gtk_m_check_version_q(self, major, minor, micro)
    VALUE self, major, minor, micro;
{
    gchar * ret = NULL;
    ret = gtk_check_version(FIX2INT(major),
                            FIX2INT(minor), FIX2INT(micro));
    return (ret == NULL) ? Qtrue : Qfalse;
}


void 
Init_gtk_main()
{
    rb_define_module_function(mGtk, "events_pending?", gtk_m_events_pending, 0);
    rb_define_module_function(mGtk, "set_locale", gtk_m_set_locale, 0);
    rb_define_module_function(mGtk, "disable_setlocale", gtk_m_disable_setlocale, 0);
    rb_define_module_function(mGtk, "default_language", gtk_m_get_default_language, 0);
    rb_define_module_function(mGtk, "init", gtk_m_init, -1);
    rb_global_variable(&rbgtk_main_threads);
    rbgtk_main_threads = rb_ary_new();
    rb_define_module_function(mGtk, "main", gtk_m_main, 0);
    rb_define_module_function(mGtk, "main_level", gtk_m_main_level, 0);
    rb_define_module_function(mGtk, "main_quit", gtk_m_main_quit, 0);
    rb_define_module_function(mGtk, "main_iteration", gtk_m_main_iteration, 0);
    rb_define_module_function(mGtk, "main_iteration_do", gtk_m_main_iteration_do, 1);
    rb_define_module_function(mGtk, "main_do_event", gtk_m_main_do_event, 1);
    rb_define_module_function(mGtk, "grab_add", gtk_m_grab_add, 1);
    rb_define_module_function(mGtk, "current", gtk_m_get_current, 0);
    rb_define_module_function(mGtk, "grab_remove", gtk_m_grab_remove, 1);
    rb_define_module_function(mGtk, "init_add", gtk_m_init_add, 0);
    rb_define_module_function(mGtk, "quit_add", gtk_m_quit_add, 1);
    rb_define_module_function(mGtk, "quit_remove", gtk_m_quit_remove, 1);

    rb_define_module_function(mGtk, "timeout_add", timeout_add, 1);
    rb_define_module_function(mGtk, "timeout_remove", timeout_remove, 1);
    rb_define_module_function(mGtk, "idle_add", idle_add, 0);
    rb_define_module_function(mGtk, "idle_add_priority", idle_add_priority, 1);
    rb_define_module_function(mGtk, "idle_remove", idle_remove, 1);
    rb_define_module_function(mGtk, "key_snooper_install", gtk_m_key_snooper_install, 0);
    rb_define_module_function(mGtk, "key_snooper_remove", gtk_m_key_snooper_remove, 1);
    rb_define_module_function(mGtk, "current_event", gtk_m_get_current_event, 0);
    rb_define_module_function(mGtk, "current_event_time", gtk_m_get_current_event_time, 0);
    rb_define_module_function(mGtk, "current_event_state", gtk_m_get_current_event_state, 0);
    rb_define_module_function(mGtk, "get_event_widget", gtk_m_get_event_widget, -1);
    rb_define_module_function(mGtk, "propagate_event", gtk_m_propagate_event, 2);
    rb_define_module_function(mGtk, "check_version", gtk_m_check_version, 3);
    rb_define_module_function(mGtk, "check_version?", gtk_m_check_version_q, 3);

    rb_define_const(mGtk, "PRIORITY_RESIZE", INT2FIX(GTK_PRIORITY_RESIZE));

}
