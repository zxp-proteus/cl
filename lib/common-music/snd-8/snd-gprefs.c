#include "snd.h"
#include "sndlib-strings.h"

static GtkWidget *preferences_dialog = NULL, *load_path_text_widget = NULL;

static bool prefs_helping = false, prefs_unsaved = false;
static char *prefs_saved_filename = NULL;
static char *include_load_path = NULL;

#define HELP_WAIT_TIME ((guint32)500)
#define POWER_WAIT_TIME ((guint32)100)
#define POWER_INITIAL_WAIT_TIME ((guint32)500)
#define ERROR_WAIT_TIME ((guint32)5000)

#define STARTUP_WIDTH 925
#define STARTUP_HEIGHT 800

typedef struct prefs_info {
  GtkWidget *label, *text, *arrow_up, *arrow_down, *arrow_right, *error, *toggle, *scale, *toggle2, *toggle3;
  GtkWidget *color, *rscl, *gscl, *bscl, *rtxt, *gtxt, *btxt, *list_menu, *radio_button, *helper;
  GtkObject *adj, *radj, *gadj, *badj;
  GtkWidget **radio_buttons;
  bool got_error;
  guint help_id, power_id, erase_id;
  const char *var_name, *saved_label;
  const char **values;
  int num_values, num_buttons;
  Float scale_max;
  GtkSizeGroup *color_texts, *color_scales;
  void (*toggle_func)(struct prefs_info *prf);
  void (*toggle2_func)(struct prefs_info *prf);
  void (*toggle3_func)(struct prefs_info *prf);
  void (*scale_func)(struct prefs_info *prf);
  void (*arrow_up_func)(struct prefs_info *prf);
  void (*arrow_down_func)(struct prefs_info *prf);
  void (*text_func)(struct prefs_info *prf);
  void (*color_func)(struct prefs_info *prf, float r, float g, float b);
  void (*reflect_func)(struct prefs_info *prf);
  void (*save_func)(struct prefs_info *prf, FILE *fd);
  void (*help_func)(struct prefs_info *prf);
  void (*clear_func)(struct prefs_info *prf);
  void (*revert_func)(struct prefs_info *prf);
} prefs_info;


static void prefs_set_dialog_title(const char *filename);
static void reflect_key(prefs_info *prf, const char *key_name);
static void save_key_binding(prefs_info *prf, FILE *fd, char *(*binder)(char *key, bool c, bool m, bool x));
static void key_bind(prefs_info *prf, char *(*binder)(char *key, bool c, bool m, bool x));
static void clear_prefs_dialog_error(void);
static void scale_set_color(prefs_info *prf, color_t pixel);
static color_t rgb_to_color(Float r, Float g, Float b);
static char *get_text(GtkWidget *w);
static void set_text(GtkWidget *w, char *value);
static void post_prefs_error(const char *msg, prefs_info *data);
#ifdef __GNUC__
  static void va_post_prefs_error(const char *msg, prefs_info *data, ...) __attribute__ ((format (printf, 1, 0)));
#else
  static void va_post_prefs_error(const char *msg, prefs_info *data, ...);
#endif

#define GET_TOGGLE(Toggle)        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(Toggle))
#define SET_TOGGLE(Toggle, Value) set_toggle_button(Toggle, Value, false, (void *)prf)
#define GET_TEXT(Text)            get_text(Text)
#define SET_TEXT(Text, Val)       set_text(Text, Val)
#define FREE_TEXT(Val)            
#define TIMEOUT(Func)             g_timeout_add_full(0, ERROR_WAIT_TIME, Func, (gpointer)prf, NULL)
#define TIMEOUT_ARGS              gpointer context
#define TIMEOUT_TYPE              gint
#define TIMEOUT_RESULT            return(0);
#define SET_SCALE(Value)          gtk_adjustment_set_value(GTK_ADJUSTMENT(prf->adj), Value)
#define GET_SCALE()               (GTK_ADJUSTMENT(prf->adj)->value * prf->scale_max)
#define SET_SENSITIVE(Wid, Val)   gtk_widget_set_sensitive(Wid, Val)
#define black_text(Prf)
#define red_text(Prf)
/* gdk_gc_set_foreground(Prf->label->style->black_gc, ss->sgx->red) no effect? */

static void set_radio_button(prefs_info *prf, int which)
{
  if ((which >= 0) && (which < prf->num_buttons))
    set_toggle_button(prf->radio_buttons[which], true, false, (void *)prf);
}

#define which_radio_button(Prf)   get_user_int_data(G_OBJECT(Prf->radio_button))

#include "snd-prefs.c"


static void sg_entry_set_text(GtkEntry* entry, const char *text)
{
  if (text)
    gtk_entry_set_text(entry, (gchar *)text);
  else gtk_entry_set_text(entry, " ");
}

static void set_text(GtkWidget *w, char *value)
{
  if (GTK_IS_ENTRY(w))
    sg_entry_set_text(GTK_ENTRY(w), value);
  else
    {
      if (GTK_IS_ENTRY(GTK_BIN(w)->child))
	sg_entry_set_text(GTK_ENTRY(GTK_BIN(w)->child), value);
#if MUS_DEBUGGING
      else
	{
	  fprintf(stderr,"oops: %s\n", value);
	  abort();
	}
#endif
    }
}

static char *get_text(GtkWidget *w)
{
  if (GTK_IS_ENTRY(w))
    return((char *)gtk_entry_get_text(GTK_ENTRY(w)));
  if (GTK_IS_ENTRY(GTK_BIN(w)->child))
    return((char *)gtk_entry_get_text(GTK_ENTRY(GTK_BIN(w)->child)));
#if MUS_DEBUGGING
  fprintf(stderr,"get oops");
  abort();
#endif
  return(NULL);
}

  


/* ---------------- help strings ---------------- */

static gboolean prefs_help_click_callback(GtkWidget *w, GdkEventButton *ev, gpointer context)
{
  prefs_help((prefs_info *)context);
  return(false);
}

static gboolean prefs_tooltip_help(gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if (help_dialog_is_active())
    prefs_help(prf);
  else prefs_helping = false;
  prf->help_id = 0;
  return(false);
}

static gboolean mouse_enter_pref_callback(GtkWidget *w, GdkEventCrossing *ev, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if (prefs_helping)
    prf->help_id = g_timeout_add_full(0,
				      HELP_WAIT_TIME,
				      prefs_tooltip_help,
				      context, NULL);
  return(false);
}

static gboolean mouse_leave_pref_callback(GtkWidget *w, GdkEventCrossing *ev, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if (prf->help_id != 0)
    {
      g_source_remove(prf->help_id);
      prf->help_id = 0;
    }
  return(false);
}

static bool prefs_dialog_error_is_posted = false;

static void post_prefs_dialog_error(const char *message, void *data)
{
  gtk_window_set_title(GTK_WINDOW(preferences_dialog), (char *)message);
  prefs_dialog_error_is_posted = (message != NULL);
}

static void clear_prefs_dialog_error(void)
{
  if (prefs_dialog_error_is_posted)
    {
      prefs_dialog_error_is_posted = false;
      post_prefs_dialog_error(NULL, NULL);
    }
}

static void prefs_change_callback(GtkWidget *w, gpointer context)
{
  prefs_unsaved = true;
  prefs_set_dialog_title(NULL);
  clear_prefs_dialog_error();
}


static GtkSizeGroup *label_group;
static GtkSizeGroup *help_group;
static GtkSizeGroup *widgets_group;

static GdkColor *rscl_color, *gscl_color, *bscl_color;

#define PACK_1 true
#define PACK_2 false


/* ---------------- row (main) label widget ---------------- */

static GtkWidget *make_row_label(prefs_info *prf, const char *label, GtkWidget *box)
{
  GtkWidget *w, *ev;

  ev = gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(box), ev, PACK_1, PACK_2, 0);
  gtk_widget_show(ev);

  w = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.0);
  gtk_size_group_add_widget(label_group, w);
  gtk_container_add(GTK_CONTAINER(ev), w);
  gtk_widget_show(w);
  prf->saved_label = label;

  SG_SIGNAL_CONNECT(ev, "button_press_event", prefs_help_click_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);

  return(w);
}

/* ---------------- row inner label widget ---------------- */

static GtkWidget *make_row_inner_label(prefs_info *prf, const char *label, GtkWidget *box)
{
  GtkWidget *w, *ev;
  ASSERT_WIDGET_TYPE(GTK_IS_HBOX(box), box);

  ev = gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(box), ev, false, false, 4);
  gtk_widget_show(ev);

  w = gtk_label_new(label);
  gtk_container_add(GTK_CONTAINER(ev), w);
  gtk_widget_show(w);

  SG_SIGNAL_CONNECT(ev, "button_press_event", prefs_help_click_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);

  return(w);
}

/* ---------------- row middle separator widget ---------------- */

static GtkWidget *make_row_middle_separator(GtkWidget *box)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_HBOX(box), box);
  w = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(box), w, false, false, 10);
  gtk_widget_show(w);
  return(w);
}

/* ---------------- row inner separator widget ---------------- */

static GtkWidget *make_row_inner_separator(int width, GtkWidget *box)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_HBOX(box), box);
  w = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(box), w, false, false, width);
  gtk_widget_show(w);
  return(w);
}

/* ---------------- row help widget ---------------- */

static GtkWidget *make_row_help(prefs_info *prf, const char *label, GtkWidget *box)
{
  GtkWidget *w, *ev;

  ev = gtk_event_box_new();
  gtk_box_pack_end(GTK_BOX(box), ev, PACK_1, PACK_2, 0);
  gtk_widget_show(ev);

  w = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.0);
  gtk_container_add(GTK_CONTAINER(ev), w);
  gtk_size_group_add_widget(help_group, w);

  gtk_widget_show(w);

  SG_SIGNAL_CONNECT(ev, "button_press_event", prefs_help_click_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);

  return(w);
}

/* ---------------- row toggle widget ---------------- */

static GtkWidget *make_row_toggle_with_label(prefs_info *prf, bool current_value, GtkWidget *box, const char *label)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_HBOX(box), box);
  if (label)
    w = gtk_check_button_new_with_label(label);
  else w = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), current_value);
  gtk_box_pack_start(GTK_BOX(box), w, false, false, 0); /* was 10 */
  gtk_widget_show(w);

  SG_SIGNAL_CONNECT(w, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(w, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(w, "toggled", prefs_change_callback, NULL);

  return(w);
}

static GtkWidget *make_row_toggle(prefs_info *prf, bool current_value, GtkWidget *box)
{
  return(make_row_toggle_with_label(prf, current_value, box, NULL));
}


/* ---------------- error widget ---------------- */

static GtkWidget *make_row_error(prefs_info *prf, GtkWidget *box)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_HBOX(box), box);

  w = gtk_label_new("");
  gtk_box_pack_end(GTK_BOX(box), w, true, false, 0);
  gtk_widget_show(w);

  SG_SIGNAL_CONNECT(w, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(w, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  
  return(w);
}

/* ---------------- row arrows ---------------- */

static gboolean remove_arrow_func(GtkWidget *w, GdkEventButton *ev, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if (prf->power_id != 0)
    {
      g_source_remove(prf->power_id);
      prf->power_id = 0;
    }
  return(false);
}

static gint arrow_func_up(gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if (GTK_WIDGET_IS_SENSITIVE(prf->arrow_up))
    {
      if ((prf) && (prf->arrow_up_func))
	{
	  (*(prf->arrow_up_func))(prf);
	  prf->power_id = g_timeout_add_full(0,
					     POWER_WAIT_TIME,
					     arrow_func_up,
					     (gpointer)prf, NULL);
	}
      else prf->power_id = 0;
    }
  return(0);
}

static gint arrow_func_down(gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if (GTK_WIDGET_IS_SENSITIVE(prf->arrow_down))
    {
      if ((prf) && (prf->arrow_down_func))
	{
	  (*(prf->arrow_down_func))(prf);
	  prf->power_id = g_timeout_add_full(0,
					     POWER_WAIT_TIME,
					     arrow_func_down,
					     (gpointer)prf, NULL);
	}
      else prf->power_id = 0;
    }
  return(0);
}

static gboolean call_arrow_down_press(GtkWidget *w, GdkEventButton *ev, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->arrow_down_func))
    {
      (*(prf->arrow_down_func))(prf);
      if (GTK_WIDGET_IS_SENSITIVE(w))
	prf->power_id = g_timeout_add_full(0,
					   POWER_INITIAL_WAIT_TIME,
					   arrow_func_down,
					   (gpointer)prf, NULL);
      else prf->power_id = 0;
    }
  return(false);
}

