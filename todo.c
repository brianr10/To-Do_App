#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <leif/leif.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum {
  ALL = 0,
  IN_PROGRESS,
  COMPLETED,
  LOW,
  MEDIUM,
  HIGH
} entry_filter;

typedef enum {
  FILTER_ALL = 0,
  FILTER_IN_PROGRESS,
  FILTER_COMPLETED,
  FILTER_LOW,
  FILTER_MEDIUM,
  FILTER_HIGH
} todo_filter;

typedef enum { TAB_DASHBOARD = 0, TAB_NEW_TASK } tab;

typedef enum {
  PRIORITY_LOW = 0,
  PRIORITY_MEDIUM,
  PRIORITY_HIGH
} entry_priority;

typedef struct {
  bool completed;
  char *desc, *date;
  entry_priority priority;
} task_entry;

#define WIN_MARGIN 20.0F

static int winw = 1280, winh = 720;
static LfFont titlefont;
static LfFont smallfont;
static entry_filter current_filter;

static task_entry *entries[1024];
static uint32_t numentries = 0;

static LfTexture removetexture;

static void rendertopbar() {
  lf_push_font(&titlefont);
  lf_text("Your To-Do List");

  {
    const float width = 160.0f;

    lf_set_ptr_x_absolute(winw - width - WIN_MARGIN * 2.0f);
    LfUIElementProps props = lf_get_theme().button_props;
    props.margin_left = 0.0f;
    props.margin_right = 0.0f;
    props.color = (LfColor){65, 167, 204, 255};
    props.border_width = 0.0f;
    props.corner_radius = 4.0f;
    lf_push_style_props(props);
    lf_set_line_should_overflow(false);
    lf_button_fixed("New Task", 160, -1);
    lf_set_line_should_overflow(true);
    lf_pop_style_props();
  }
}

static void renderfilters() {
  const uint32_t numfilters = 6;
  static const char *filters[] = {"ALL", "IN PROGRESS", "COMPLETED",
                                  "LOW", "MEDIUM",      "HIGH"};

  LfUIElementProps props = lf_get_theme().button_props;
  props.color = LF_NO_COLOR;
  props.margin_top = 30.0f;
  props.margin_right = 10.0f;
  props.margin_left = 10.0f;
  props.padding = 10.0f;
  props.border_width = 0.0f;
  props.text_color = LF_WHITE;
  props.corner_radius = 8.0f;

  float width = 0.0f;
  float ptrx_before = lf_get_ptr_x();
  float ptry_before = lf_get_ptr_y();
  lf_push_style_props(props);
  lf_set_no_render(true);
  lf_set_ptr_y_absolute(lf_get_ptr_y() + 50.0f);

  for (uint32_t i = 0; i < numfilters; i++) {
    lf_button(filters[i]);
  }

  lf_set_no_render(false);
  lf_set_ptr_y_absolute(ptry_before);

  width = lf_get_ptr_x() - ptrx_before - props.margin_right - props.padding;

  lf_set_ptr_x_absolute(winw - width - WIN_MARGIN);

  lf_set_line_should_overflow(false);

  for (uint32_t i = 0; i < numfilters; i++) {
    props.color = (current_filter == (entry_filter)i)
                      ? (LfColor){255, 255, 255, 50}
                      : LF_NO_COLOR;
    lf_push_style_props(props);
    if (lf_button(filters[i]) == LF_CLICKED) {
      current_filter = (entry_filter)i;
    }
    lf_pop_style_props();
  }
  lf_set_line_should_overflow(true);
}

