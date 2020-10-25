#include "rxxx.h"
#include "esphome/core/log.h"
#include <string.h>

namespace esphome {
namespace rxxx {

static const char* TAG = "rxxx";

void RxxxComponent::update() {
  if (this->waitingRemoval) {
    if (this->finger_->getImage() == FINGERPRINT_NOFINGER) {
      ESP_LOGD(TAG, "Finger removed");
      this->waitingRemoval = false;
    }
    return;
  }

  if (this->enrollmentImage_ > this->enrollmentBuffers_) {
    ESP_LOGI(TAG, "Creating model");
    uint8_t result = this->finger_->createModel();
    if (result == FINGERPRINT_OK) {
      ESP_LOGI(TAG, "Storing model");
      result = this->finger_->storeModel(this->enrollmentSlot_);
      if (result == FINGERPRINT_OK) {
        ESP_LOGI(TAG, "Stored model");
      } else {
        ESP_LOGE(TAG, "Error storing model: %d", result);
      }
    } else {
      ESP_LOGE(TAG, "Error creating model: %d", result);
    }
    this->finish_enrollment(result);
    return;
  }

  if (this->sensing_pin_->digital_read() == HIGH) {
    ESP_LOGV(TAG, "No touch sensing");
    return;
  }

  if (this->enrollmentImage_ == 0) {
    ESP_LOGD(TAG, "Scan and match");
    this->scan_and_match();
    return;
  }

  uint8_t result = this->scan_image(this->enrollmentImage_);
  if (result == FINGERPRINT_NOFINGER) {
    return;
  }
  this->waitingRemoval = true;
  if (result != FINGERPRINT_OK) {
    this->finish_enrollment(result);
    return;
  }
  this->enrollment_scan_callback_.call(this->enrollmentImage_, this->enrollmentSlot_);
  ++this->enrollmentImage_;
}

void RxxxComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Rxxx Fingerprint Sensor...");
  if (!this->finger_->verifyPassword()) {
    ESP_LOGE(TAG, "Could not find fingerprint sensor");
    this->mark_failed();
  }
  this->finger_->getParameters();
  this->status_sensor_->publish_state(this->finger_->status_reg);
  this->capacity_sensor_->publish_state(this->finger_->capacity);
  this->security_level_sensor_->publish_state(this->finger_->security_level);
  this->enrolling_binary_sensor_->publish_state(false);
  this->get_fingerprint_count();
}

void RxxxComponent::enroll_fingerprint(uint16_t finger_id, uint8_t num_buffers) {
  ESP_LOGD(TAG, "Starting enrollment in slot %d", finger_id);
  this->enrolling_binary_sensor_->publish_state(true);
  this->enrollmentSlot_ = finger_id, this->enrollmentBuffers_ = num_buffers, this->enrollmentImage_ = 1;
}

void RxxxComponent::finish_enrollment(uint8_t result) {
  this->enrollment_done_callback_.call(result == FINGERPRINT_OK, result, this->enrollmentSlot_);
  this->enrollmentImage_ = 0;
  this->enrollmentSlot_ = 0;
  this->get_fingerprint_count();
  this->enrolling_binary_sensor_->publish_state(false);
}

void RxxxComponent::scan_and_match() {
  uint8_t result = this->scan_image(1);
  ESP_LOGD(TAG, "Image scanned");
  if (result == FINGERPRINT_NOFINGER) {
    return;
  }
  int finger_id = -1;
  uint16_t confidence = 0;
  this->waitingRemoval = true;
  if (result == FINGERPRINT_OK) {
    result = this->finger_->fingerSearch();
    ESP_LOGD(TAG, "Finger searched");
    if (result == FINGERPRINT_OK) {
      finger_id = this->finger_->fingerID;
      this->last_finger_id_sensor_->publish_state(finger_id);
      confidence = this->finger_->confidence;
      this->last_confidence_sensor_->publish_state(confidence);
    }
  }
  this->finger_scanned_callback_.call(result == FINGERPRINT_OK, result, finger_id, confidence);
}

uint8_t RxxxComponent::scan_image(uint8_t buffer) {
  ESP_LOGD(TAG, "Getting image %d", buffer);
  uint8_t p = this->finger_->getImage();
  if (p != FINGERPRINT_OK) {
    ESP_LOGD(TAG, "No image. Result: %d", p);
    return p;
  }

  ESP_LOGD(TAG, "Processing image %d", buffer);
  p = this->finger_->image2Tz(buffer);
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
  this->finger_->getTemplateCount();
  this->fingerprint_count_sensor_->publish_state(this->finger_->templateCount);
}

void RxxxComponent::delete_all_fingerprints() {
  ESP_LOGI(TAG, "Deleting all stored fingerprints");
  uint8_t result = this->finger_->emptyDatabase();
    if (result == FINGERPRINT_OK) {
      ESP_LOGI(TAG, "Successfully deleted all fingerprints");
      this->get_fingerprint_count();
    } else {
      ESP_LOGE(TAG, "Error deleting all fingerprints: %d", result);
    }
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