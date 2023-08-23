#include "speed_fan.h"
#include "esphome/core/log.h"

namespace esphome {
namespace speed {

static const char *const TAG = "speed.fan";

void SpeedFan::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(*this);
    this->write_state_();
  }
}
void SpeedFan::dump_config() { LOG_FAN("", "Speed Fan", this); }
fan::FanTraits SpeedFan::get_traits() {
  return fan::FanTraits(this->oscillating_ != nullptr, true, this->direction_ != nullptr, this->speed_count_);
}
void SpeedFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value())
    this->state = *call.get_state();
  if (call.get_speed().has_value())
    this->speed = *call.get_speed();
  if (call.get_oscillating().has_value())
    this->oscillating = *call.get_oscillating();
  if (call.get_direction().has_value())
    this->direction = *call.get_direction();

  this->write_state_();
  this->publish_state();
}
void SpeedFan::write_state_() {
  static bool previous_state = false;

  int speed_to_use = this->speed;
  if (this->state) {
    if (!previous_state && this->speed_kickstart_ > speed_to_use) {
      // Fan needs to start spinning. Give it a kick.
      // TODO: somehow, after enough time has passed to make the fan spin, go back
      // to the actually intended speed.
      speed_to_use = this->speed_kickstart_;
    }
    speed_to_use = max(speed_to_use, this->speed_min_);
  } else {
    speed_to_use = 0;
  }
  previous_state = this->state;

  if (this->state && this->speed != speed_to_use) {
    ESP_LOGV(TAG, "'%s': using speed %d (instead of %d)", this->name_.c_str(), speed_to_use, this->speed);
  }

  float speed = static_cast<float>(speed_to_use) / static_cast<float>(this->speed_count_);
  this->output_->set_level(speed);

  if (this->oscillating_ != nullptr)
    this->oscillating_->set_state(this->oscillating);
  if (this->direction_ != nullptr)
    this->direction_->set_state(this->direction == fan::FanDirection::REVERSE);
}

}  // namespace speed
}  // namespace esphome
