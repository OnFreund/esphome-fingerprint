#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/uart/uart.h"
#include "Adafruit_Fingerprint.h"

namespace esphome {
namespace rxxx {

enum AuraLEDMode : uint8_t {
  BREATHING = FINGERPRINT_LED_BREATHING,
  FLASHING = FINGERPRINT_LED_FLASHING,
  ALWAYS_ON = FINGERPRINT_LED_ON,
  ALWAYS_OFF = FINGERPRINT_LED_OFF,
  GRADUAL_ON = FINGERPRINT_LED_GRADUAL_ON,
  GRADUAL_OFF = FINGERPRINT_LED_GRADUAL_OFF,
  RED = FINGERPRINT_LED_RED,
  BLUE = FINGERPRINT_LED_BLUE,
  PURPLE = FINGERPRINT_LED_PURPLE,
};

class RxxxComponent : public PollingComponent, public uart::UARTDevice {
  public:
  void update() override;
  void setup() override;
  void dump_config() override;

  void set_fingerprint_count_sensor(sensor::Sensor *fingerprint_count_sensor) {
    fingerprint_count_sensor_ = fingerprint_count_sensor;
  }
  void set_status_sensor(sensor::Sensor *status_sensor) { status_sensor_ = status_sensor; }
  void set_capacity_sensor(sensor::Sensor *capacity_sensor) { capacity_sensor_ = capacity_sensor; }
  void set_security_level_sensor(sensor::Sensor *security_level_sensor) {
    security_level_sensor_ = security_level_sensor;
  }
  void set_last_finger_id_sensor(sensor::Sensor *last_finger_id_sensor) {
    last_finger_id_sensor_ = last_finger_id_sensor;
  }
  void set_last_confidence_sensor(sensor::Sensor *last_confidence_sensor) {
    last_confidence_sensor_ = last_confidence_sensor;
  }
  void set_enrolling_binary_sensor(binary_sensor::BinarySensor *enrolling_binary_sensor) {
    enrolling_binary_sensor_ = enrolling_binary_sensor;
  }
  void set_sensing_pin(GPIOPin *sensing_pin) { this->sensing_pin_ = sensing_pin; }
  void set_password(uint32_t password) { password_ = password_; }
  void set_uart(Stream *uart_device) { finger_ = new Adafruit_Fingerprint(uart_device, password_); }
  void add_on_finger_scan_matched_callback(std::function<void(uint16_t, uint16_t)> callback) {
    this->finger_scan_matched_callback_.add(std::move(callback));
  }
  void add_on_finger_scan_unmatched_callback(std::function<void()> callback) {
    this->finger_scan_unmatched_callback_.add(std::move(callback));
  }
  void add_on_enrollment_scan_callback(std::function<void(uint8_t, uint16_t)> callback) {
    this->enrollment_scan_callback_.add(std::move(callback));
  }
  void add_on_enrollment_done_callback(std::function<void(uint16_t)> callback) {
    this->enrollment_done_callback_.add(std::move(callback));
  }

  void add_on_enrollment_failed_callback(std::function<void(uint16_t)> callback) {
    this->enrollment_failed_callback_.add(std::move(callback));
  }

  void enroll_fingerprint(uint16_t finger_id, uint8_t num_buffers);
  void finish_enrollment(uint8_t result);
  void delete_fingerprint(uint16_t finger_id);
  void delete_all_fingerprints();

  void led_control(bool on);
  void aura_led_control(uint8_t state, uint8_t speed, uint8_t color, uint8_t count);

  protected:

  void scan_and_match_();
  uint8_t scan_image_(uint8_t buffer);

  void get_fingerprint_count_();

