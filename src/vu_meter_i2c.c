#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "vu_config.h"
#include "vu_meter_i2c.h"

void vu_meter_i2c_init(vu_meter_i2c_t *meter,
              int8_t address,
              // calibration_data_t *calibration_eeprom,
              bool flipped,
              const uint8_t *background_image,
              const uint8_t *peak_indicator_image,
              const uint8_t *watermark_image)
{
  meter->flipped = flipped;

  // calibration_init(&(meter->calibration), calibration_eeprom);

  display_init(&(meter->display), address);
  progmem_image_sprite_init(&(meter->background), background_image, 0, 0);

  display_add_sprite(&(meter->display), &(meter->background.sprite));

#if ENABLE_WATERMARK
  progmem_image_sprite_init(
    &(meter->watermark),
    watermark_image,
    WATERMARK_POSITION_X,
    WATERMARK_POSITION_Y
  );

  display_add_sprite(&(meter->display), &(meter->watermark.sprite));
#endif

  progmem_image_sprite_init(
    &(meter->peak_indicator),
    peak_indicator_image,
    PEAK_POSITION_X,
    PEAK_POSITION_Y
  );
  display_add_sprite(&(meter->display), &(meter->peak_indicator.sprite));

  needle_sprite_init(&(meter->needle));
  needle_sprite_draw(&(meter->needle), 0);
  display_add_sprite(&(meter->display), &(meter->needle.sprite));

  meter->peak_indicator.sprite.visible = false;
}

/**
 * @param needle_level Range [0-255]. Corresponding to min/max needle angle
 * @param peak_level
 */
void vu_meter_i2c_update(vu_meter_i2c_t *meter, uint16_t needle_level, uint16_t peak_level)
{
  #if INCLUDE_CALIBRATION
    calibration_run(
      &(meter->calibration),
      needle_level,
      peak_level
    );
  #endif

  // uint8_t angle = calibration_adc_to_angle(&(meter->calibration), needle_level);
  uint8_t angle = (uint8_t)((needle_level * 255) / 1024); // TODO: Check
  bool peak;
  printf("Needle %d Angle %d\n", needle_level, angle);

#if PEAK_IS_ZERO_VU
  peak = (angle > NEEDLE_ZERO_VU_ANGLE);
#else
  // peak = calibration_adc_to_peak(&(meter->calibration), peak_level);
  peak = 0;   // TODO: Check
#endif

  if (meter->flipped) {
    angle = 255 - angle;
  }

  if (peak) {
    meter->peak_timer = 10;
  }
  else if (meter->peak_timer > 0) {
    --(meter->peak_timer);
  }

  bool new_visible = (meter->peak_timer > 0);
  meter->peak_indicator.sprite.changed = (meter->peak_indicator.sprite.visible != new_visible);
  meter->peak_indicator.sprite.visible = new_visible;

  needle_sprite_draw(&(meter->needle), angle);
  display_update(&(meter->display));
}


void vu_meter_i2c_splash(vu_meter_i2c_t *meter, const uint8_t *image)
{
  const uint8_t *background_tmp = meter->background.data;
  uint8_t sprites_n = meter->display.sprites_n;

  progmem_image_sprite_init(&(meter->background), image, 0, 0);
  meter->display.sprites_n = 1;

  display_force_full_update(&(meter->display));
  display_update(&(meter->display));

  progmem_image_sprite_init(&(meter->background), background_tmp, 0, 0);
  meter->display.sprites_n = sprites_n;
}

