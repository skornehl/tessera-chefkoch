#include "chefkoch.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace esphome::chefkoch {

using tessera::Font;
namespace ui = tessera::ui;

// The tile: the recipe's name and where we are on top, the step's words as large as they fit in the middle, and three
// keys at the bottom (back, the step's timer, next or done). A tap on the words goes on (right half) or back (left).
//
// Home Assistant sends the step's words as the list `teile`, one word per item and "\n" for a new line, because the
// app cuts every text of a tile's data at 48 bytes; a list of words reaches the screen whole up to about 2.5 KB.
class StepTile : public tessera::Tile {
 public:
  explicit StepTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    back_ = c.options["back"] | "";
    next_ = c.options["next"] | "";
    finish_ = c.options["finish"] | "";
    timer_ = c.options["timer"] | "";

    gap_ = ui::px(ui::large() ? 12 : 6);
    head_h_ = ui::line_height(Font::TITLE);
    key_h_ = std::max(ui::mm(12), ui::line_height(Font::BODY_LARGE) + ui::px(16));
    keys_y_ = height_ - key_h_;
    body_y_ = head_h_ + gap_;
    body_h_ = std::max(1, keys_y_ - gap_ - body_y_);

    title_ = ui::label(c.parent, Font::TITLE, theme::INK);
    lv_obj_set_pos(title_, 0, 0);
    where_ = ui::label(c.parent, Font::BODY_LARGE, theme::MUTED);
    lv_obj_set_style_text_align(where_, LV_TEXT_ALIGN_RIGHT, 0);

    // Up to three columns of words: a step is one, a long list of ingredients takes two or three (paint()).
    for (auto *&col : body_) {
      col = ui::text(c.parent, Font::BODY_LARGE, theme::INK);
      lv_obj_set_pos(col, 0, body_y_);
      lv_obj_set_size(col, width_, body_h_);
    }

