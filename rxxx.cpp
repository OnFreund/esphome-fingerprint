#include "rxxx.h"
#include "esphome/core/log.h"
#include <string.h>

namespace esphome {
namespace rxxx {

static const char* TAG = "rxxx";

void RxxxComponent::update() {
  if (waitingRemoval) {
    if (finger_->getImage() == FINGERPRINT_NOFINGER) {
      waitingRemoval = false;
    }
    return;
  }

  if (enrollmentImage_ > enrollmentBuffers_) {
    ESP_LOGI(TAG, "Creating model");
    int result = finger_->createModel();
    if (result == FINGERPRINT_OK) {
      ESP_LOGI(TAG, "Storing model");
      result = finger_->storeModel(enrollmentSlot_);
      if (result == FINGERPRINT_OK) {
        ESP_LOGI(TAG, "Stored model");
      } else {
        ESP_LOGE(TAG, "Error storing model: %d", result);
      }
    } else {
      ESP_LOGE(TAG, "Error creating model: %d", result);
    }
    finish_enrollment(result);
    return;
  }

  if (this->sensing_pin_->digital_read() == HIGH) {
    ESP_LOGV(TAG, "No touch sensing");
    return;
  }

  if (enrollmentImage_ == 0) {
    scan_and_match();
    return;
  }

  int result = scan_image(enrollmentImage_);
  if (result == FINGERPRINT_NOFINGER) {
    return;
  }
  waitingRemoval = true;
  if (result != FINGERPRINT_OK) {
    finish_enrollment(result);
    return;
  }
  this->enrollment_scan_callback_.call(enrollmentImage_, enrollmentSlot_);
  ++enrollmentImage_;
}

void RxxxComponent::setup() {
  if (!finger_->verifyPassword()) {
    ESP_LOGE(TAG, "Could not find fingerprint sensor");
  }
  finger_->getParameters();
  status_sensor_->publish_state(finger_->status_reg);
  capacity_sensor_->publish_state(finger_->capacity);
  security_level_sensor_->publish_state(finger_->security_level);
  enrolling_binary_sensor_->publish_state(false);
  get_fingerprint_count();
}

void RxxxComponent::enroll_fingerprint(int finger_id, int num_buffers) {
  ESP_LOGD(TAG, "Starting enrollment in slot %d", finger_id);
  enrollmentSlot_ = finger_id, enrollmentBuffers_ = num_buffers, enrollmentImage_ = 1;
  enrolling_binary_sensor_->publish_state(true);
}

void RxxxComponent::finish_enrollment(int result) {
  this->enrollment_callback_.call(result == FINGERPRINT_OK, result, enrollmentSlot_);
  enrollmentImage_ = 0;
  enrollmentSlot_ = 0;
  enrolling_binary_sensor_->publish_state(false);
}

void RxxxComponent::scan_and_match() {
  int result = scan_image(1);
  int finger_id = -1;
  int confidence = 0;
  if (result == FINGERPRINT_NOFINGER) {
    return;
  }
  if (result == FINGERPRINT_OK) {
    result = finger_->fingerSearch();
    if (result == FINGERPRINT_OK) {
      finger_id = finger_->fingerID;
      last_finger_id_sensor_->publish_state(finger_id);
      confidence = finger_->confidence;
      last_confidence_sensor_->publish_state(confidence);
    }
  }
  waitingRemoval = true;
  this->finger_scanned_callback_.call(result == FINGERPRINT_OK, result, finger_id, confidence);
}

int RxxxComponent::scan_image(int buffer) {
  ESP_LOGD(TAG, "Getting image %d", buffer);
  int p = finger_->getImage();
  if (p != FINGERPRINT_OK) {
    ESP_LOGD(TAG, "No image. Result: %d", p);
    return p;
  }

  ESP_LOGD(TAG, "Processing image %d", buffer);
  p = finger_->image2Tz(buffer);
  switch (p) {
    case FINGERPRINT_OK:
      ESP_LOGI(TAG, "Processed image %d", buffer);
      return p;
    case FINGERPRINT_IMAGEMESS:
      ESP_LOGE(TAG, "Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      ESP_LOGE(TAG, "Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
    case FINGERPRINT_INVALIDIMAGE:
      ESP_LOGE(TAG, "Could not find fingerprint features");
      return p;
    default:
      ESP_LOGE(TAG, "Unknown error");
      return p;
  }
}

void RxxxComponent::get_fingerprint_count() {
  finger_->getTemplateCount();
  fingerprint_count_sensor_->publish_state(finger_->templateCount);
}

void RxxxComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "RXXX_FINGERPRINT_READER:");
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Fingerprint Count", this->fingerprint_count_sensor_);
  LOG_SENSOR("  ", "Status", this->status_sensor_);
  LOG_SENSOR("  ", "Capacity", this->capacity_sensor_);
  LOG_SENSOR("  ", "Security Level", this->security_level_sensor_);
  LOG_SENSOR("  ", "Last Finger ID", this->last_finger_id_sensor_);
  LOG_SENSOR("  ", "Last Confidence", this->last_confidence_sensor_);
}

}  // namespace rxxx
}  // namespace esphome