static void renderentries() {

  lf_div_begin(((vec2s){lf_get_ptr_x(), lf_get_ptr_y()}),
               ((vec2s){winw - lf_get_ptr_x() - WIN_MARGIN,
                        (winh - lf_get_ptr_y() - WIN_MARGIN)}),
               true);
  uint32_t renderedcount = 0;

  for (uint32_t i = 0; i < numentries; i++) {
    task_entry *entry = entries[i];

    if (current_filter == FILTER_LOW && entry->priority != PRIORITY_LOW)
      continue;
    if (current_filter == FILTER_MEDIUM && entry->priority != PRIORITY_MEDIUM)
      continue;
    if (current_filter == FILTER_HIGH && entry->priority != PRIORITY_HIGH)
      continue;
    if (current_filter == FILTER_COMPLETED && !entry->completed)
      continue;

    if (current_filter == FILTER_IN_PROGRESS && entry->completed)
      continue;

    float priority_size = 15.0f;
    // float ptry_before = lf_get_ptr_y();
    lf_set_ptr_y_absolute(lf_get_ptr_y() + 5.0f);
    lf_set_ptr_x_absolute(lf_get_ptr_x() + 5.0f);

    switch (entry->priority) {
    case PRIORITY_LOW: {
      lf_rect(priority_size, priority_size, (LfColor){76, 175, 80, 255}, 4.0f);
      break;
    }
    case PRIORITY_MEDIUM: {
      lf_rect(priority_size, priority_size, (LfColor){255, 235, 59, 255}, 4.0f);
      break;
    }
    case PRIORITY_HIGH: {
      lf_rect(priority_size, priority_size, (LfColor){244, 67, 54, 255}, 4.0f);
      break;
    }
    }

    {
      LfUIElementProps props = lf_get_theme().button_props;
      props.color = LF_NO_COLOR;
      props.border_width = 0.0f;
      props.padding = 0.0f;
      props.margin_top = 3.0f;
      props.margin_left = 10.0f;
      lf_push_style_props(props);
      if (lf_image_button(((LfTexture){
              .id = removetexture.id, .width = 20.0f, .height = 20.f})) ==
          LF_CLICKED) {
        for (uint32_t j = i; j < numentries; j++) {
          entries[j] = entries[j + 1];
        }
        numentries--;
      }
      lf_pop_style_props();
    }

    {
      LfUIElementProps props = lf_get_theme().checkbox_props;
      props.border_width = 1.0f;
      props.corner_radius = 0.0f;
      props.margin_top = 0.0f;
      props.padding = 5.0f;
      props.margin_left = 5.0f;
      props.color = lf_color_from_zto((vec4s){0.05f, 0.05f, 0.05f, 1.0f});
      lf_push_style_props(props);
      if (lf_checkbox("", &entry->completed, LF_NO_COLOR,
                      ((LfColor){65, 167, 204, 255})) == LF_CLICKED) {
      }
      lf_pop_style_props();
    }
    lf_push_font(&smallfont);
    LfUIElementProps props = lf_get_theme().text_props;
    props.margin_top = 4.0f;
    props.margin_left = 5.0f;
    lf_push_style_props(props);
    float descptr_x = lf_get_ptr_x();
    lf_text(entry->desc);

    lf_set_ptr_x_absolute(descptr_x);
    lf_set_ptr_y_absolute(lf_get_ptr_y() + smallfont.font_size);
    props.text_color = (LfColor){150, 150, 150, 255};
    lf_push_style_props(props);
    lf_text(entry->date);
    lf_pop_style_props();
    lf_next_line();

    lf_pop_font();

    renderedcount++;
  }
  if (!renderedcount) {
    lf_text("There is no task in your To-Do.");
  }
}

int main() {
  glfwInit();

  GLFWwindow *window = glfwCreateWindow(winw, winh, "To-Do", NULL, NULL);

  glfwMakeContextCurrent(window);

  lf_init_glfw(winw, winh, window);

  LfTheme theme = lf_get_theme();
  theme.div_props.color = LF_NO_COLOR;
  lf_set_theme(theme);

  titlefont = lf_load_font("./fonts/inter-bold.ttf", 40);
  smallfont = lf_load_font("./fonts/inter.ttf", 20);

  removetexture =
      lf_load_texture("./icons/remove.png", true, LF_TEX_FILTER_LINEAR);

  for (uint32_t i = 0; i < 5; i++) {

    task_entry *entry = (task_entry *)malloc(sizeof(*entry));
    entry->priority = PRIORITY_LOW;
    entry->completed = false;
    entry->date = "NULL";
    entry->desc = "Complete tutorials";
    entries[numentries++] = entry;
  }

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    lf_begin();

    lf_div_begin(((vec2s){WIN_MARGIN, WIN_MARGIN}),
                 ((vec2s){winw - WIN_MARGIN * 2.0f, winh - WIN_MARGIN * 2.0}),
                 true);

    rendertopbar();
    lf_next_line();
    renderfilters();
    lf_next_line();

    renderentries();

    lf_div_end();

    // lf_pop_font();
    lf_div_end();
    lf_end();

    glfwPollEvents();
    glfwSwapBuffers(window);
  }

  lf_free_font(&titlefont);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