    // Three keys of equal width; the middle one (timer) only shows when the step has a timer.
    key_w_ = std::max(1, (width_ - 2 * gap_) / 3);
    for (int i = 0; i < 3; ++i) {
      keys_[i] = ui::block(c.parent, i == 2 ? theme::ACCENT : theme::TRACK);
      lv_obj_set_size(keys_[i], key_w_, key_h_);
      lv_obj_set_pos(keys_[i], i * (key_w_ + gap_), keys_y_);
      key_labels_[i] = ui::label(keys_[i], Font::BODY_LARGE, i == 2 ? theme::ON_ACCENT : theme::INK);
      lv_obj_set_width(key_labels_[i], key_w_ - ui::px(8));
      lv_obj_set_style_text_align(key_labels_[i], LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_center(key_labels_[i]);
    }

    // What the tile says while nothing is cooking, in the middle of the card.
    empty_ = ui::text(c.parent, Font::HEADLINE, theme::MUTED);
    lv_obj_set_size(empty_, width_, ui::line_height(Font::HEADLINE) * 2 + ui::line_height(Font::BODY_LARGE));
    lv_obj_set_style_text_align(empty_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(empty_, 0, std::max(0, height_ / 2 - ui::line_height(Font::HEADLINE)));
  }

  void on_state(JsonObjectConst data) override {
    chosen_ = !data["state"].isNull();
    const char *state = data["state"] | "";
    JsonObjectConst a = data["attributes"];
    recipe_ = a["rezept"] | "";
    step_ = number(a["schritt"]);
    total_ = number(a["schritte_gesamt"]);
    timer_min_ = number(a["timer_minuten"]);
    last_ = truth(a["letzter_schritt"]);
    words_.clear();
    bool line_start = true;
    for (JsonVariantConst part : a["teile"].as<JsonArrayConst>()) {
      const char *w = part | "";
      if (std::strcmp(w, "\n") == 0) {
        words_ += "\n";
        line_start = true;
        continue;
      }
      if (!*w) continue;
      if (!line_start) words_ += " ";
      words_ += w;
      line_start = false;
    }
    cooking_ = chosen_ && !words_.empty() && std::strcmp(state, "unavailable") != 0 &&
               std::strcmp(state, "unknown") != 0;
    dirty_ = true;
  }

  void on_tick(uint32_t) override {
    if (!dirty_) return;
    dirty_ = false;
    paint();
  }

  void on_theme() override {
    ui::set_color(title_, theme::INK);
    ui::set_color(where_, theme::MUTED);
    for (auto *col : body_) ui::set_color(col, theme::INK);
    ui::set_color(empty_, theme::MUTED);
    for (int i = 0; i < 3; ++i) ui::set_color(key_labels_[i], i == 2 ? theme::ON_ACCENT : theme::INK);
    dirty_ = true;
  }

  void on_tap() override {
    if (!cooking_) return;
    const tessera::Point p = tessera::tap_point();
    if (p.x < 0) return;
    if (p.y >= keys_y_) {
      const int key = std::min(2, p.x / std::max(1, key_w_ + gap_));
      if (key == 0 && show_back()) press(back_);
      else if (key == 1 && show_timer()) tessera::action("script.turn_on", roomy::plain(timer_));
      else if (key == 2) press(last_ && !finish_.empty() ? finish_ : next_);
      return;
    }
    if (p.x >= width_ / 2) press(last_ && !finish_.empty() ? finish_ : next_);
    else if (show_back()) press(back_);
  }

 private:
  static int number(JsonVariantConst v) {
    if (v.is<int>()) return v.as<int>();
    const char *s = v | "";
    return static_cast<int>(std::strtol(s, nullptr, 10));
  }
  static bool truth(JsonVariantConst v) {
    if (v.is<bool>()) return v.as<bool>();
    const char *s = v | "";
    return std::strcmp(s, "True") == 0 || std::strcmp(s, "true") == 0;
  }
  bool show_back() const { return step_ > 0 && !back_.empty(); }
  bool show_timer() const { return step_ > 0 && timer_min_ > 0 && !timer_.empty(); }
  static void press(const tessera::Text &button) {
    if (!button.empty()) tessera::action("button.press", roomy::plain(button));
  }
  int column_width(int n) const { return std::max(1, (width_ - (n - 1) * 2 * gap_) / n); }

  // Whether the lines, cut into n columns in their order, fit the body in font f.
  bool fits(const std::vector<std::string> &lines, int n, Font f, int32_t line_space) const {
    const size_t per = (lines.size() + n - 1) / n;
    const int w = column_width(n);
    for (int i = 0; i < n; ++i) {
      std::string part;
      for (size_t l = i * per; l < lines.size() && l < (i + 1) * per; ++l) {
        if (!part.empty()) part += "\n";
        part += lines[l];
      }
      lv_point_t size;
      lv_text_get_size(&size, part.c_str(), ui::font(f), 0, line_space, w, LV_TEXT_FLAG_NONE);
      if (size.y > body_h_) return false;
    }
    return true;
  }

  static void show(lv_obj_t *obj, bool on) {
    if (on) lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }

  void paint() {
    show(empty_, !cooking_);
    show(title_, cooking_);
    show(where_, cooking_);
    for (auto *col : body_) show(col, cooking_);
    if (!cooking_) {
      for (auto *k : keys_) show(k, false);
      const std::string msg = chosen_ ? std::string(plugin_->text("nothing")) + "\n" + plugin_->text("nothing_hint")
                                      : std::string(plugin_->text("choose"));
      ui::set_text(empty_, msg);
      return;
    }

    // Top: where we are on the right, the recipe's name in the room left of it.
    std::string where = step_ <= 0 ? std::string(plugin_->text("ingredients"))
                                   : tessera::fill(tessera::format(plugin_->text("step"), step_).c_str(), "total",
                                                   std::to_string(total_));
    const int where_w = std::min(width_ / 2, ui::text_width(where, Font::BODY_LARGE) + ui::px(4));
    lv_obj_set_size(where_, where_w, ui::line_height(Font::BODY_LARGE));
    lv_obj_set_pos(where_, width_ - where_w, (head_h_ - ui::line_height(Font::BODY_LARGE)) / 2);
    ui::set_text(where_, where);
    lv_obj_set_width(title_, std::max(1, width_ - where_w - gap_));
    ui::set_text(title_, roomy::plain(recipe_));

    // The words in the largest font they fit in; lines (the ingredients) may spread over two or three columns, and
    // of two layouts in the same font the one with fewer columns wins.
    const std::string text = roomy::plain(words_);
    std::vector<std::string> lines;
    for (size_t at = 0;;) {
      const size_t end = text.find('\n', at);
      lines.push_back(text.substr(at, end == std::string::npos ? std::string::npos : end - at));
      if (end == std::string::npos) break;
      at = end + 1;
    }
    const int32_t line_space = lv_obj_get_style_text_line_space(body_[0], LV_PART_MAIN);
    Font face = Font::BODY;
    int columns = 1;
    bool found = false;
    for (Font f : {Font::HEADLINE, Font::BODY_LARGE, Font::BODY}) {
      if (f != Font::BODY && !ui::covers(text, f)) continue;
      for (int n = 1; n <= 3 && n <= static_cast<int>(lines.size()) && !found; ++n) {
        if (fits(lines, n, f, line_space)) {
          face = f;
          columns = n;
          found = true;
        }
      }
      if (found) break;
    }
    if (!found) columns = std::min<int>(3, lines.size());
    const int col_w = column_width(columns);
    const size_t per = (lines.size() + columns - 1) / columns;
    for (int i = 0; i < 3; ++i) {
      std::string part;
      for (size_t l = i * per; i < columns && l < lines.size() && l < (i + 1) * per; ++l) {
        if (!part.empty()) part += "\n";
        part += lines[l];
      }
      show(body_[i], i < columns);
      lv_obj_set_style_text_font(body_[i], ui::font(face), 0);
      lv_obj_set_size(body_[i], col_w, body_h_);
      lv_obj_set_pos(body_[i], i * (col_w + 2 * gap_), body_y_);
      ui::set_text(body_[i], part);
    }

    // The keys.
    show(keys_[0], show_back());
    ui::set_text(key_labels_[0], plugin_->text("back"));
    show(keys_[1], show_timer());
    ui::set_text(key_labels_[1], tessera::format(plugin_->text("timer"), timer_min_));
    show(keys_[2], true);
    ui::set_text(key_labels_[2], plugin_->text(last_ && !finish_.empty() ? "finish" : "next"));
    for (auto *l : key_labels_) lv_obj_center(l);
  }

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, gap_ = 0, head_h_ = 0, key_h_ = 0, key_w_ = 0, keys_y_ = 0, body_y_ = 0, body_h_ = 0;
  int step_ = 0, total_ = 0, timer_min_ = 0;
  bool chosen_ = false, cooking_ = false, last_ = false, dirty_ = true;
  tessera::Text back_, next_, finish_, timer_, recipe_, words_;
  lv_obj_t *title_{}, *where_{}, *empty_{};
  lv_obj_t *body_[3]{};
  lv_obj_t *keys_[3]{}, *key_labels_[3]{};
};

void Chefkoch::setup() {
  add_tile("step", [this]() { return new StepTile(this); });
}

}  // namespace esphome::chefkoch
