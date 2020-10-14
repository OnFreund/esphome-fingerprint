#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/uart/uart.h"
#include <Adafruit_Fingerprint.h>

namespace esphome {
namespace rxxx {

class RxxxComponent : public PollingComponent, public uart::UARTDevice {
  public:
  void update() override;
  void setup() override;
  void dump_config() override;

  void set_fingerprint_count_sensor(sensor::Sensor *fingerprint_count_sensor) { fingerprint_count_sensor_ = fingerprint_count_sensor; }
  void set_status_sensor(sensor::Sensor *status_sensor) { status_sensor_ = status_sensor; }
  void set_capacity_sensor(sensor::Sensor *capacity_sensor) { capacity_sensor_ = capacity_sensor; }
  void set_security_level_sensor(sensor::Sensor *security_level_sensor) { security_level_sensor_ = security_level_sensor; }
  void set_last_finger_id_sensor(sensor::Sensor *last_finger_id_sensor) { last_finger_id_sensor_ = last_finger_id_sensor; }
  void set_last_confidence_sensor(sensor::Sensor *last_confidence_sensor) { last_confidence_sensor_ = last_confidence_sensor; }
  void set_enrolling_binary_sensor(binary_sensor::BinarySensor *enrolling_binary_sensor) { enrolling_binary_sensor_ = enrolling_binary_sensor; }
  void set_sensing_pin(GPIOPin *sensing_pin) { sensing_pin_ = sensing_pin; }
  void set_password(uint32_t password) { password_ = password_; }
  void add_on_finger_scanned_callback(std::function<void(bool, int, int, int)> callback) {
    this->finger_scanned_callback_.add(std::move(callback));
  }
  void add_on_enrollment_scan_callback(std::function<void(int, int)> callback) {
    this->enrollment_scan_callback_.add(std::move(callback));
  }
  void add_on_enrollment_callback(std::function<void(bool, int, int)> callback) {
    this->enrollment_callback_.add(std::move(callback));
  }

  void enroll_fingerprint(int finger_id, int num_buffers);

  protected:

  void finish_enrollment(int result);
  void scan_and_match();
  int scan_image(int buffer);

  void get_fingerprint_count();

  Adafruit_Fingerprint *finger_;
  uint32_t password_ = 0x0;
  GPIOPin *sensing_pin_;
  int enrollmentImage_ = 0;
  int enrollmentSlot_ = 0;
  int enrollmentBuffers_ = 5;
  bool waitingRemoval = false;
  sensor::Sensor *fingerprint_count_sensor_;
  sensor::Sensor *status_sensor_;
  sensor::Sensor *capacity_sensor_;
  sensor::Sensor *security_level_sensor_;
  sensor::Sensor *last_finger_id_sensor_;
  sensor::Sensor *last_confidence_sensor_;
  binary_sensor::BinarySensor *enrolling_binary_sensor_;
  CallbackManager<void(bool, int, int, int)> finger_scanned_callback_;
  CallbackManager<void(bool, int)> enrollment_scan_callback_;
  CallbackManager<void(bool, int, int)> enrollment_callback_;
};

class FingerScannedTrigger : public Trigger<bool, int, int, int> {
 public:
  explicit FingerScannedTrigger(RxxxComponent *parent) {
    parent->add_on_finger_scanned_callback(
        [this](bool success, int result, int finger_id, int confidence) {
          this->trigger(success, result, finger_id, confidence);
        });
  }
};

class EnrollmentScanTrigger : public Trigger<int, int> {
 public:
  explicit EnrollmentScanTrigger(RxxxComponent *parent) {
    parent->add_on_enrollment_scan_callback(
        [this](int scan_number, int finger_id) {
          this->trigger(scan_number, finger_id);
        });
  }
};

class EnrollmentTrigger : public Trigger<bool, int, int> {
 public:
  explicit EnrollmentTrigger(RxxxComponent *parent) {
    parent->add_on_enrollment_callback(
        [this](bool success, int result, int finger_id) {
          this->trigger(success, result, finger_id);
        });
  }
};

template<typename... Ts> class FingerprintEnrollAction : public Action<Ts...> {
 public:
  FingerprintEnrollAction(RxxxComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(int, finger_id)
  TEMPLATABLE_VALUE(int, num_scans)

  void play(Ts... x) {
    auto finger_id = this->finger_id.value(x...);
    auto num_scans = this->num_scans.value(x...);
    this->parent_->enroll_fingerprint(finger_id, num_scans);
  }

 protected:
  RxxxComponent *parent_;
};

}  // namespace rxxx
}  // namespace esphome