static gboolean call_arrow_up_press(GtkWidget *w, GdkEventButton *ev, gpointer context) 
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->arrow_up_func))
    {
      (*(prf->arrow_up_func))(prf);
      if (GTK_WIDGET_IS_SENSITIVE(w))
	prf->power_id = g_timeout_add_full(0,
					   POWER_INITIAL_WAIT_TIME,
					   arrow_func_up,
					   (gpointer)prf, NULL);
      else prf->power_id = 0;
    }
  return(false);
}

static GtkWidget *make_row_arrows(prefs_info *prf, GtkWidget *box)
{
  GtkWidget *ev_up, *ev_down, *up, *down;

  ev_down = gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(box), ev_down, false, false, 0);
  gtk_widget_show(ev_down);

  down = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(ev_down), down);
  gtk_widget_show(down);

  ev_up = gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(box), ev_up, false, false, 0);
  gtk_widget_show(ev_up);

  up = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_ETCHED_OUT);
  gtk_container_add(GTK_CONTAINER(ev_up), up);
  gtk_widget_show(up);

  prf->arrow_up = up;
  prf->arrow_down = down;

  SG_SIGNAL_CONNECT(ev_down, "button_press_event", call_arrow_down_press, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev_down, "button_release_event", remove_arrow_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev_up, "button_press_event", call_arrow_up_press, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev_up, "button_release_event", remove_arrow_func, (gpointer)prf);

  SG_SIGNAL_CONNECT(ev_up, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev_down, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev_up, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(ev_down, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);

  SG_SIGNAL_CONNECT(ev_down, "button_press_event", prefs_change_callback, NULL);
  SG_SIGNAL_CONNECT(ev_up, "button_press_event", prefs_change_callback, NULL);

  return(up);
}


/* ---------------- bool row ---------------- */

static void call_toggle_func(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->toggle_func))
    (*(prf->toggle_func))(prf);
}

static prefs_info *prefs_row_with_toggle(const char *label, const char *varname, bool current_value,
					 GtkWidget *box,
					 void (*toggle_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *hb, *row, *help;

  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(box), box);
  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->toggle_func = toggle_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->toggle = make_row_toggle(prf, current_value, hb);
  help = make_row_help(prf, varname, row);

  SG_SIGNAL_CONNECT(prf->toggle, "toggled", call_toggle_func, (gpointer)prf);

  return(prf);
}


/* ---------------- two toggles ---------------- */

static void call_toggle2_func(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->toggle2_func))
    (*(prf->toggle2_func))(prf);
}

static prefs_info *prefs_row_with_two_toggles(const char *label, const char *varname, 
					      const char *label1, bool value1,
					      const char *label2, bool value2,
					      GtkWidget *box,
					      void (*toggle_func)(prefs_info *prf),
					      void (*toggle2_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *help, *sep1, *row, *hb;

  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(box), box);
  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->toggle_func = toggle_func;
  prf->toggle2_func = toggle2_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->toggle = make_row_toggle_with_label(prf, value1, hb, label1);
  sep1 = make_row_inner_separator(20, hb);
  prf->toggle2 = make_row_toggle_with_label(prf, value2, hb, label2);
  help = make_row_help(prf, varname, row);

  SG_SIGNAL_CONNECT(prf->toggle, "toggled", call_toggle_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->toggle2, "toggled", call_toggle2_func, (gpointer)prf);

  return(prf);
}



/* ---------------- toggle with text ---------------- */

static GtkWidget *make_row_text(prefs_info *prf, const char *text_value, int cols, GtkWidget *box)
{
  int len;
  GtkWidget *w;
  GtkSettings *settings;
  ASSERT_WIDGET_TYPE(GTK_IS_HBOX(box), box);
  len = snd_strlen(text_value);
  w = gtk_entry_new();
  gtk_entry_set_has_frame(GTK_ENTRY(w), true);
  if (text_value) sg_entry_set_text(GTK_ENTRY(w), text_value);
  gtk_entry_set_has_frame(GTK_ENTRY(w), false);
  if (cols > 0)
    gtk_entry_set_width_chars(GTK_ENTRY(w), cols);
  else
    {
      if (len > 24) /* sigh... */
	gtk_entry_set_width_chars(GTK_ENTRY(w), len);
    }
  gtk_editable_set_editable(GTK_EDITABLE(w), true);
  settings = gtk_widget_get_settings(w);
  g_object_set(settings, "gtk-entry-select-on-focus", false, NULL);
  gtk_box_pack_start(GTK_BOX(box), w, false, false, 0);
  gtk_widget_show(w);

  SG_SIGNAL_CONNECT(w, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(w, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(w, "activate", prefs_change_callback, NULL);

  return(w);
}

static void call_text_func(GtkWidget *w, gpointer context) 
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->text_func))
    (*(prf->text_func))(prf);
}

static prefs_info *prefs_row_with_toggle_with_text(const char *label, const char *varname, bool current_value,
						   const char *text_label, const char *text_value, int cols,
						   GtkWidget *box,
						   void (*toggle_func)(prefs_info *prf),
						   void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *sep1, *lab1, *hb, *row, *help;

  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->toggle_func = toggle_func;
  prf->text_func = text_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->toggle = make_row_toggle(prf, current_value, hb);
  sep1 = make_row_inner_separator(16, hb);
  lab1 = make_row_inner_label(prf, text_label, hb);
  prf->text= make_row_text(prf, text_value, cols, hb);
  help = make_row_help(prf, varname, row);

  SG_SIGNAL_CONNECT(prf->toggle, "toggled", call_toggle_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);

  return(prf);
}

static prefs_info *prefs_row_with_toggle_with_two_texts(const char *label, const char *varname, bool current_value,
							const char *label1, const char *text1, 
							const char *label2, const char *text2, int cols,
							GtkWidget *box,
							void (*toggle_func)(prefs_info *prf),
							void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *sep1, *lab1, *lab2, *hb, *row, *help;
  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->toggle_func = toggle_func;
  prf->text_func = text_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->toggle = make_row_toggle(prf, current_value, hb);
  sep1 = make_row_inner_separator(16, hb);
  lab1 = make_row_inner_label(prf, label1, hb);
  prf->text= make_row_text(prf, text1, cols, hb);
  lab2 = make_row_inner_label(prf, label2, hb);
  prf->rtxt= make_row_text(prf, text2, cols, hb);
  help = make_row_help(prf, varname, row);

  SG_SIGNAL_CONNECT(prf->toggle, "toggled", call_toggle_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->rtxt, "activate", call_text_func, (gpointer)prf);

  return(prf);
}



/* ---------------- text with toggle ---------------- */

static prefs_info *prefs_row_with_text_with_toggle(const char *label, const char *varname, bool current_value,
						   const char *toggle_label, const char *text_value, int cols,
						   GtkWidget *box,
						   void (*toggle_func)(prefs_info *prf),
						   void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *sep1, *lab1, *hb, *row, *help;

  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->toggle_func = toggle_func;
  prf->text_func = text_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->text = make_row_text(prf, text_value, cols, hb);
  sep1 = make_row_inner_separator(8, hb);
  lab1 = make_row_inner_label(prf, toggle_label, hb);
  prf->toggle = make_row_toggle(prf, current_value, hb);  
  help = make_row_help(prf, varname, row);
  
  SG_SIGNAL_CONNECT(prf->toggle, "toggled", call_toggle_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);

  return(prf);
}


/* ---------------- text with three toggle ---------------- */

static prefs_info *prefs_row_with_text_and_three_toggles(const char *label, const char *varname,
							 const char *text_label, int cols,
							 const char *toggle1_label, const char *toggle2_label, const char *toggle3_label,
							 const char *text_value, 
							 bool toggle1_value, bool toggle2_value, bool toggle3_value,
							 GtkWidget *box,
							 void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *sep1, *sep2, *sep3, *lab1, *lab2, *lab3, *lab4, *hb, *row, *help;

  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->text_func = text_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  lab4 = make_row_inner_label(prf, text_label, hb);
  prf->text = make_row_text(prf, text_value, cols, hb);
  sep1 = make_row_inner_separator(12, hb);
  lab1 = make_row_inner_label(prf, toggle1_label, hb);
  prf->toggle = make_row_toggle(prf, toggle1_value, hb);  
  sep2 = make_row_inner_separator(4, hb);
  lab2 = make_row_inner_label(prf, toggle2_label, hb);
  prf->toggle2 = make_row_toggle(prf, toggle2_value, hb);  
  sep3 = make_row_inner_separator(4, hb);
  lab3 = make_row_inner_label(prf, toggle3_label, hb);
  prf->toggle3 = make_row_toggle(prf, toggle3_value, hb);  
  help = make_row_help(prf, varname, row);
  
  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);
  return(prf);
}


/* ---------------- radio row ---------------- */

static void call_radio_func(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->toggle_func))
    {
      prf->radio_button = w;
      (*(prf->toggle_func))(prf);
    }
}

static GtkWidget *make_row_radio_box(prefs_info *prf,
				     const char **labels, int num_labels, int current_value,
				     GtkWidget *box)
{
  GtkWidget *w, *current_button;
  int i;

  w = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), w, false, false, 0);
  gtk_widget_show(w);

  prf->radio_buttons = (GtkWidget **)CALLOC(num_labels, sizeof(GtkWidget *));
  prf->num_buttons = num_labels;

  for (i = 0; i < num_labels; i++)
    {
      if (i == 0)
	current_button = gtk_radio_button_new_with_label(NULL, labels[i]);
      else current_button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(prf->radio_buttons[0])), labels[i]);
      prf->radio_buttons[i] = current_button;
      gtk_box_pack_start(GTK_BOX(w), current_button, false, false, 0);
      set_user_int_data(G_OBJECT(current_button), i);
      gtk_widget_show(current_button);
      SG_SIGNAL_CONNECT(current_button, "clicked", call_radio_func, (gpointer)prf);
      SG_SIGNAL_CONNECT(current_button, "clicked", prefs_change_callback, NULL);
      SG_SIGNAL_CONNECT(current_button, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
      SG_SIGNAL_CONNECT(current_button, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
    }

  if (current_value != -1)
    set_toggle_button(prf->radio_buttons[current_value], true, false, (void *)prf);

  return(w);
}
  

static prefs_info *prefs_row_with_radio_box(const char *label, const char *varname, 
					    const char **labels, int num_labels, int current_value,
					    GtkWidget *box,
					    void (*toggle_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *hb, *row, *help;
  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->toggle_func = toggle_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);

  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->toggle = make_row_radio_box(prf, labels, num_labels, current_value, hb);
  help = make_row_help(prf, varname, row);

  return(prf);
}

static prefs_info *prefs_row_with_radio_box_and_number(const char *label, const char *varname, 
						       const char **labels, int num_labels, int current_value,
						       int number, const char *text_value, int text_cols,
						       GtkWidget *box,
						       void (*toggle_func)(prefs_info *prf),
						       void (*arrow_up_func)(prefs_info *prf), void (*arrow_down_func)(prefs_info *prf), 
						       void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *help, *row, *hb;
  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->toggle_func = toggle_func;
  prf->text_func = text_func;
  prf->arrow_up_func = arrow_up_func;
  prf->arrow_down_func = arrow_down_func;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);

  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->toggle = make_row_radio_box(prf, labels, num_labels, current_value, hb);
  prf->text = make_row_text(prf, text_value, text_cols, hb);
  prf->arrow_up = make_row_arrows(prf, hb);
  help = make_row_help(prf, varname, row);

  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);

  return(prf);
}



/* ---------------- scale row ---------------- */

static void call_scale_func(GtkAdjustment *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->scale_func))
    (*(prf->scale_func))(prf);
}

static void call_scale_text_func(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->text_func))
    (*(prf->text_func))(prf);
}

static void prefs_scale_callback(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  float_to_textfield(prf->text, GTK_ADJUSTMENT(prf->adj)->value * prf->scale_max);
}

static prefs_info *prefs_row_with_scale(const char *label, const char *varname, 
					Float max_val, Float current_value,
					GtkWidget *box,
					void (*scale_func)(prefs_info *prf),
					void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *hb, *row, *help;
  char *str;

  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  prf->scale_max = max_val;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  
  str = (char *)CALLOC(12, sizeof(char));
  mus_snprintf(str, 12, "%.3f", current_value);
  prf->text = make_row_text(prf, str, 6, hb);
  FREE(str);

  prf->adj = gtk_adjustment_new(current_value /max_val, 0.0, 1.01, 0.001, 0.01, .01);
  prf->scale = gtk_hscale_new(GTK_ADJUSTMENT(prf->adj));
  gtk_box_pack_start(GTK_BOX(hb), prf->scale, true, true, 4);
  gtk_widget_show(prf->scale);
  gtk_range_set_update_policy(GTK_RANGE(GTK_SCALE(prf->scale)), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_draw_value(GTK_SCALE(prf->scale), false);
  
  help = make_row_help(prf, varname, row);

  prf->scale_func = scale_func;
  prf->text_func = text_func;

  SG_SIGNAL_CONNECT(prf->scale, "value_changed", call_scale_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->scale, "value_changed", prefs_change_callback, NULL);
  SG_SIGNAL_CONNECT(prf->scale, "value_changed", prefs_scale_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->text, "activate", call_scale_text_func, (gpointer)prf);

  SG_SIGNAL_CONNECT(prf->scale, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->scale, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);

  return(prf);
}


/* ---------------- text row ---------------- */

static prefs_info *prefs_row_with_text(const char *label, const char *varname, const char *value,
				       GtkWidget *box,
				       void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *hb, *row, *help;

  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(box), box);
  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->text = make_row_text(prf, value, 0, hb);
  help = make_row_help(prf, varname, row);

  prf->text_func = text_func;
  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);

  return(prf);
}