  Adafruit_Fingerprint *finger_;
  uint32_t password_ = 0x0;
  GPIOPin *sensing_pin_{nullptr};
  uint8_t enrollment_image_ = 0;
  uint16_t enrollment_slot_ = 0;
  uint8_t enrollment_buffers_ = 5;
  bool waiting_removal_ = false;
  uint32_t last_aura_led_control_ = 0;
  uint16_t last_aura_led_duration_ = 0;
  sensor::Sensor *fingerprint_count_sensor_;
  sensor::Sensor *status_sensor_;
  sensor::Sensor *capacity_sensor_;
  sensor::Sensor *security_level_sensor_;
  sensor::Sensor *last_finger_id_sensor_;
  sensor::Sensor *last_confidence_sensor_;
  binary_sensor::BinarySensor *enrolling_binary_sensor_;
  CallbackManager<void(uint16_t, uint16_t)> finger_scan_matched_callback_;
  CallbackManager<void()> finger_scan_unmatched_callback_;
  CallbackManager<void(uint8_t, uint16_t)> enrollment_scan_callback_;
  CallbackManager<void(uint16_t)> enrollment_done_callback_;
  CallbackManager<void(uint16_t)> enrollment_failed_callback_;
};

class FingerScanMatchedTrigger : public Trigger<uint16_t, uint16_t> {
 public:
  explicit FingerScanMatchedTrigger(RxxxComponent *parent) {
    parent->add_on_finger_scan_matched_callback(
        [this](uint16_t finger_id, uint16_t confidence) {
          this->trigger(finger_id, confidence);
        });
  }
};

class FingerScanUnmatchedTrigger : public Trigger<> {
 public:
  explicit FingerScanUnmatchedTrigger(RxxxComponent *parent) {
    parent->add_on_finger_scan_unmatched_callback([this]() { this->trigger(); });
  }
};

class EnrollmentScanTrigger : public Trigger<uint8_t, uint16_t> {
 public:
  explicit EnrollmentScanTrigger(RxxxComponent *parent) {
    parent->add_on_enrollment_scan_callback(
        [this](uint8_t scan_number, uint16_t finger_id) {
          this->trigger(scan_number, finger_id);
        });
  }
};

class EnrollmentDoneTrigger : public Trigger<uint16_t> {
 public:
  explicit EnrollmentDoneTrigger(RxxxComponent *parent) {
    parent->add_on_enrollment_done_callback(
        [this](uint16_t finger_id) {
          this->trigger(finger_id);
        });
  }
};

class EnrollmentFailedTrigger : public Trigger<uint16_t> {
 public:
  explicit EnrollmentFailedTrigger(RxxxComponent *parent) {
    parent->add_on_enrollment_failed_callback(
        [this](uint16_t finger_id) {
          this->trigger(finger_id);
        });
  }
};

template<typename... Ts> class EnrollmentAction : public Action<Ts...>, public Parented<RxxxComponent> {
 public:
  TEMPLATABLE_VALUE(uint16_t, finger_id)
  TEMPLATABLE_VALUE(uint8_t, num_scans)

  void play(Ts... x) override {
    auto finger_id = this->finger_id_.value(x...);
    auto num_scans = this->num_scans_.value(x...);
    if (num_scans) {
      this->parent_->enroll_fingerprint(finger_id, num_scans);
    } else {
      this->parent_->enroll_fingerprint(finger_id, 2);
    }
  }
};

template<typename... Ts> class CancelEnrollmentAction : public Action<Ts...>, public Parented<RxxxComponent> {
 public:
  void play(Ts... x) override { this->parent_->finish_enrollment(1); }
};

template<typename... Ts> class DeleteAction : public Action<Ts...>, public Parented<RxxxComponent> {
 public:
  TEMPLATABLE_VALUE(uint16_t, finger_id)

  void play(Ts... x) override {
    auto finger_id = this->finger_id_.value(x...);
    this->parent_->delete_fingerprint(finger_id);
  }
};

template<typename... Ts> class DeleteAllAction : public Action<Ts...>, public Parented<RxxxComponent> {
 public:
  void play(Ts... x) override { this->parent_->delete_all_fingerprints(); }
};

template<typename... Ts> class LEDControlAction : public Action<Ts...>, public Parented<RxxxComponent> {
 public:
  TEMPLATABLE_VALUE(bool, state)

  void play(Ts... x) override {
    auto state = this->state_.value(x...);
    this->parent_->led_control(state);
  }
};

template<typename... Ts> class AuraLEDControlAction : public Action<Ts...>, public Parented<RxxxComponent> {
 public:
  TEMPLATABLE_VALUE(uint8_t, state)
  TEMPLATABLE_VALUE(uint8_t, speed)
  TEMPLATABLE_VALUE(uint8_t, color)
  TEMPLATABLE_VALUE(uint8_t, count)

  void play(Ts... x) override {

    auto state = this->state_.value(x...);
    auto speed = this->speed_.value(x...);
    auto color = this->color_.value(x...);
    auto count = this->count_.value(x...);

    this->parent_->aura_led_control(state, speed, color, count);
  }
};

}  // namespace rxxx
}  // namespace esphome