#pragma once
#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::chefkoch {

// The plugin: one tile type ("step"), the current step of the recipe ha-chefkoch is cooking.
class Chefkoch : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
};

}  // namespace esphome::chefkoch