/* ---------------- two texts in a row ---------------- */

static prefs_info *prefs_row_with_two_texts(const char *label, const char *varname,
					    const char *label1, const char *text1, const char *label2, const char *text2, int cols,
					    GtkWidget *box,
					    void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *lab1, *lab2, *hb, *row, *help;
  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(box), box);
  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  lab1 = make_row_inner_label(prf, label1, hb);
  prf->text = make_row_text(prf, text1, cols, hb);
  lab2 = make_row_inner_label(prf, label2, hb);  
  prf->rtxt = make_row_text(prf, text2, cols, hb);
  help = make_row_help(prf, varname, row);

  prf->text_func = text_func;

  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->rtxt, "activate", call_text_func, (gpointer)prf);

  return(prf);
}

/* ---------------- number row ---------------- */

static prefs_info *prefs_row_with_number(const char *label, const char *varname, const char *value, int cols,
					 GtkWidget *box,
 					 void (*arrow_up_func)(prefs_info *prf), void (*arrow_down_func)(prefs_info *prf), 
					 void (*text_func)(prefs_info *prf))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *hb, *row, *help;

  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);

  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);
  prf->text = make_row_text(prf, value, cols, hb);
  prf->arrow_up = make_row_arrows(prf, hb);
  prf->error = make_row_error(prf, hb);
  help = make_row_help(prf, varname, row);

  prf->text_func = text_func;
  prf->arrow_up_func = arrow_up_func;
  prf->arrow_down_func = arrow_down_func;

  SG_SIGNAL_CONNECT(prf->text, "activate", call_text_func, (gpointer)prf);
  return(prf);
}


/* ---------------- list row ---------------- */

#if HAVE_GTK_COMBO_BOX_ENTRY_NEW_TEXT
static prefs_info *prefs_row_with_list(const char *label, const char *varname, const char *value,
				       const char **values, int num_values,
				       GtkWidget *box,
				       void (*text_func)(prefs_info *prf),
				       char *(*completion_func)(char *text, void *context), void *completion_context)
{
  int i;
  prefs_info *prf = NULL;
  GtkWidget *sep, *hb, *row, *help;

  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;

  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);  
  
  prf->text = gtk_combo_box_entry_new_text();
  for (i = 0; i < num_values; i++)
    gtk_combo_box_append_text(GTK_COMBO_BOX(prf->text), (values[i]) ? values[i] : " ");
  sg_entry_set_text(GTK_ENTRY(GTK_BIN(prf->text)->child), value);
  gtk_box_pack_start(GTK_BOX(hb), prf->text, false, false, 4);
  gtk_widget_show(prf->text);

  prf->error = make_row_error(prf, hb);
  help = make_row_help(prf, varname, row);

  prf->text_func = text_func;
  SG_SIGNAL_CONNECT(prf->text, "changed", call_text_func, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->text, "changed", prefs_change_callback, NULL);

  SG_SIGNAL_CONNECT(prf->text, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->text, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  return(prf);
}
#endif

/* ---------------- color selector row(s) ---------------- */

static color_t rgb_to_color(Float r, Float g, Float b)
{
  GdkColor gcolor;
  GdkColor *ccolor;
  gcolor.red = (unsigned short)(65535 * r);
  gcolor.green = (unsigned short)(65535 * g);
  gcolor.blue = (unsigned short)(65535 * b);
  ccolor = gdk_color_copy(&gcolor);
  gdk_rgb_find_color(gdk_colormap_get_system(), ccolor);
  return(ccolor);
}

static void pixel_to_rgb(color_t pix, float *r, float *g, float *b)
{
  (*r) = (float)(pix->red) / 65535.0;
  (*g) = (float)(pix->green) / 65535.0;
  (*b) = (float)(pix->blue) / 65535.0;
}

static void scale_set_color(prefs_info *prf, color_t pixel)
{
  float r = 0.0, g = 0.0, b = 0.0;
  pixel_to_rgb(pixel, &r, &g, &b);
  float_to_textfield(prf->rtxt, r);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(prf->radj), r);
  float_to_textfield(prf->gtxt, g);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(prf->gadj), g);
  float_to_textfield(prf->btxt, b);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(prf->badj), b);
  gtk_widget_modify_bg(prf->color, GTK_STATE_NORMAL, pixel);
}

static void reflect_color(prefs_info *prf)
{
  Float r, g, b;
  GdkColor *current_color;

  r = GTK_ADJUSTMENT(prf->radj)->value;
  g = GTK_ADJUSTMENT(prf->gadj)->value;
  b = GTK_ADJUSTMENT(prf->badj)->value;

  current_color = rgb_to_color(r, g, b);
  gtk_widget_modify_bg(prf->color, GTK_STATE_NORMAL, current_color);

  r = current_color->red / 65535.0;
  g = current_color->green / 65535.0;
  b = current_color->blue / 65535.0;

  float_to_textfield(prf->rtxt, r);
  float_to_textfield(prf->gtxt, g);
  float_to_textfield(prf->btxt, b);
}

static void prefs_color_callback(GtkWidget *w, gpointer context)
{
  reflect_color((prefs_info *)context);
}

static gint unpost_color_error(gpointer data)
{
  prefs_info *prf = (prefs_info *)data;
  prf->got_error = false;
  gtk_label_set_text(GTK_LABEL(prf->label), prf->saved_label);
  reflect_color(prf);
  return(0);
}

static void errors_to_color_text(const char *msg, void *data)
{
  prefs_info *prf = (prefs_info *)data;
  prf->got_error = true;
  gtk_label_set_text(GTK_LABEL(prf->label), msg);
  TIMEOUT(unpost_color_error);
}

static void prefs_r_callback(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  char *str;
  float r = 0.0;
  str = (char *)gtk_entry_get_text(GTK_ENTRY(w));
  redirect_errors_to(errors_to_color_text, context);
  r = (float)string_to_Float(str, 0.0, "red amount");
  redirect_errors_to(NULL, NULL);
  if (!(prf->got_error))
    {
      gtk_adjustment_set_value(GTK_ADJUSTMENT(prf->radj), (float)mus_fclamp(0.0, r, 1.0));
      reflect_color(prf);
    }
}

static void prefs_g_callback(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  char *str;
  float r = 0.0;
  str = (char *)gtk_entry_get_text(GTK_ENTRY(w));
  redirect_errors_to(errors_to_color_text, context);
  r = (float)string_to_Float(str, 0.0, "green amount");
  redirect_errors_to(NULL, NULL);
  if (!(prf->got_error))
    {
      gtk_adjustment_set_value(GTK_ADJUSTMENT(prf->gadj), (float)mus_fclamp(0.0, r, 1.0));
      reflect_color(prf);
    }
}

static void prefs_b_callback(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  char *str;
  float r = 0.0;
  str = (char *)gtk_entry_get_text(GTK_ENTRY(w));
  redirect_errors_to(errors_to_color_text, context);
  r = (float)string_to_Float(str, 0.0, "blue amount");
  redirect_errors_to(NULL, NULL);
  if (!(prf->got_error))
    {
      gtk_adjustment_set_value(GTK_ADJUSTMENT(prf->badj), (float)mus_fclamp(0.0, r, 1.0));
      reflect_color(prf);
    }
}

static void prefs_call_color_func_callback(GtkWidget *w, gpointer context)
{
  prefs_info *prf = (prefs_info *)context;
  if ((prf) && (prf->color_func))
    {
      float r, g, b;
      r = GTK_ADJUSTMENT(prf->radj)->value;
      g = GTK_ADJUSTMENT(prf->gadj)->value;
      b = GTK_ADJUSTMENT(prf->badj)->value;
      (*(prf->color_func))(prf, r, g, b);
    }
}

static prefs_info *prefs_color_selector_row(const char *label, const char *varname, 
					    color_t current_pixel,
					    GtkWidget *box,
					    void (*color_func)(prefs_info *prf, float r, float g, float b))
{
  prefs_info *prf = NULL;
  GtkWidget *sep, *sep1, *hb, *row, *row2, *sep2, *sep3, *help;
  float r = 0.0, g = 0.0, b = 0.0;

  prf = (prefs_info *)CALLOC(1, sizeof(prefs_info));
  prf->var_name = varname;
  pixel_to_rgb(current_pixel, &r, &g, &b);

  /* first row */
  row = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row, false, false, 0);
  gtk_widget_show(row);

  prf->label = make_row_label(prf, label, row);
  hb = gtk_hbox_new(false, 0);
  gtk_size_group_add_widget(widgets_group, hb);
  gtk_box_pack_start(GTK_BOX(row), hb, false, false, 0);
  gtk_widget_show(hb);

  sep = make_row_middle_separator(hb);    

  prf->color_texts = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  prf->color = gtk_drawing_area_new();
  gtk_widget_set_events(prf->color, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  gtk_box_pack_start(GTK_BOX(hb), prf->color, false, false, 4);
  gtk_size_group_add_widget(prf->color_texts, prf->color);
  gtk_widget_modify_bg(prf->color, GTK_STATE_NORMAL, current_pixel);
  gtk_widget_show(prf->color);
  
  sep1 = make_row_inner_separator(8, hb);
  
  prf->rtxt = make_row_text(prf, NULL, 6, hb);
  gtk_size_group_add_widget(prf->color_texts, prf->rtxt);
  float_to_textfield(prf->rtxt, r);

  prf->gtxt = make_row_text(prf, NULL, 6, hb);
  gtk_size_group_add_widget(prf->color_texts, prf->gtxt);
  float_to_textfield(prf->gtxt, g);

  prf->btxt = make_row_text(prf, NULL, 6, hb);
  gtk_size_group_add_widget(prf->color_texts, prf->btxt);
  float_to_textfield(prf->btxt, b);
  help = make_row_help(prf, varname, row);

  /* second row */

  row2 = gtk_hbox_new(false, 0);
  gtk_box_pack_start(GTK_BOX(box), row2, false, false, 0);
  gtk_widget_show(row2);

  sep2 = make_row_inner_separator(20, row2);

  prf->radj = gtk_adjustment_new(r, 0.0, 1.01, 0.001, 0.01, .01);
  prf->rscl = gtk_hscale_new(GTK_ADJUSTMENT(prf->radj));
  gtk_box_pack_start(GTK_BOX(row2), prf->rscl, true, true, 4);
  /* normal = slider, active = trough, selected unused */
  gtk_widget_modify_bg(prf->rscl, GTK_STATE_NORMAL, rscl_color);
  gtk_widget_modify_bg(prf->rscl, GTK_STATE_PRELIGHT, rscl_color);
  gtk_widget_show(prf->rscl);
  gtk_range_set_update_policy(GTK_RANGE(GTK_SCALE(prf->rscl)), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_draw_value(GTK_SCALE(prf->rscl), false);

  prf->gadj = gtk_adjustment_new(g, 0.0, 1.01, 0.001, 0.01, .01);
  prf->gscl = gtk_hscale_new(GTK_ADJUSTMENT(prf->gadj));
  gtk_box_pack_start(GTK_BOX(row2), prf->gscl, true, true, 4);
  gtk_widget_modify_bg(prf->gscl, GTK_STATE_NORMAL, gscl_color);
  gtk_widget_modify_bg(prf->gscl, GTK_STATE_PRELIGHT, gscl_color);
  gtk_widget_show(prf->gscl);
  gtk_range_set_update_policy(GTK_RANGE(GTK_SCALE(prf->gscl)), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_draw_value(GTK_SCALE(prf->gscl), false);

  prf->badj = gtk_adjustment_new(b, 0.0, 1.01, 0.001, 0.01, .01);
  prf->bscl = gtk_hscale_new(GTK_ADJUSTMENT(prf->badj));
  gtk_box_pack_start(GTK_BOX(row2), prf->bscl, true, true, 4);
  gtk_widget_modify_bg(prf->bscl, GTK_STATE_NORMAL, bscl_color);
  gtk_widget_modify_bg(prf->bscl, GTK_STATE_PRELIGHT, bscl_color);
  gtk_widget_show(prf->bscl);
  gtk_range_set_update_policy(GTK_RANGE(GTK_SCALE(prf->bscl)), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_draw_value(GTK_SCALE(prf->bscl), false);

  sep3 = gtk_hseparator_new();
  gtk_box_pack_end(GTK_BOX(row2), sep3, false, false, 20);
  gtk_widget_show(sep3);

  SG_SIGNAL_CONNECT(prf->rtxt, "activate", prefs_r_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->gtxt, "activate", prefs_g_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->btxt, "activate", prefs_b_callback, (gpointer)prf);

  SG_SIGNAL_CONNECT(prf->radj, "value_changed", prefs_call_color_func_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->gadj, "value_changed", prefs_call_color_func_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->badj, "value_changed", prefs_call_color_func_callback, (gpointer)prf);

  SG_SIGNAL_CONNECT(prf->radj, "value_changed", prefs_change_callback, NULL);
  SG_SIGNAL_CONNECT(prf->gadj, "value_changed", prefs_change_callback, NULL);
  SG_SIGNAL_CONNECT(prf->badj, "value_changed", prefs_change_callback, NULL);

  SG_SIGNAL_CONNECT(prf->radj, "value_changed", prefs_color_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->gadj, "value_changed", prefs_color_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->badj, "value_changed", prefs_color_callback, (gpointer)prf);

  SG_SIGNAL_CONNECT(prf->color, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->color, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->rscl, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->rscl, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->gscl, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->gscl, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->bscl, "enter_notify_event", mouse_enter_pref_callback, (gpointer)prf);
  SG_SIGNAL_CONNECT(prf->bscl, "leave_notify_event", mouse_leave_pref_callback, (gpointer)prf);

  prf->color_func = color_func;

  return(prf);
}


/* ---------------- topic separator ---------------- */

static GtkWidget *make_inter_topic_separator(GtkWidget *topics)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(topics), topics);
  w = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(topics), w, false, false, 0);
  gtk_widget_show(w);
  return(w);
  /* height = INTER_TOPIC_SPACE no line */
}

