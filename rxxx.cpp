#include "rxxx.h"
#include "esphome/core/log.h"
#include <string.h>

namespace esphome {
namespace rxxx {

static const char* TAG = "rxxx";

void RxxxComponent::update() {
  if (waitingRemoval) {
    if (FINGERPRINT_NOFINGER == finger_->getImage()) {
      waitingRemoval = false;
    }
    return;
  }

  if (enrollementImage_ > enrollementBuffers_) {
    ESP_LOGI(TAG, "Creating model");
    int result = finger_->createModel();
    if (FINGERPRINT_OK == result) {
      ESP_LOGI(TAG, "Storing model");
      result = finger_->storeModel(enrollementSlot_);
      if (FINGERPRINT_OK == result) {
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

  if (HIGH == digitalRead(sensing_pin_)) {
    ESP_LOGV(TAG, "No touch sensing");
    return;
  }

  if (0 == enrollementImage_) {
    scan_and_match();
    return;

  int result = scan_image(enrollementImage_);
  if (FINGERPRINT_NOFINGER == result) {
    return;
  }
  waitingRemoval = true;
  if (result != FINGERPRINT_OK) {
    finish_enrollment(result);
    return;
  }
  this->enrollment_scan_callback_.call(enrollementImage_, finger_id)
  ++enrollementImage_;
}

void RxxxComponent::setup() {
  pinMode(sensing_pin_, INPUT);
  finger_->begin(57600);
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

void RxxxComponent::finish_enrollment(int result) {
  this->enrollment_callback_.call(FINGERPRINT_OK == result, result, enrollementSlot_)
  enrollementImage_ = 0;
  enrollementSlot_ = 0;
  enrolling_binary_sensor_->publish_state(false);
}

void RxxxComponent::scan_and_match() {
  int result = scan_image(1);
  int finger_id = -1;
  int confidence = 0;
  if (FINGERPRINT_NOFINGER == result) {
    return;
  }
  if (FINGERPRINT_OK == result) {
    result = finger_->fingerSearch();
    if (FINGERPRINT_OK == result) {
      finger_id = finger_->fingerID;
      last_finger_id_sensor_->publish_state(finger_id);
      confidence = finger_->confidence;
      last_confidence_sensor_->publish_state(confidence);
    }
  }
  waitingRemoval = true;
  this->finger_scanned_callback_.call(FINGERPRINT_OK == result, result, finger_id, confidence)
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

void RxxxComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "RXXX_FINGERPRINT_READER:");
  // ESP_LOGCONFIG(TAG, "  RSSI: %d dB", this->rssi_);
}

}  // namespace rxxx
}  // namespace esphome