/* ---------------- variable separator ---------------- */

static GtkWidget *make_inter_variable_separator(GtkWidget *topics)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(topics), topics);
  w = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(topics), w, false, false, 0);
  gtk_widget_show(w);
  return(w);
  /* height = INTER_VARIABLE_SPACE no line */
}

/* ---------------- top-level contents label ---------------- */

static GtkWidget *make_top_level_label(const char *label, GtkWidget *parent)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(parent), parent);
#if HAVE_GTK_BUTTON_SET_ALIGNMENT
  w = snd_gtk_highlight_label_new(label);
  gtk_button_set_alignment(GTK_BUTTON(w), 0.01, 0.5);
  gtk_widget_modify_bg(w, GTK_STATE_NORMAL, ss->sgx->light_blue);
  gtk_widget_modify_bg(w, GTK_STATE_PRELIGHT, ss->sgx->light_blue);
#else
  w = snd_gtk_entry_label_new(label, ss->sgx->light_blue);
#endif
  gtk_box_pack_start(GTK_BOX(parent), w, false, false, 0);
  gtk_widget_show(w);
  return(w);
}

static GtkWidget *make_top_level_box(GtkWidget *topics)
{
  GtkWidget *w, *frame;
  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(topics), topics);
  frame = gtk_frame_new(NULL);
  gtk_box_pack_start(GTK_BOX(topics), frame, true, true, 0);
  gtk_widget_show(frame);
  w = gtk_vbox_new(false, 0);
  gtk_container_add(GTK_CONTAINER(frame), w);
  gtk_widget_show(w);
  return(w);
}

static GtkWidget *make_inner_label(const char *label, GtkWidget *parent)
{
  GtkWidget *w;
  ASSERT_WIDGET_TYPE(GTK_IS_VBOX(parent), parent);
#if HAVE_GTK_BUTTON_SET_ALIGNMENT
  w = snd_gtk_highlight_label_new(label);
  gtk_button_set_alignment(GTK_BUTTON(w), 0.0, 0.5);
#else
  w = snd_gtk_entry_label_new(label, ss->sgx->highlight_color);
#endif
  gtk_box_pack_start(GTK_BOX(parent), w, false, false, 0);
  gtk_widget_show(w);
  return(w);
}


/* ---------------- base buttons ---------------- */

static gint preferences_delete_callback(GtkWidget *w, GdkEvent *event, gpointer context)
{
  prefs_helping = false;
  clear_prefs_dialog_error();
  gtk_widget_hide(preferences_dialog);
  return(true);
}

static void preferences_dismiss_callback(GtkWidget *w, gpointer context) 
{
  prefs_helping = false;
  clear_prefs_dialog_error();
  gtk_widget_hide(preferences_dialog);
}

static void preferences_help_callback(GtkWidget *w, gpointer context) 
{
  prefs_helping = true;
  snd_help("preferences",
	   "This dialog sets various global variables. 'Save' then writes the new values \
to ~/.snd_prefs_guile|ruby|forth|gauche so that they take effect the next time you start Snd.  'Revert' resets each variable either to \
its value when the Preferences dialog was started, or to the last saved value.  'Clear' resets each variable to its default value (its \
value when Snd starts, before loading initialization files). 'Help' starts this dialog, and as long as it's active, it will post helpful \
information if the mouse lingers over some variable -- sort of a tooltip that stays out of your way. \
You can also request help on a given topic by clicking the variable name on the far right.",
	   WITH_WORD_WRAP);
}

static void prefs_set_dialog_title(const char *filename)
{
  char *str;
  if (filename)
    {
      if (prefs_saved_filename) FREE(prefs_saved_filename);
      prefs_saved_filename = copy_string(filename);
    }
  if (prefs_saved_filename)
    str = mus_format("Preferences%s (saved in %s)\n",
		     (prefs_unsaved) ? "*" : "",
		     prefs_saved_filename);
  else str = mus_format("Preferences%s",
			(prefs_unsaved) ? "*" : "");
  gtk_window_set_title(GTK_WINDOW(preferences_dialog), str);
  FREE(str);
}

static void preferences_revert_callback(GtkWidget *w, gpointer context) 
{
  preferences_revert_or_clear(true);
}

static void preferences_clear_callback(GtkWidget *w, gpointer context) 
{
  preferences_revert_or_clear(false);
}

static void preferences_save_callback(GtkWidget *w, gpointer context) 
{
  clear_prefs_dialog_error();
  redirect_snd_error_to(post_prefs_dialog_error, NULL);
  redirect_snd_warning_to(post_prefs_dialog_error, NULL);
  save_prefs(save_options_in_prefs());
  redirect_snd_error_to(NULL, NULL);
  redirect_snd_warning_to(NULL, NULL);
}



/* ---------------- errors ---------------- */

static void clear_prefs_error(GtkWidget *w, gpointer context) 
{
  prefs_info *prf = (prefs_info *)context;
  g_signal_handler_disconnect(prf->text, prf->erase_id);
  prf->erase_id = 0;
  set_label(prf->error, "");
}

static void post_prefs_error(const char *msg, prefs_info *prf)
{
  prf->got_error = true;
  set_label(prf->error, msg);
  if (prf->erase_id != 0)
    g_signal_handler_disconnect(prf->text, prf->erase_id);
  prf->erase_id = SG_SIGNAL_CONNECT(prf->text, "changed", clear_prefs_error, (gpointer)prf);
}

static void va_post_prefs_error(const char *msg, prefs_info *data, ...)
{
  char *buf;
  va_list ap;
  va_start(ap, data);
  buf = vstr(msg, ap);
  va_end(ap);
  post_prefs_error(buf, data);
  FREE(buf);
}


/* ---------------- preferences dialog ---------------- */

widget_t start_preferences_dialog(void)
{
  GtkWidget *saveB, *revertB, *clearB, *helpB, *dismissB, *topics, *scroller, *current_sep;
  prefs_info *prf;
  char *str;

  if (preferences_dialog)
    {
      gtk_widget_show(preferences_dialog);
      return(preferences_dialog);
    }

  preferences_dialog = snd_gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(preferences_dialog), _("Preferences"));
  sg_make_resizable(preferences_dialog);
  /* gtk_container_set_border_width (GTK_CONTAINER(preferences_dialog), 10); */
  gtk_widget_realize(preferences_dialog);

  if ((STARTUP_WIDTH < gdk_screen_width()) &&
      (STARTUP_HEIGHT < gdk_screen_height()))
    gtk_window_resize(GTK_WINDOW(preferences_dialog), STARTUP_WIDTH, STARTUP_HEIGHT);

  helpB = gtk_button_new_from_stock(GTK_STOCK_HELP);
  gtk_widget_set_name(helpB, "help_button");

  saveB = gtk_button_new_from_stock(GTK_STOCK_SAVE);
  gtk_widget_set_name(saveB, "doit_button");

  revertB = gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED);
  gtk_widget_set_name(revertB, "reset_button");

  clearB = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
  gtk_widget_set_name(clearB, "reset_button");

  dismissB = gtk_button_new_from_stock(GTK_STOCK_QUIT);
  gtk_widget_set_name(dismissB, "quit_button");

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(preferences_dialog)->action_area), dismissB, true, true, 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(preferences_dialog)->action_area), revertB, true, true, 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(preferences_dialog)->action_area), clearB, true, true, 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(preferences_dialog)->action_area), saveB, true, true, 10);
  gtk_box_pack_end(GTK_BOX(GTK_DIALOG(preferences_dialog)->action_area), helpB, true, true, 10);

  SG_SIGNAL_CONNECT(preferences_dialog, "delete_event", preferences_delete_callback, NULL);
  SG_SIGNAL_CONNECT(dismissB, "clicked", preferences_dismiss_callback, NULL);
  SG_SIGNAL_CONNECT(revertB, "clicked", preferences_revert_callback, NULL);
  SG_SIGNAL_CONNECT(clearB, "clicked", preferences_clear_callback, NULL);
  SG_SIGNAL_CONNECT(saveB, "clicked", preferences_save_callback, NULL);
  SG_SIGNAL_CONNECT(helpB, "clicked", preferences_help_callback, NULL);

  gtk_widget_show(dismissB);
  gtk_widget_show(saveB);
  gtk_widget_show(revertB);
  gtk_widget_show(clearB);
  gtk_widget_show(helpB);

  topics = gtk_vbox_new(false, 0);
  scroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(preferences_dialog)->vbox), scroller, true, true, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroller), topics);

  gtk_widget_show(topics);
  gtk_widget_show(scroller);

  label_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  widgets_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
  help_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

  rscl_color = rgb_to_color(1.0, 0.5, 0.5);
  gscl_color = rgb_to_color(0.5, 0.9, 0.5);
  bscl_color = rgb_to_color(0.7, 0.8, 0.9);


  /* ---------------- overall behavior ---------------- */

  {
    GtkWidget *dpy_box, *dpy_label, *file_label, *cursor_label, *key_label;
    char *str1, *str2;

    /* ---------------- overall behavior ----------------*/

    dpy_box = make_top_level_box(topics);
    dpy_label = make_top_level_label("overall behavior choices", dpy_box);

    str1 = mus_format("%d", ss->init_window_width);
    str2 = mus_format("%d", ss->init_window_height);
    rts_init_window_width = ss->init_window_width;
    rts_init_window_height = ss->init_window_height;
    prf = prefs_row_with_two_texts("start up size", S_window_width, 
				   "width:", str1, "height:", str2, 6,
				   dpy_box,
				   startup_size_text);
    /* this is not reflected */
    remember_pref(prf, NULL, save_init_window_size, NULL, clear_init_window_size, revert_init_window_size);  
    FREE(str2);
    FREE(str1);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("ask before overwriting anything", S_ask_before_overwrite,
				rts_ask_before_overwrite = ask_before_overwrite(ss), 
				dpy_box,
				overwrite_toggle);
    remember_pref(prf, reflect_ask_before_overwrite, save_ask_before_overwrite, NULL, NULL, revert_ask_before_overwrite);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("ask about unsaved edits before exiting", "check-for-unsaved-edits",
				rts_unsaved_edits = unsaved_edits(), 
				dpy_box,
				unsaved_edits_toggle);
    remember_pref(prf, reflect_unsaved_edits, save_unsaved_edits, help_unsaved_edits, clear_unsaved_edits, revert_unsaved_edits);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("include thumbnail graph in upper right corner", "make-current-window-display",
				rts_current_window_display = current_window_display(),
				dpy_box,
				current_window_display_toggle);
    remember_pref(prf, reflect_current_window_display, save_current_window_display, help_current_window, 
		  clear_current_window_display, revert_current_window_display);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("resize main window as sounds open and close", S_auto_resize,
				rts_auto_resize = auto_resize(ss), 
				dpy_box, 
				resize_toggle);
    remember_pref(prf, reflect_auto_resize, save_auto_resize, NULL, NULL, revert_auto_resize);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("focus follows mouse", "focus-follows-mouse",
				rts_focus_follows_mouse = focus_follows_mouse(),
				dpy_box,
				focus_follows_mouse_toggle);
    remember_pref(prf, reflect_focus_follows_mouse, save_focus_follows_mouse, help_focus_follows_mouse, clear_focus_follows_mouse, revert_focus_follows_mouse);

    current_sep = make_inter_variable_separator(dpy_box);
    rts_sync_choice = sync_choice();
    prf = prefs_row_with_two_toggles("operate on all channels together", S_sync,
				     "within each sound", rts_sync_choice == SYNC_WITHIN_EACH_SOUND,
				     "across all sounds", rts_sync_choice == SYNC_ACROSS_ALL_SOUNDS,
				     dpy_box,
				     sync1_choice, sync2_choice);
    remember_pref(prf, reflect_sync_choice, save_sync_choice, help_sync_choice, clear_sync_choice, revert_sync_choice);

    current_sep = make_inter_variable_separator(dpy_box);
    rts_remember_sound_state_choice = find_remember_sound_state_choice();
    prf = prefs_row_with_two_toggles("restore a sound's state if reopened later", "remember-sound-state",
				     "within one run", rts_remember_sound_state_choice & 1,
				     "across runs", rts_remember_sound_state_choice & 2,
				     dpy_box,
				     remember_sound_state_1_choice, remember_sound_state_2_choice);
    remember_pref(prf, reflect_remember_sound_state_choice, save_remember_sound_state_choice, help_remember_sound_state_choice, 
		  clear_remember_sound_state, revert_remember_sound_state);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("show the control panel upon opening a sound", S_show_controls,
				rts_show_controls = in_show_controls(ss), 
				dpy_box, 
				controls_toggle);
    remember_pref(prf, reflect_show_controls, save_show_controls, NULL, NULL, revert_show_controls);

    current_sep = make_inter_variable_separator(dpy_box);
    include_peak_env_directory = copy_string(peak_env_directory());
    rts_peak_env_directory = copy_string(include_peak_env_directory);
    include_peak_envs = find_peak_envs();
    rts_peak_envs = include_peak_envs;
    prf = prefs_row_with_toggle_with_text("save peak envs to speed up initial display", "save-peak-env-info",
					  include_peak_envs,
					  "directory:", include_peak_env_directory, 25,
					  dpy_box,
					  peak_envs_toggle, peak_envs_text);
    remember_pref(prf, reflect_peak_envs, save_peak_envs, help_peak_envs, clear_peak_envs, revert_peak_envs);

    current_sep = make_inter_variable_separator(dpy_box);
    str = mus_format("%d", rts_max_regions = max_regions(ss));
    prf = prefs_row_with_toggle_with_text("selection creates an associated region", S_selection_creates_region,
					  rts_selection_creates_region = selection_creates_region(ss),
					  "max regions:", str, 5,
					  dpy_box,
					  selection_creates_region_toggle, max_regions_text);
    remember_pref(prf, reflect_selection_creates_region, save_selection_creates_region, NULL, NULL, revert_selection_creates_region);
    FREE(str);


    /* ---------------- file options ---------------- */

    current_sep = make_inter_variable_separator(dpy_box);
    file_label = make_inner_label("  file options", dpy_box);

    rts_load_path = find_sources();
    prf = prefs_row_with_text("directory containing Snd's " XEN_LANGUAGE_NAME " files", "load path", 
			      rts_load_path,
			      dpy_box,
			      load_path_text);
    remember_pref(prf, reflect_load_path, NULL, help_load_path, clear_load_path, revert_load_path);
    load_path_text_widget = prf->text;

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("display only sound files in various file lists", S_just_sounds,
				rts_just_sounds = just_sounds(ss), 
				dpy_box,
				just_sounds_toggle);
    remember_pref(prf, prefs_reflect_just_sounds, save_just_sounds, NULL, NULL, revert_just_sounds);

    current_sep = make_inter_variable_separator(dpy_box);
    rts_temp_dir = copy_string(temp_dir(ss));
    prf = prefs_row_with_text("directory for temporary files", S_temp_dir, 
			      temp_dir(ss), 
			      dpy_box,
			      temp_dir_text);
    remember_pref(prf, reflect_temp_dir, save_temp_dir, NULL, NULL, revert_temp_dir);

    current_sep = make_inter_variable_separator(dpy_box);
    rts_save_dir = copy_string(save_dir(ss));
    prf = prefs_row_with_text("directory for save-state files", S_save_dir, 
			      save_dir(ss), 
			      dpy_box,
			      save_dir_text);
    remember_pref(prf, reflect_save_dir, save_save_dir, NULL, NULL, revert_save_dir);

    current_sep = make_inter_variable_separator(dpy_box);
    rts_save_state_file = copy_string(save_state_file(ss));
    prf = prefs_row_with_text("default save-state filename", S_save_state_file, 
			      save_state_file(ss), 
			      dpy_box,
			      save_state_file_text);
    remember_pref(prf, reflect_save_state_file, save_save_state_file, NULL, NULL, revert_save_state_file);

#if HAVE_LADSPA
    current_sep = make_inter_variable_separator(dpy_box);
    rts_ladspa_dir = copy_string(ladspa_dir(ss));
    prf = prefs_row_with_text("directory for ladspa plugins", S_ladspa_dir, 
			      ladspa_dir(ss), 
			      dpy_box,
			      ladspa_dir_text);
    remember_pref(prf, reflect_ladspa_dir, save_ladspa_dir, NULL, NULL, revert_ladspa_dir);
#endif

    current_sep = make_inter_variable_separator(dpy_box);
    rts_vf_directory = copy_string(view_files_find_any_directory());
    prf = prefs_row_with_text("directory for view-files dialog", S_add_directory_to_view_files_list,
			      rts_vf_directory,
			      dpy_box,
			      view_files_directory_text);
    remember_pref(prf, reflect_view_files_directory, save_view_files_directory, NULL, NULL, revert_view_files_directory);
    /* in this case clear_vf doesn't make much sense */

    current_sep = make_inter_variable_separator(dpy_box);
    rts_html_program = copy_string(html_program(ss));
    prf = prefs_row_with_text("external program to read HTML files via snd-help", S_html_program,
			      html_program(ss),
			      dpy_box,
			      html_program_text);
    remember_pref(prf, reflect_html_program, save_html_program, NULL, NULL, revert_html_program);
    current_sep = make_inter_variable_separator(dpy_box);

    rts_default_output_chans = default_output_chans(ss);
    prf = prefs_row_with_radio_box("default new sound attributes: chans", S_default_output_chans,
				   output_chan_choices, NUM_OUTPUT_CHAN_CHOICES, -1,
				   dpy_box,
				   default_output_chans_choice);
    remember_pref(prf, reflect_default_output_chans, save_default_output_chans, NULL, NULL, revert_default_output_chans);
    reflect_default_output_chans(prf);

    rts_default_output_srate = default_output_srate(ss);
    prf = prefs_row_with_radio_box("srate", S_default_output_srate,
				   output_srate_choices, NUM_OUTPUT_SRATE_CHOICES, -1,
				   dpy_box,
				   default_output_srate_choice);
    remember_pref(prf, reflect_default_output_srate, save_default_output_srate, NULL, NULL, revert_default_output_srate);
    reflect_default_output_srate(prf);

    rts_default_output_header_type = default_output_header_type(ss);
    prf = prefs_row_with_radio_box("header type", S_default_output_header_type,
				   output_type_choices, NUM_OUTPUT_TYPE_CHOICES, -1,
				   dpy_box,
				   default_output_header_type_choice);
    output_header_type_prf = prf;
    remember_pref(prf, reflect_default_output_header_type, save_default_output_header_type, NULL, NULL, revert_default_output_header_type);

    rts_default_output_data_format = default_output_data_format(ss);
    prf = prefs_row_with_radio_box("data format", S_default_output_data_format,
				   output_format_choices, NUM_OUTPUT_FORMAT_CHOICES, -1,
				   dpy_box,
				   default_output_data_format_choice);
    output_data_format_prf = prf;
    remember_pref(prf, reflect_default_output_data_format, save_default_output_data_format, NULL, NULL, revert_default_output_data_format);
    reflect_default_output_header_type(output_header_type_prf);
    reflect_default_output_data_format(output_data_format_prf);

    current_sep = make_inter_variable_separator(dpy_box);
    {
      int i, srate = 0, chans = 0, format = 0;
      mus_header_raw_defaults(&srate, &chans, &format);
      str = mus_format("%d", chans);
      str1 = mus_format("%d", srate);
      rts_raw_chans = chans;
      rts_raw_srate = srate;
      rts_raw_data_format = format;
      raw_data_format_choices = (char **)CALLOC(MUS_NUM_DATA_FORMATS - 1, sizeof(char *));
      for (i = 1; i < MUS_NUM_DATA_FORMATS; i++)
	raw_data_format_choices[i - 1] = raw_data_format_to_string(i); /* skip MUS_UNKNOWN */
      prf = prefs_row_with_text("default raw sound attributes: chans", S_mus_header_raw_defaults, str,
				dpy_box, 
				raw_chans_choice);
      remember_pref(prf, reflect_raw_chans, save_raw_chans, NULL, NULL, revert_raw_chans);

      prf = prefs_row_with_text("srate", S_mus_header_raw_defaults, str1,
				dpy_box, 
				raw_srate_choice);
      remember_pref(prf, reflect_raw_srate, save_raw_srate, NULL, NULL, revert_raw_srate);
      FREE(str);
      FREE(str1);

#if HAVE_GTK_COMBO_BOX_ENTRY_NEW_TEXT
      prf = prefs_row_with_list("data format", S_mus_header_raw_defaults, raw_data_format_choices[format - 1],
				(const char **)raw_data_format_choices, MUS_NUM_DATA_FORMATS - 1,
				dpy_box, 
				raw_data_format_from_text,
				NULL, NULL);
      remember_pref(prf, reflect_raw_data_format, save_raw_data_format, NULL, NULL, revert_raw_data_format);
#endif
    }
    current_sep = make_inter_variable_separator(dpy_box);


  /* ---------------- extra menus ---------------- */

#if HAVE_STATIC_XM
    cursor_label = make_inner_label("  extra menus", dpy_box);
#else
    cursor_label = make_inner_label("  extra menus (these will need the xm module)", dpy_box);
#endif

    prf = prefs_row_with_toggle("context-sensitive popup menu", "add-selection-popup",
				(include_context_sensitive_popup = find_context_sensitive_popup()),
				dpy_box,
				context_sensitive_popup_toggle);
    remember_pref(prf, reflect_context_sensitive_popup, save_context_sensitive_popup, help_context_sensitive_popup, 
		  clear_context_sensitive_popup, revert_context_sensitive_popup);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("effects menu",
#if HAVE_SCHEME
				"new-effects.scm",
#else
				"effects." XEN_FILE_EXTENSION,
#endif
				(include_effects_menu = find_effects_menu()),
				dpy_box, 
				effects_menu_toggle);
    remember_pref(prf, reflect_effects_menu, save_effects_menu, help_effects_menu, clear_effects_menu, revert_effects_menu);

#if HAVE_SCHEME
    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("edit menu additions", "edit-menu.scm",
				(include_edit_menu = find_edit_menu()),
				dpy_box, 
				edit_menu_toggle);
    remember_pref(prf, reflect_edit_menu, save_edit_menu, help_edit_menu, clear_edit_menu, revert_edit_menu);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("marks menu", "marks-menu.scm",
				(include_marks_menu = find_marks_menu()),
				dpy_box,
				marks_menu_toggle);
    remember_pref(prf, reflect_marks_menu, save_marks_menu, help_marks_menu, clear_marks_menu, revert_marks_menu);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("mix/track menu", "mix-menu.scm",
				(include_mix_menu = find_mix_menu()),
				dpy_box,
				mix_menu_toggle);
    remember_pref(prf, reflect_mix_menu, save_mix_menu, help_mix_menu, clear_mix_menu, revert_mix_menu);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("a toolbar", "toolbar.scm",
				(include_icon_box = find_icon_box()),
				dpy_box,
				icon_box_toggle);
    remember_pref(prf, reflect_icon_box, save_icon_box, help_icon_box, clear_icon_box, revert_icon_box);
#endif

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_toggle("reopen menu", "with-reopen-menu",
				(include_reopen_menu = find_reopen_menu()),
				dpy_box,
				reopen_menu_toggle);
    remember_pref(prf, reflect_reopen_menu, save_reopen_menu, help_reopen_menu, clear_reopen_menu, revert_reopen_menu);

    current_sep = make_inter_variable_separator(dpy_box);


    /* ---------------- additional key bindings ---------------- */

    {
      key_info *ki;

      key_label = make_inner_label("  additional key bindings", dpy_box);

      ki = find_prefs_key_binding("play-from-cursor");
      prf = prefs_row_with_text_and_three_toggles("play all chans from cursor", S_play, 
						  "key:", 8, "ctrl:", "meta:",  "C-x:",
						  ki->key, ki->c, ki->m, ki->x,						
						  dpy_box,
						  bind_play_from_cursor);
      remember_pref(prf, reflect_play_from_cursor, save_pfc_binding, help_play_from_cursor, clear_play_from_cursor, NULL);
      FREE(ki);

      current_sep = make_inter_variable_separator(dpy_box);
      ki = find_prefs_key_binding("show-all");
      prf = prefs_row_with_text_and_three_toggles("show entire sound", S_x_bounds, 
						  "key:", 8, "ctrl:", "meta:",  "C-x:",
						  ki->key, ki->c, ki->m, ki->x,
						  dpy_box,
						  bind_show_all);
      remember_pref(prf, reflect_show_all, save_show_all_binding, help_show_all, clear_show_all, NULL);
      FREE(ki);

      current_sep = make_inter_variable_separator(dpy_box);
      ki = find_prefs_key_binding("select-all");
      prf = prefs_row_with_text_and_three_toggles("select entire sound", S_select_all, 
						  "key:", 8, "ctrl:", "meta:",  "C-x:",
						  ki->key, ki->c, ki->m, ki->x,
						  dpy_box,
						  bind_select_all);
      remember_pref(prf, reflect_select_all, save_select_all_binding, help_select_all, clear_select_all, NULL);
      FREE(ki);

      current_sep = make_inter_variable_separator(dpy_box);
      ki = find_prefs_key_binding("show-selection");
      prf = prefs_row_with_text_and_three_toggles("show current selection", "show-selection", 
						  "key:", 8, "ctrl:", "meta:",  "C-x:",
						  ki->key, ki->c, ki->m, ki->x,
						  dpy_box,
						  bind_show_selection);
      remember_pref(prf, reflect_show_selection, save_show_selection_binding, help_show_selection, clear_show_selection, NULL);
      FREE(ki);

      current_sep = make_inter_variable_separator(dpy_box);
      ki = find_prefs_key_binding("revert-sound");
      prf = prefs_row_with_text_and_three_toggles("undo all edits (revert)", S_revert_sound, 
						  "key:", 8, "ctrl:", "meta:",  "C-x:",
						  ki->key, ki->c, ki->m, ki->x,
						  dpy_box,
						  bind_revert);
      remember_pref(prf, reflect_revert, save_revert_binding, help_revert, clear_revert_sound, NULL);
      FREE(ki);

      current_sep = make_inter_variable_separator(dpy_box);
      ki = find_prefs_key_binding("exit");
      prf = prefs_row_with_text_and_three_toggles("exit from Snd", S_exit, 
						  "key:", 8, "ctrl:", "meta:",  "C-x:",
						  ki->key, ki->c, ki->m, ki->x,
						  dpy_box,
						  bind_exit);
      remember_pref(prf, reflect_exit, save_exit_binding, help_exit, clear_exit, NULL);
      FREE(ki);

      current_sep = make_inter_variable_separator(dpy_box);
      ki = find_prefs_key_binding("goto-maxamp");
      prf = prefs_row_with_text_and_three_toggles("move cursor to channel's maximum sample", S_maxamp_position, 
						  "key:", 8, "ctrl:", "meta:",  "C-x:",
						  ki->key, ki->c, ki->m, ki->x,
						  dpy_box,
						  bind_goto_maxamp);
      remember_pref(prf, reflect_goto_maxamp, save_goto_maxamp_binding, help_goto_maxamp, clear_goto_maxamp, NULL);
      FREE(ki);

    }

    /* ---------------- cursor options ---------------- */

    current_sep = make_inter_variable_separator(dpy_box);
    cursor_label = make_inner_label("  cursor options", dpy_box);

    prf = prefs_row_with_toggle("report cursor location as it moves", S_with_verbose_cursor,
				rts_verbose_cursor = verbose_cursor(ss), 
				dpy_box,
				verbose_cursor_toggle);
    remember_pref(prf, reflect_verbose_cursor, save_verbose_cursor, NULL, NULL, revert_verbose_cursor);

    current_sep = make_inter_variable_separator(dpy_box);
    {
      char *str1;
      str = mus_format("%.2f", rts_cursor_update_interval = cursor_update_interval(ss));
      str1 = mus_format("%d", rts_cursor_location_offset = cursor_location_offset(ss));
      prf = prefs_row_with_toggle_with_two_texts("track current location while playing", S_with_tracking_cursor,
						 (rts_with_tracking_cursor = with_tracking_cursor(ss)), 
						 "update:", str,
						 "offset:", str1, 8, 
						 dpy_box,
						 with_tracking_cursor_toggle,
						 cursor_location_text);
      remember_pref(prf, reflect_with_tracking_cursor, save_with_tracking_cursor, NULL, NULL, revert_with_tracking_cursor);
      FREE(str);
      FREE(str1);
    }

    current_sep = make_inter_variable_separator(dpy_box);
    str = mus_format("%d", rts_cursor_size = cursor_size(ss));
    prf = prefs_row_with_number("size", S_cursor_size,
				str, 4, 
				dpy_box,
				cursor_size_up, cursor_size_down, cursor_size_from_text);
    remember_pref(prf, reflect_cursor_size, save_cursor_size, NULL, NULL, revert_cursor_size);
    FREE(str);
    if (cursor_size(ss) <= 0) gtk_widget_set_sensitive(prf->arrow_down, false);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_radio_box("shape", S_cursor_style,
				   cursor_styles, NUM_CURSOR_STYLES, 
				   rts_cursor_style = cursor_style(ss),
				   dpy_box, 
				   cursor_style_choice);
    remember_pref(prf, reflect_cursor_style, save_cursor_style, NULL, NULL, revert_cursor_style);

    current_sep = make_inter_variable_separator(dpy_box);
    prf = prefs_row_with_radio_box("tracking cursor shape", S_tracking_cursor_style,
				   cursor_styles, NUM_CURSOR_STYLES, 
				   rts_tracking_cursor_style = tracking_cursor_style(ss),
				   dpy_box, 
				   tracking_cursor_style_choice);
    remember_pref(prf, reflect_tracking_cursor_style, save_tracking_cursor_style, NULL, NULL, revert_tracking_cursor_style);

    current_sep = make_inter_variable_separator(dpy_box);
    saved_cursor_color = ss->sgx->cursor_color;
    prf = prefs_color_selector_row("color", S_cursor_color, ss->sgx->cursor_color,
				   dpy_box,
				   cursor_color_func);
    remember_pref(prf, NULL, save_cursor_color, NULL, clear_cursor_color, revert_cursor_color);

    /* ---------------- (overall) colors ---------------- */

    current_sep = make_inter_variable_separator(dpy_box);
    cursor_label = make_inner_label("  colors", dpy_box);
    
    saved_basic_color = ss->sgx->basic_color;
    prf = prefs_color_selector_row("main background color", S_basic_color, ss->sgx->basic_color,
				   dpy_box,
				   basic_color_func);
    remember_pref(prf, NULL, save_basic_color, NULL, clear_basic_color, revert_basic_color);

    current_sep = make_inter_variable_separator(dpy_box);
    saved_highlight_color = ss->sgx->highlight_color;
    prf = prefs_color_selector_row("main highlight color", S_highlight_color, ss->sgx->highlight_color,
				   dpy_box,
				   highlight_color_func);
    remember_pref(prf, NULL, save_highlight_color, NULL, clear_highlight_color, revert_highlight_color);

    current_sep = make_inter_variable_separator(dpy_box);
    saved_position_color = ss->sgx->position_color;
    prf = prefs_color_selector_row("second highlight color", S_position_color, ss->sgx->position_color,
				   dpy_box,
				   position_color_func);
    remember_pref(prf, NULL, save_position_color, NULL, clear_position_color, revert_position_color);

    current_sep = make_inter_variable_separator(dpy_box);
    saved_zoom_color = ss->sgx->zoom_color;
    prf = prefs_color_selector_row("third highlight color", S_zoom_color, ss->sgx->zoom_color,
				   dpy_box,
				   zoom_color_func);
    remember_pref(prf, NULL, save_zoom_color, NULL, clear_zoom_color, revert_zoom_color);
  }

  current_sep = make_inter_topic_separator(topics);

  /* -------- graphs -------- */
  {
    GtkWidget *grf_box, *grf_label, *colgrf_label;

    /* ---------------- graph options ---------------- */

    grf_box = make_top_level_box(topics);
    grf_label = make_top_level_label("graph options", grf_box);

    prf = prefs_row_with_radio_box("how to connect the dots", S_graph_style,
				   graph_styles, NUM_GRAPH_STYLES, 
				   rts_graph_style = graph_style(ss),
				   grf_box,
				   graph_style_choice);
    remember_pref(prf, reflect_graph_style, save_graph_style, NULL, NULL, revert_graph_style);

    current_sep = make_inter_variable_separator(grf_box);
    str = mus_format("%d", rts_dot_size = dot_size(ss));
    prf = prefs_row_with_number("dot size", S_dot_size,
				str, 4, 
				grf_box,
				dot_size_up, dot_size_down, dot_size_from_text);
    remember_pref(prf, reflect_dot_size, save_dot_size, NULL, NULL, revert_dot_size);
    FREE(str);
    if (dot_size(ss) <= 0) gtk_widget_set_sensitive(prf->arrow_down, false);

    current_sep = make_inter_variable_separator(grf_box);
    rts_initial_beg = initial_beg();
    rts_initial_dur = initial_dur();
    str = mus_format("%.2f : %.2f", rts_initial_beg, rts_initial_dur);
    prf = prefs_row_with_text_with_toggle("initial graph x bounds", S_initial_graph_hook, 
					  (rts_full_duration = full_duration()),
					  "show full duration", str, 16,
					  grf_box, 
					  initial_bounds_toggle,
					  initial_bounds_text);
    FREE(str);
    remember_pref(prf, reflect_initial_bounds, save_initial_bounds, help_initial_bounds, clear_initial_bounds, revert_initial_bounds);

    current_sep = make_inter_variable_separator(grf_box);
    prf = prefs_row_with_radio_box("how to layout multichannel graphs", S_channel_style,
				   channel_styles, NUM_CHANNEL_STYLES, 
				   rts_channel_style = channel_style(ss),
				   grf_box,
				   channel_style_choice);
    remember_pref(prf, reflect_channel_style, save_channel_style, NULL, NULL, revert_channel_style);

    current_sep = make_inter_variable_separator(grf_box);
    prf = prefs_row_with_toggle("layout wave and fft graphs horizontally", S_graphs_horizontal,
				rts_graphs_horizontal = graphs_horizontal(ss),
				grf_box,
				graphs_horizontal_toggle);
    remember_pref(prf, reflect_graphs_horizontal, save_graphs_horizontal, NULL, NULL, revert_graphs_horizontal);

    current_sep = make_inter_variable_separator(grf_box);
    prf = prefs_row_with_toggle("include y=0 line in sound graphs", S_show_y_zero,
				rts_show_y_zero = show_y_zero(ss),
				grf_box,
				y_zero_toggle);
    remember_pref(prf, reflect_show_y_zero, save_show_y_zero, NULL, NULL, revert_show_y_zero);

    current_sep = make_inter_variable_separator(grf_box);
    rts_show_grid = show_grid(ss);
    prf = prefs_row_with_toggle("include a grid in sound graphs", S_show_grid,
				rts_show_grid == WITH_GRID,
				grf_box,
				grid_toggle);
    remember_pref(prf, reflect_show_grid, save_show_grid, NULL, NULL, revert_show_grid);

    current_sep = make_inter_variable_separator(grf_box);
    prf = prefs_row_with_scale("grid density", S_grid_density, 
			       2.0, rts_grid_density = grid_density(ss),
			       grf_box,
			       grid_density_scale_callback, grid_density_text_callback);
    remember_pref(prf, reflect_grid_density, save_grid_density, NULL, NULL, revert_grid_density);

#if HAVE_GTK_COMBO_BOX_ENTRY_NEW_TEXT
    current_sep = make_inter_variable_separator(grf_box);
    rts_show_axes = show_axes(ss);
    prf = prefs_row_with_list("what axes to display", S_show_axes, show_axes_choices[(int)rts_show_axes],
			      show_axes_choices, NUM_SHOW_AXES,
			      grf_box,
			      show_axes_from_text,
			      NULL, NULL);
    remember_pref(prf, reflect_show_axes, save_show_axes, NULL, clear_show_axes, revert_show_axes);

    current_sep = make_inter_variable_separator(grf_box);
    rts_x_axis_style = x_axis_style(ss);
    prf = prefs_row_with_list("time division", S_x_axis_style, x_axis_styles[(int)rts_x_axis_style],
			      x_axis_styles, NUM_X_AXIS_STYLES,
			      grf_box,
			      x_axis_style_from_text,
			      NULL, NULL);
    remember_pref(prf, reflect_x_axis_style, save_x_axis_style, NULL, clear_x_axis_style, revert_x_axis_style);
#endif

    current_sep = make_inter_variable_separator(grf_box);
    prf = prefs_row_with_toggle("include smpte info", "show-smpte-label",
				(include_smpte = find_smpte()),
				grf_box,
				smpte_toggle);
    remember_pref(prf, reflect_smpte, save_smpte, help_smpte, clear_smpte, revert_smpte);

    /* ---------------- (graph) colors ---------------- */

    current_sep = make_inter_variable_separator(grf_box); 
    colgrf_label = make_inner_label("  colors", grf_box);

    saved_data_color = ss->sgx->data_color;    
    prf = prefs_color_selector_row("unselected data (waveform) color", S_data_color, ss->sgx->data_color,
				   grf_box, 
				   data_color_func);
    remember_pref(prf, NULL, save_data_color, NULL, clear_data_color, revert_data_color);

    current_sep = make_inter_variable_separator(grf_box);
    saved_graph_color = ss->sgx->graph_color;
    prf = prefs_color_selector_row("unselected graph (background) color", S_graph_color, ss->sgx->graph_color,
				   grf_box,
				   graph_color_func);
    remember_pref(prf, NULL, save_graph_color, NULL, clear_graph_color, revert_graph_color);

    current_sep = make_inter_variable_separator(grf_box);
    saved_selected_data_color = ss->sgx->selected_data_color;
    prf = prefs_color_selector_row("selected channel data (waveform) color", S_selected_data_color, ss->sgx->selected_data_color,
				   grf_box,
				   selected_data_color_func);
    remember_pref(prf, NULL, save_selected_data_color, NULL, clear_selected_data_color, revert_selected_data_color);

    current_sep = make_inter_variable_separator(grf_box);
    saved_selected_graph_color = ss->sgx->selected_graph_color;
    prf = prefs_color_selector_row("selected channel graph (background) color", S_selected_graph_color, ss->sgx->selected_graph_color,
				   grf_box,
				   selected_graph_color_func);
    remember_pref(prf, NULL, save_selected_graph_color, NULL, clear_selected_graph_color, revert_selected_graph_color);

    current_sep = make_inter_variable_separator(grf_box);
    saved_selection_color = ss->sgx->selection_color;
    prf = prefs_color_selector_row("selection color", S_selection_color, ss->sgx->selection_color,
				   grf_box,
				   selection_color_func);
    remember_pref(prf, NULL, save_selection_color, NULL, clear_selection_color, revert_selection_color);

    /* ---------------- (graph) fonts ---------------- */

    current_sep = make_inter_variable_separator(grf_box);
    colgrf_label = make_inner_label("  fonts", grf_box);

    rts_axis_label_font = copy_string(axis_label_font(ss));
    prf = prefs_row_with_text("axis label font", S_axis_label_font, 
			      axis_label_font(ss), 
			      grf_box, 
			      axis_label_font_text);
    remember_pref(prf, reflect_axis_label_font, save_axis_label_font, NULL, clear_axis_label_font, revert_axis_label_font);

    current_sep = make_inter_variable_separator(grf_box);    
    rts_axis_numbers_font = copy_string(axis_numbers_font(ss)); 
    prf = prefs_row_with_text("axis number font", S_axis_numbers_font, 
			      axis_numbers_font(ss), 
			      grf_box,
			      axis_numbers_font_text);
    remember_pref(prf, reflect_axis_numbers_font, save_axis_numbers_font, NULL, clear_axis_numbers_font, revert_axis_numbers_font);

    current_sep = make_inter_variable_separator(grf_box);  
    rts_peaks_font = copy_string(peaks_font(ss));    
    prf = prefs_row_with_text("fft peaks font", S_peaks_font, 
			      peaks_font(ss), 
			      grf_box,
			      peaks_font_text);
    remember_pref(prf, reflect_peaks_font, save_peaks_font, NULL, clear_peaks_font, revert_peaks_font);

    current_sep = make_inter_variable_separator(grf_box);    
    rts_bold_peaks_font = copy_string(bold_peaks_font(ss));     
    prf = prefs_row_with_text("fft peaks bold font (for main peaks)", S_bold_peaks_font, 
			      bold_peaks_font(ss), 
			      grf_box,
			      bold_peaks_font_text);
    remember_pref(prf, reflect_bold_peaks_font, save_bold_peaks_font, NULL, clear_bold_peaks_font, revert_bold_peaks_font);

    current_sep = make_inter_variable_separator(grf_box);  
    rts_tiny_font = copy_string(tiny_font(ss));        
    prf = prefs_row_with_text("tiny font (for various annotations)", S_peaks_font, 
			      tiny_font(ss), 
			      grf_box,
			      tiny_font_text);
    remember_pref(prf, reflect_tiny_font, save_tiny_font, NULL, clear_tiny_font, revert_tiny_font);
  }

  current_sep = make_inter_topic_separator(topics);

  /* -------- transform -------- */
  {
    GtkWidget *fft_box, *fft_label;

    /* ---------------- transform options ---------------- */

    fft_box = make_top_level_box(topics);
    fft_label = make_top_level_label("transform options", fft_box);

    str = mus_format("%d", rts_fft_size = transform_size(ss));
    prf = prefs_row_with_number("size", S_transform_size,
				str, 12, 
				fft_box,
				fft_size_up, fft_size_down, fft_size_from_text);
    remember_pref(prf, reflect_fft_size, save_fft_size, NULL, NULL, revert_fft_size);
    FREE(str);
    if (transform_size(ss) <= 2) gtk_widget_set_sensitive(prf->arrow_down, false);

    current_sep = make_inter_variable_separator(fft_box);
    prf = prefs_row_with_radio_box("transform graph choice", S_transform_graph_type,
				   transform_graph_types, NUM_TRANSFORM_GRAPH_TYPES, 
				   rts_transform_graph_type = transform_graph_type(ss),
				   fft_box,
				   transform_graph_type_choice);
    remember_pref(prf, reflect_transform_graph_type, save_transform_graph_type, NULL, NULL, revert_transform_graph_type);

#if HAVE_GTK_COMBO_BOX_ENTRY_NEW_TEXT
    current_sep = make_inter_variable_separator(fft_box);
    rts_transform_type = transform_type(ss);
    prf = prefs_row_with_list("transform", S_transform_type, transform_types[rts_transform_type],
			      transform_types, NUM_BUILTIN_TRANSFORM_TYPES,
			      fft_box,
			      transform_type_from_text,
			      transform_type_completer, NULL);
    remember_pref(prf, reflect_transform_type, save_transform_type, NULL, clear_transform_type, revert_transform_type);

    current_sep = make_inter_variable_separator(fft_box);
    rts_fft_window = fft_window(ss);
    prf = prefs_row_with_list("data window", S_fft_window, fft_windows[(int)rts_fft_window],
			      fft_windows, MUS_NUM_WINDOWS,
			      fft_box,
			      fft_window_from_text,
			      fft_window_completer, NULL);
    remember_pref(prf, reflect_fft_window, save_fft_window, NULL, clear_fft_window, revert_fft_window);
#endif

    current_sep = make_inter_variable_separator(fft_box);
    prf = prefs_row_with_scale("data window family parameter", S_fft_window_beta, 
			       1.0, rts_fft_window_beta = fft_window_beta(ss),
			       fft_box,
			       fft_window_beta_scale_callback, fft_window_beta_text_callback);
    remember_pref(prf, reflect_fft_window_beta, save_fft_window_beta, NULL, NULL, revert_fft_window_beta);

    current_sep = make_inter_variable_separator(fft_box);
    str = mus_format("%d", rts_max_transform_peaks = max_transform_peaks(ss));
    prf = prefs_row_with_toggle_with_text("show fft peak data", S_show_transform_peaks,
					  rts_show_transform_peaks = show_transform_peaks(ss),
					  "max peaks:", str, 5,
					  fft_box,
					  transform_peaks_toggle, max_peaks_text);
    remember_pref(prf, reflect_transform_peaks, save_transform_peaks, NULL, NULL, revert_transform_peaks);
    FREE(str);

#if HAVE_GTK_COMBO_BOX_ENTRY_NEW_TEXT
    current_sep = make_inter_variable_separator(fft_box);
    {
      const char **cmaps;
      int i, len;
      len = num_colormaps();
      cmaps = (const char **)CALLOC(len, sizeof(const char *));
      for (i = 0; i < len; i++)
	cmaps[i] = (const char *)colormap_name(i);
      rts_colormap = color_map(ss);
      prf = prefs_row_with_list("sonogram colormap", S_colormap, cmaps[rts_colormap],
				cmaps, len,
				fft_box,
				colormap_from_text,
				colormap_completer, NULL);
      remember_pref(prf, reflect_colormap, save_colormap, NULL, clear_colormap, revert_colormap);
      FREE(cmaps);
    }
#endif

    current_sep = make_inter_variable_separator(fft_box);
    prf = prefs_row_with_toggle("y axis as log magnitude (dB)", S_fft_log_magnitude,
				rts_fft_log_magnitude = fft_log_magnitude(ss),
				fft_box,
				log_magnitude_toggle);
    remember_pref(prf, reflect_fft_log_magnitude, save_fft_log_magnitude, NULL, NULL, revert_fft_log_magnitude);

    current_sep = make_inter_variable_separator(fft_box);
    str = mus_format("%.1f", rts_min_dB = min_dB(ss));
    prf = prefs_row_with_text("minimum y-axis dB value", S_min_dB, str,
			      fft_box,
			      min_dB_text);
    remember_pref(prf, reflect_min_dB, save_min_dB, NULL, NULL, revert_min_dB);
    FREE(str);

    current_sep = make_inter_variable_separator(fft_box);
    prf = prefs_row_with_toggle("x axis as log freq", S_fft_log_frequency,
				rts_fft_log_frequency = fft_log_frequency(ss),
				fft_box,
				log_frequency_toggle);
    remember_pref(prf, reflect_fft_log_frequency, save_fft_log_frequency, NULL, NULL, revert_fft_log_frequency);

    current_sep = make_inter_variable_separator(fft_box);
    prf = prefs_row_with_radio_box("normalization", S_transform_normalization,
				   transform_normalizations, NUM_TRANSFORM_NORMALIZATIONS, 
				   rts_transform_normalization = transform_normalization(ss),
				   fft_box,
				   transform_normalization_choice);
    remember_pref(prf, reflect_transform_normalization, save_transform_normalization, NULL, NULL, revert_transform_normalization);
  }

  current_sep = make_inter_topic_separator(topics);

  /* -------- marks, mixes, and regions -------- */
  {
    GtkWidget *mmr_box, *mmr_label;
    char *str1, *str2;

    /* ---------------- marks and mixes ---------------- */

    mmr_box = make_top_level_box(topics);
    mmr_label = make_top_level_label("marks and mixes", mmr_box);

    saved_mark_color = ss->sgx->mark_color;
    prf = prefs_color_selector_row("mark and mix tag color", S_mark_color, ss->sgx->mark_color,
				   mmr_box,
				   mark_color_func);
    remember_pref(prf, NULL, save_mark_color, NULL, clear_mark_color, revert_mark_color);

    current_sep = make_inter_variable_separator(mmr_box);

    str1 = mus_format("%d", rts_mark_tag_width = mark_tag_width(ss));
    str2 = mus_format("%d", rts_mark_tag_height = mark_tag_height(ss));
    prf = prefs_row_with_two_texts("mark tag size", S_mark_tag_width, 
				   "width:", str1, "height:", str2, 4,
				   mmr_box,
				   mark_tag_size_text);
    remember_pref(prf, reflect_mark_tag_size, save_mark_tag_size, NULL, NULL, revert_mark_tag_size);
    FREE(str2);
    FREE(str1);

    current_sep = make_inter_variable_separator(mmr_box);
    str1 = mus_format("%d", rts_mix_tag_width = mix_tag_width(ss));
    str2 = mus_format("%d", rts_mix_tag_height = mix_tag_height(ss));
    prf = prefs_row_with_two_texts("mix tag size", S_mix_tag_width, 
				   "width:", str1, "height:", str2, 4,
				   mmr_box,
				   mix_tag_size_text);
    remember_pref(prf, reflect_mix_tag_size, save_mix_tag_size, NULL, NULL, revert_mix_tag_size);
    FREE(str2);
    FREE(str1);

    current_sep = make_inter_variable_separator(mmr_box);
    saved_mix_color = ss->sgx->mix_color;
    prf = prefs_color_selector_row("mix waveform color", S_mix_color, ss->sgx->mix_color,
				   mmr_box,
				   mix_color_func);
    remember_pref(prf, NULL, save_mix_color, NULL, clear_mix_color, revert_mix_color);

    current_sep = make_inter_variable_separator(mmr_box);
    str = mus_format("%d", rts_mix_waveform_height = mix_waveform_height(ss));
    prf = prefs_row_with_toggle_with_text("show mix waveforms (attached to the mix tag)", S_show_mix_waveforms,
					  rts_show_mix_waveforms = show_mix_waveforms(ss),
					  "max waveform height:", str, 5,
					  mmr_box,
					  show_mix_waveforms_toggle, mix_waveform_height_text);
    remember_pref(prf, reflect_mix_waveforms, save_mix_waveforms, NULL, NULL, revert_mix_waveforms);
    FREE(str);
  }
  

  current_sep = make_inter_topic_separator(topics);

  /* -------- clm -------- */
  {
    GtkWidget *clm_box, *clm_label;

    /* ---------------- clm options ---------------- */

    clm_box = make_top_level_box(topics);
    clm_label = make_top_level_label("clm", clm_box);

    rts_with_sound = with_sound_is_loaded();
    prf = prefs_row_with_toggle("include with-sound", "with-sound",
				rts_with_sound,
				clm_box,
				with_sound_toggle);
    remember_pref(prf, reflect_with_sound, save_with_sound, help_with_sound, clear_with_sound, revert_with_sound);

    current_sep = make_inter_variable_separator(clm_box);
    str = mus_format("%d", rts_speed_control_tones = speed_control_tones(ss));
    rts_speed_control_style = speed_control_style(ss);
    prf = prefs_row_with_radio_box_and_number("speed control choice", S_speed_control_style,
					      speed_control_styles, NUM_SPEED_CONTROL_STYLES, (int)speed_control_style(ss),
					      speed_control_tones(ss), str, 6,
					      clm_box,
					      speed_control_choice, speed_control_up, speed_control_down, speed_control_text);
    remember_pref(prf, reflect_speed_control, save_speed_control, NULL, NULL, revert_speed_control);
    FREE(str);

#if HAVE_SCHEME
    current_sep = make_inter_variable_separator(clm_box);
    prf = prefs_row_with_toggle("include hidden controls dialog", "hidden-controls-dialog",
				(include_hidden_controls = find_hidden_controls()),
				clm_box,
				hidden_controls_toggle);
    remember_pref(prf, reflect_hidden_controls, save_hidden_controls, help_hidden_controls, clear_hidden_controls, revert_hidden_controls);
#endif

    current_sep = make_inter_variable_separator(clm_box);
    str = mus_format("%d", rts_sinc_width = sinc_width(ss));
    prf = prefs_row_with_text("sinc interpolation width in srate converter", S_sinc_width, str,
			      clm_box,
			      sinc_width_text);
    remember_pref(prf, reflect_sinc_width, save_sinc_width, NULL, NULL, revert_sinc_width);
    FREE(str);

    current_sep = make_inter_variable_separator(clm_box);
    rts_clm_file_name = copy_string(find_clm_file_name());
    prf = prefs_row_with_text("with-sound default output file name", "*clm-file-name*", rts_clm_file_name,
			      clm_box,
			      clm_file_name_text);
    remember_pref(prf, reflect_clm_file_name, save_clm_file_name, help_clm_file_name, clear_clm_file_name, revert_clm_file_name);

    current_sep = make_inter_variable_separator(clm_box);
    rts_clm_table_size = find_clm_table_size();
    rts_clm_file_buffer_size = find_clm_file_buffer_size();
    prf = prefs_row_with_two_texts("sizes", "*clm-table-size*",
				   "wave table:", NULL, "file buffer:", NULL, 8,
				   clm_box,
				   clm_sizes_text);
    reflect_clm_sizes(prf);
    remember_pref(prf, reflect_clm_sizes, save_clm_sizes, help_clm_sizes, clear_clm_sizes, revert_clm_sizes);
  }

  current_sep = make_inter_topic_separator(topics);

  /* -------- programming -------- */
  {
    GtkWidget *prg_box, *prg_label;

    /* ---------------- listener options ---------------- */

    prg_box = make_top_level_box(topics);
    prg_label = make_top_level_label("listener options", prg_box);

    prf = prefs_row_with_toggle("show listener at start up", S_show_listener,
				rts_show_listener = listener_is_visible(),
				prg_box,
				show_listener_toggle);
    remember_pref(prf, reflect_show_listener, save_show_listener, NULL, clear_show_listener, revert_show_listener);

#if WITH_RUN
    current_sep = make_inter_variable_separator(prg_box);
    str = mus_format("%d", rts_optimization = optimization(ss));
    prf = prefs_row_with_number("optimization level", S_optimization,
				str, 3, 
				prg_box,
				optimization_up, optimization_down, optimization_from_text);
    remember_pref(prf, reflect_optimization, save_optimization, NULL, NULL, revert_optimization);
    FREE(str);
    if (optimization(ss) == 6) gtk_widget_set_sensitive(prf->arrow_up, false);
    if (optimization(ss) == 0) gtk_widget_set_sensitive(prf->arrow_down, false);
#endif

    current_sep = make_inter_variable_separator(prg_box);
    rts_listener_prompt = copy_string(listener_prompt(ss));
    prf = prefs_row_with_text("prompt", S_listener_prompt, 
			      listener_prompt(ss), 
			      prg_box,
			      listener_prompt_text);
    remember_pref(prf, reflect_listener_prompt, save_listener_prompt, NULL, NULL, revert_listener_prompt);

    current_sep = make_inter_variable_separator(prg_box);
    prf = prefs_row_with_toggle("include backtrace in error report", S_show_backtrace,
				rts_show_backtrace = show_backtrace(ss),
				prg_box,
				show_backtrace_toggle);
    remember_pref(prf, reflect_show_backtrace, save_show_backtrace, NULL, NULL, revert_show_backtrace);

#if HAVE_GUILE
    current_sep = make_inter_variable_separator(prg_box);
    prf = prefs_row_with_toggle("include debugging aids", "snd-break",
				(include_debugging_aids = find_debugging_aids()),
				prg_box,
				debugging_aids_toggle);
    remember_pref(prf, reflect_debugging_aids, save_debugging_aids, NULL, clear_debugging_aids, revert_debugging_aids);
    include_debugging_aids = find_debugging_aids();
#endif

    current_sep = make_inter_variable_separator(prg_box);
    str = mus_format("%d", rts_print_length = print_length(ss));
    prf = prefs_row_with_text("number of vector elements to display", S_print_length, str,
			      prg_box,
			      print_length_text);
    remember_pref(prf, reflect_print_length, save_print_length, NULL, NULL, revert_print_length);
    FREE(str);

    current_sep = make_inter_variable_separator(prg_box);
    rts_listener_font = copy_string(listener_font(ss));
    prf = prefs_row_with_text("font", S_listener_font, 
			      listener_font(ss), 
			      prg_box,
			      listener_font_text);
    remember_pref(prf, reflect_listener_font, save_listener_font, NULL, clear_listener_font, revert_listener_font);

    current_sep = make_inter_variable_separator(prg_box);
    saved_listener_color = ss->sgx->listener_color;
    prf = prefs_color_selector_row("background color", S_listener_color, ss->sgx->listener_color,
				   prg_box,
				   listener_color_func);
    remember_pref(prf, NULL, save_listener_color, NULL, clear_listener_color, revert_listener_color);

    current_sep = make_inter_variable_separator(prg_box);
    saved_listener_text_color = ss->sgx->listener_text_color;
    prf = prefs_color_selector_row("text color", S_listener_text_color, ss->sgx->listener_text_color,
				   prg_box,
				   listener_text_color_func);
    remember_pref(prf, NULL, save_listener_text_color, NULL, clear_listener_text_color, revert_listener_text_color);
  }

  /* -------- audio -------- */
  {
    GtkWidget *aud_box, *aud_label;

    /* ---------------- audio options ---------------- */

    aud_box = make_top_level_box(topics);
    aud_label = make_top_level_label("audio options", aud_box);

    str = mus_format("%d", rts_dac_size = dac_size(ss));
    prf = prefs_row_with_text("dac buffer size", S_dac_size, 
			      str,
			      aud_box,
			      dac_size_text);
    remember_pref(prf, reflect_dac_size, save_dac_size, NULL, NULL, revert_dac_size);
    FREE(str);

    current_sep = make_inter_variable_separator(aud_box);
    prf = prefs_row_with_toggle("fold in otherwise unplayable channels", S_dac_combines_channels,
				rts_dac_combines_channels = dac_combines_channels(ss),
				aud_box,
				dac_combines_channels_toggle);
    remember_pref(prf, reflect_dac_combines_channels, save_dac_combines_channels, NULL, NULL, revert_dac_combines_channels);

    current_sep = make_inter_variable_separator(aud_box);
    make_inner_label("  recorder options", aud_box);

    rts_recorder_filename = copy_string(rec_filename());
    prf = prefs_row_with_text("recorder output file name", S_recorder_file, rec_filename(),
			      aud_box, 
			      recorder_filename_text);
    remember_pref(prf, reflect_recorder_filename, save_recorder_filename, NULL, NULL, revert_recorder_filename);
    current_sep = make_inter_variable_separator(aud_box);

    prf = prefs_row_with_toggle("automatically open the recorded sound", S_recorder_autoload,
				rts_recorder_autoload = rec_autoload(),
				aud_box, 
				recorder_autoload_toggle);
    remember_pref(prf, reflect_recorder_autoload, save_recorder_autoload, NULL, NULL, revert_recorder_autoload);
    current_sep = make_inter_variable_separator(aud_box);

    str = mus_format("%d", rts_recorder_buffer_size = rec_buffer_size());
    prf = prefs_row_with_text("input buffer size", S_recorder_buffer_size, 
			      str,
			      aud_box,
			      recorder_buffer_size_text);
    remember_pref(prf, reflect_recorder_buffer_size, save_recorder_buffer_size, NULL, NULL, revert_recorder_buffer_size);
    FREE(str);
    current_sep = make_inter_variable_separator(aud_box);

    rts_recorder_output_chans = rec_output_chans();
    prf = prefs_row_with_radio_box("default recorder output sound attributes: chans", S_recorder_out_chans,
				   recorder_out_chans_choices, NUM_RECORDER_OUT_CHANS_CHOICES, -1,
				   aud_box,
				   recorder_output_chans_choice);
    reflect_recorder_output_chans(prf);
    remember_pref(prf, reflect_recorder_output_chans, save_recorder_output_chans, NULL, NULL, revert_recorder_output_chans);

    rts_recorder_srate = rec_srate();
    prf = prefs_row_with_radio_box("srate", S_recorder_srate,
				   recorder_srate_choices, NUM_RECORDER_SRATE_CHOICES, -1,
				   aud_box,
				   recorder_srate_choice);
    reflect_recorder_srate(prf);
    remember_pref(prf, reflect_recorder_srate, save_recorder_srate, NULL, NULL, revert_recorder_srate);

    rts_recorder_output_header_type = rec_output_header_type();
    prf = prefs_row_with_radio_box("header type", S_recorder_out_header_type,
				   recorder_out_type_choices, NUM_RECORDER_OUT_TYPE_CHOICES, -1,
				   aud_box,
				   recorder_output_header_type_choice);
    recorder_output_header_type_prf = prf;
    remember_pref(prf, reflect_recorder_output_header_type, save_recorder_output_header_type, NULL, NULL, revert_recorder_output_header_type);

    rts_recorder_output_data_format = rec_output_data_format();
    prf = prefs_row_with_radio_box("data format", S_recorder_out_data_format,
				   recorder_out_format_choices, NUM_RECORDER_OUT_FORMAT_CHOICES, -1,
				   aud_box,
				   recorder_output_data_format_choice);
    recorder_output_data_format_prf = prf;
    remember_pref(prf, reflect_recorder_output_data_format, save_recorder_output_data_format, NULL, NULL, revert_recorder_output_data_format);
    reflect_recorder_output_header_type(recorder_output_header_type_prf);
    reflect_recorder_output_data_format(recorder_output_data_format_prf);
  }

  set_dialog_widget(PREFERENCES_DIALOG, preferences_dialog);
  gtk_widget_show(preferences_dialog);
  prefs_unsaved = false;
  prefs_set_dialog_title(NULL);
  return(preferences_dialog);
}
