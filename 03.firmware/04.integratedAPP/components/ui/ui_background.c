/**
 * @file ui_background.c
 * @brief Background image management implementation
 *
 * Uses ESP32-P4 hardware JPEG decoder for fast JPEG loading.
 * Supports MJPEG video backgrounds using the mjpeg_player component.
 */

#include "ui_background.h"
#include "ui.h"
#include "sdcard.h"
#include "mjpeg_player.h"
#include "bsp/jc4880p443c.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/jpeg_decode.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>
#include <strings.h>

static const char *TAG = "UI_BG";

// LVGL filesystem drive letter for POSIX (CONFIG_LV_FS_POSIX_LETTER=83='S')
#define LVGL_FS_PREFIX "S:"

// Max JPEG file size for hardware decoder (256KB)
#define MAX_JPEG_FILE_SIZE (256 * 1024)

// ============================================================================
// Shared JPEG Decoder (Singleton)
// ============================================================================
// ESP32-P4 hardware JPEG decoder requires internal DMA memory for rxlink.
// To avoid running out of internal memory, we share a single decoder instance.

static jpeg_decoder_handle_t s_shared_jpeg_decoder = NULL;
static SemaphoreHandle_t s_decoder_mutex = NULL;

esp_err_t ui_bg_init_jpeg_decoder(void)
{
    if (s_decoder_mutex == NULL) {
        s_decoder_mutex = xSemaphoreCreateMutex();
        if (s_decoder_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create decoder mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_shared_jpeg_decoder != NULL) {
        ESP_LOGD(TAG, "Shared JPEG decoder already initialized");
        return ESP_OK;
    }

    // Log available internal DMA memory before allocation
    size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Free internal DMA memory before JPEG init: %u bytes", free_dma);

    jpeg_decode_engine_cfg_t decode_eng_cfg = {
        .timeout_ms = 100,
    };
    esp_err_t ret = jpeg_new_decoder_engine(&decode_eng_cfg, &s_shared_jpeg_decoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create shared JPEG decoder: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Free internal DMA memory: %u bytes (may need more for rxlink)", free_dma);
        return ret;
    }

    ESP_LOGI(TAG, "Hardware JPEG decoder initialized successfully");
    return ESP_OK;
}

/**
 * @brief Lock shared decoder for exclusive use during decode operation
 */
static bool lock_jpeg_decoder(uint32_t timeout_ms)
{
    if (s_decoder_mutex == NULL) {
        return false;
    }
    return xSemaphoreTake(s_decoder_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

/**
 * @brief Unlock shared decoder
 */
static void unlock_jpeg_decoder(void)
{
    if (s_decoder_mutex != NULL) {
        xSemaphoreGive(s_decoder_mutex);
    }
}

/**
 * @brief Check if file is a JPEG
 */
static bool is_jpeg_file(const char *filename)
{
    if (filename == NULL) return false;
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) return false;
    ext++;
    return (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0);
}

/**
 * @brief Check if file is an MJPEG video
 */
static bool is_mjpeg_file(const char *filename)
{
    if (filename == NULL) return false;
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) return false;
    ext++;
    return (strcasecmp(ext, "mjpg") == 0 || strcasecmp(ext, "mjpeg") == 0);
}

/**
 * @brief Decode JPEG file using hardware decoder
 *
 * @param[in] file_path Path to JPEG file (POSIX path, not LVGL path)
 * @param[out] image_dsc LVGL image descriptor (caller must free data with sdcard_free_background)
 * @return ESP_OK on success
 */
static esp_err_t hw_jpeg_decode_file(const char *file_path, lv_image_dsc_t *image_dsc, uint8_t **out_data)
{
    if (file_path == NULL || image_dsc == NULL || out_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *out_data = NULL;
    memset(image_dsc, 0, sizeof(*image_dsc));

    if (s_shared_jpeg_decoder == NULL) {
        ESP_LOGE(TAG, "JPEG decoder not initialized - call ui_bg_init_jpeg_decoder() first");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;

    // Open file
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        return ESP_ERR_NOT_FOUND;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0 || file_size > MAX_JPEG_FILE_SIZE) {
        ESP_LOGE(TAG, "Invalid file size: %ld (max %d)", file_size, MAX_JPEG_FILE_SIZE);
        fclose(file);
        return ESP_ERR_INVALID_SIZE;
    }

    // Allocate DMA-capable input buffer
    jpeg_decode_memory_alloc_cfg_t tx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER,
    };
    size_t jpeg_buffer_size = 0;
    uint8_t *jpeg_buffer = (uint8_t *)jpeg_alloc_decoder_mem(file_size, &tx_mem_cfg, &jpeg_buffer_size);
    if (jpeg_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate JPEG input buffer");
        fclose(file);
        return ESP_ERR_NO_MEM;
    }

    // Read file into buffer
    size_t bytes_read = fread(jpeg_buffer, 1, file_size, file);
    fclose(file);

    if (bytes_read != (size_t)file_size) {
        ESP_LOGE(TAG, "Failed to read file");
        free(jpeg_buffer);
        return ESP_FAIL;
    }

    // Get JPEG info
    jpeg_decode_picture_info_t info;
    ret = jpeg_decoder_get_info(jpeg_buffer, file_size, &info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get JPEG info: %s", esp_err_to_name(ret));
        free(jpeg_buffer);
        return ret;
    }

    ESP_LOGD(TAG, "JPEG info: %lux%lu", (unsigned long)info.width, (unsigned long)info.height);

    // Hardware JPEG decoder aligns output to 16-byte boundaries
    uint32_t aligned_width = (info.width + 15) & ~15;
    uint32_t aligned_height = (info.height + 15) & ~15;

    // Allocate DMA-capable output buffer (RGB565: 2 bytes per pixel)
    size_t rgb_size = aligned_width * aligned_height * 2;
    jpeg_decode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER,
    };
    size_t rgb_buffer_size = 0;
    uint8_t *rgb_buffer = (uint8_t *)jpeg_alloc_decoder_mem(rgb_size, &rx_mem_cfg, &rgb_buffer_size);
    if (rgb_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RGB output buffer");
        free(jpeg_buffer);
        return ESP_ERR_NO_MEM;
    }

    // Decode JPEG to RGB565 (matches display format, no software conversion needed)
    jpeg_decode_cfg_t decode_cfg = {
        .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
        .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,  // Display expects BGR order
    };

    // Lock shared decoder for exclusive access
    if (!lock_jpeg_decoder(1000)) {
        ESP_LOGE(TAG, "Failed to lock shared decoder");
        free(rgb_buffer);
        free(jpeg_buffer);
        return ESP_ERR_TIMEOUT;
    }

    uint32_t out_size = 0;
    ret = jpeg_decoder_process(
        s_shared_jpeg_decoder,
        &decode_cfg,
        jpeg_buffer,
        file_size,
        rgb_buffer,
        rgb_buffer_size,
        &out_size
    );

    unlock_jpeg_decoder();

    // Free input buffer
    free(jpeg_buffer);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "JPEG decode failed: %s", esp_err_to_name(ret));
        free(rgb_buffer);
        return ret;
    }

    // Set up LVGL image descriptor
    image_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    image_dsc->header.w = info.width;
    image_dsc->header.h = info.height;
    image_dsc->header.stride = info.width * 2;  // RGB565 = 2 bytes per pixel
    image_dsc->data_size = out_size;
    image_dsc->data = rgb_buffer;

    *out_data = rgb_buffer;

    ESP_LOGI(TAG, "JPEG decoded (HW): %s (%ux%u, %lu bytes)",
             file_path, info.width, info.height, (unsigned long)out_size);

    return ESP_OK;
}

// Cache current background paths (POSIX paths)
static char s_touchpad_bg_path[128] = {0};
static char s_clock_bg_path[128] = {0};

// Decoded image data for hardware-decoded JPEGs
static uint8_t *s_touchpad_image_data = NULL;
static lv_image_dsc_t s_touchpad_image_dsc = {0};
static uint8_t *s_clock_image_data = NULL;
static lv_image_dsc_t s_clock_image_dsc = {0};

/**
 * @brief Convert POSIX path to LVGL path with drive letter prefix
 * @param posix_path Path like "/sdcard/background/image.jpg"
 * @param lvgl_path Output buffer for path like "S:/sdcard/background/image.jpg"
 * @param lvgl_path_size Size of output buffer
 */
static void posix_to_lvgl_path(const char *posix_path, char *lvgl_path, size_t lvgl_path_size)
{
    if (!posix_path || !lvgl_path || lvgl_path_size < 4) {
        if (lvgl_path && lvgl_path_size > 0) lvgl_path[0] = '\0';
        return;
    }
    snprintf(lvgl_path, lvgl_path_size, "%s%s", LVGL_FS_PREFIX, posix_path);
}

/**
 * @brief Free decoded image data
 */
static void free_touchpad_image(void)
{
    if (s_touchpad_image_data != NULL) {
        free(s_touchpad_image_data);
        s_touchpad_image_data = NULL;
    }
    memset(&s_touchpad_image_dsc, 0, sizeof(s_touchpad_image_dsc));
}

static void free_clock_image(void)
{
    if (s_clock_image_data != NULL) {
        free(s_clock_image_data);
        s_clock_image_data = NULL;
    }
    memset(&s_clock_image_dsc, 0, sizeof(s_clock_image_dsc));
}

// Track which screen objects we've applied backgrounds to
static lv_obj_t *s_last_touchpad1 = NULL;
static lv_obj_t *s_last_touchpad2 = NULL;
static lv_obj_t *s_last_touchpad3 = NULL;
static lv_obj_t *s_last_clock_panel = NULL;      // ui_Panel41 (background)
static lv_obj_t *s_last_clock_container = NULL;  // ui_AnalogClockContainer (overlay)

// MJPEG player handles
static mjpeg_player_handle_t s_touchpad_mjpeg_player = NULL;
static mjpeg_player_handle_t s_clock_mjpeg_player = NULL;

// LVGL image widgets for MJPEG playback
static lv_obj_t *s_touchpad_mjpeg_image = NULL;
static lv_obj_t *s_clock_mjpeg_image = NULL;

/**
 * @brief Stop and destroy MJPEG player for touchpad
 */
static void stop_touchpad_mjpeg(void)
{
    if (s_touchpad_mjpeg_player != NULL) {
        mjpeg_player_stop(s_touchpad_mjpeg_player);
        mjpeg_player_destroy(s_touchpad_mjpeg_player);
        s_touchpad_mjpeg_player = NULL;
    }
    if (s_touchpad_mjpeg_image != NULL) {
        if (bsp_display_lock(100)) {
            lv_obj_delete(s_touchpad_mjpeg_image);
            bsp_display_unlock();
        }
        s_touchpad_mjpeg_image = NULL;
    }
}

/**
 * @brief Stop and destroy MJPEG player for clock
 */
static void stop_clock_mjpeg(void)
{
    if (s_clock_mjpeg_player != NULL) {
        mjpeg_player_stop(s_clock_mjpeg_player);
        mjpeg_player_destroy(s_clock_mjpeg_player);
        s_clock_mjpeg_player = NULL;
    }
    if (s_clock_mjpeg_image != NULL) {
        if (bsp_display_lock(100)) {
            lv_obj_delete(s_clock_mjpeg_image);
            bsp_display_unlock();
        }
        s_clock_mjpeg_image = NULL;
    }
}

// External UI objects (from SquareLine generated code)
extern lv_obj_t *ui_TouchAndScrollPanel;    // JPKeyboardScreen
extern lv_obj_t *ui_TouchAndScrollPanel1;   // AtoZKeyboardScreen
extern lv_obj_t *ui_TouchAndScrollPanel2;   // CursorScreen
extern lv_obj_t *ui_Panel41;                // AnalogClock full-screen panel (for background)
extern lv_obj_t *ui_AnalogClockContainer;   // Analog clock face (overlay)

// Default clock background image (embedded asset)
LV_IMG_DECLARE(ui_img_717184261);

/**
 * @brief Apply background image to a single panel using image descriptor
 */
static void apply_bg_to_panel_with_dsc(lv_obj_t *panel, const lv_image_dsc_t *dsc)
{
    if (!panel) return;

    if (dsc && dsc->data) {
        lv_obj_set_style_bg_image_src(panel, dsc, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_image_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_bg_image_src(panel, NULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

/**
 * @brief Apply background image to a single panel using LVGL file path
 */
static void apply_bg_to_panel_with_path(lv_obj_t *panel, const char *lvgl_path)
{
    if (!panel) return;

    if (lvgl_path && lvgl_path[0] != '\0') {
        lv_obj_set_style_bg_image_src(panel, lvgl_path, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_image_opa(panel, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_bg_image_src(panel, NULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

esp_err_t ui_bg_apply_to_touchpad(const char *path)
{
    // Stop any existing MJPEG player
    stop_touchpad_mjpeg();

    // Free previous image data
    free_touchpad_image();

    // Cache the POSIX path
    if (path && path[0] != '\0') {
        strncpy(s_touchpad_bg_path, path, sizeof(s_touchpad_bg_path) - 1);
        s_touchpad_bg_path[sizeof(s_touchpad_bg_path) - 1] = '\0';
    } else {
        s_touchpad_bg_path[0] = '\0';
    }

    // Decode image if path provided
    bool use_hw_jpeg = false;
    bool use_mjpeg = false;
    static char lvgl_path[140] = {0};  // Extra space for "S:" prefix
    lvgl_path[0] = '\0';

    if (s_touchpad_bg_path[0] != '\0') {
        if (is_mjpeg_file(s_touchpad_bg_path)) {
            // Use MJPEG player for video backgrounds
            use_mjpeg = true;
            ESP_LOGI(TAG, "Touchpad: using MJPEG player");
        } else if (is_jpeg_file(s_touchpad_bg_path)) {
            // Use hardware JPEG decoder
            esp_err_t ret = hw_jpeg_decode_file(s_touchpad_bg_path, &s_touchpad_image_dsc, &s_touchpad_image_data);
            if (ret == ESP_OK) {
                use_hw_jpeg = true;
                ESP_LOGI(TAG, "Touchpad: using HW JPEG decoder");
            } else {
                ESP_LOGW(TAG, "HW JPEG decode failed, falling back to LVGL loader");
                posix_to_lvgl_path(s_touchpad_bg_path, lvgl_path, sizeof(lvgl_path));
            }
        } else {
            // Use LVGL file loader for PNG/BMP
            posix_to_lvgl_path(s_touchpad_bg_path, lvgl_path, sizeof(lvgl_path));
            ESP_LOGI(TAG, "Touchpad: using LVGL loader for %s", lvgl_path);
        }
    }

    // Apply to touchpad panel (use first available panel for MJPEG)
    if (bsp_display_lock(100)) {
        if (use_mjpeg) {
            // Create MJPEG player for the first touchpad panel
            lv_obj_t *target_panel = ui_TouchAndScrollPanel;
            if (target_panel == NULL) target_panel = ui_TouchAndScrollPanel1;
            if (target_panel == NULL) target_panel = ui_TouchAndScrollPanel2;

            if (target_panel != NULL) {
                // Clear any existing background
                apply_bg_to_panel_with_dsc(target_panel, NULL);

                // Create lv_image widget for MJPEG playback
                s_touchpad_mjpeg_image = lv_image_create(target_panel);
                if (s_touchpad_mjpeg_image != NULL) {
                    lv_obj_set_size(s_touchpad_mjpeg_image, LV_PCT(100), LV_PCT(100));
                    lv_obj_center(s_touchpad_mjpeg_image);
                    lv_image_set_inner_align(s_touchpad_mjpeg_image, LV_IMAGE_ALIGN_STRETCH);

                    bsp_display_unlock();

                    // Create MJPEG player (this may take time for indexing)
                    mjpeg_player_config_t cfg = {
                        .file_path = s_touchpad_bg_path,
                        .target_image = s_touchpad_mjpeg_image,
                        .target_width = 480,
                        .target_height = 370,
                        .fps = 24,
                        .loop = true,
                        .cancel_check = NULL,
                        .cancel_user_data = NULL,
                    };
                    esp_err_t ret = mjpeg_player_create(&cfg, &s_touchpad_mjpeg_player);
                    if (ret == ESP_OK) {
                        mjpeg_player_start(s_touchpad_mjpeg_player);
                        ESP_LOGI(TAG, "Touchpad MJPEG player started");
                    } else {
                        ESP_LOGE(TAG, "Failed to create MJPEG player: %s", esp_err_to_name(ret));
                        // Clean up on failure
                        if (bsp_display_lock(100)) {
                            lv_obj_delete(s_touchpad_mjpeg_image);
                            s_touchpad_mjpeg_image = NULL;
                            bsp_display_unlock();
                        }
                    }

                    // Re-acquire lock for tracking
                    if (!bsp_display_lock(100)) {
                        goto done;
                    }
                }
            }
        } else if (use_hw_jpeg) {
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel, &s_touchpad_image_dsc);
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel1, &s_touchpad_image_dsc);
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel2, &s_touchpad_image_dsc);
        } else if (lvgl_path[0] != '\0') {
            apply_bg_to_panel_with_path(ui_TouchAndScrollPanel, lvgl_path);
            apply_bg_to_panel_with_path(ui_TouchAndScrollPanel1, lvgl_path);
            apply_bg_to_panel_with_path(ui_TouchAndScrollPanel2, lvgl_path);
        } else {
            // Clear backgrounds
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel, NULL);
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel1, NULL);
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel2, NULL);
        }

        // Track which objects we applied to
        s_last_touchpad1 = ui_TouchAndScrollPanel;
        s_last_touchpad2 = ui_TouchAndScrollPanel1;
        s_last_touchpad3 = ui_TouchAndScrollPanel2;

        bsp_display_unlock();
    }

done:
    ESP_LOGI(TAG, "Touchpad background: %s", s_touchpad_bg_path[0] ? s_touchpad_bg_path : "(none)");
    return ESP_OK;
}

esp_err_t ui_bg_apply_to_clock(const char *path)
{
    // Stop any existing MJPEG player
    stop_clock_mjpeg();

    // Free previous image data
    free_clock_image();

    // Cache the POSIX path
    if (path && path[0] != '\0') {
        strncpy(s_clock_bg_path, path, sizeof(s_clock_bg_path) - 1);
        s_clock_bg_path[sizeof(s_clock_bg_path) - 1] = '\0';
    } else {
        s_clock_bg_path[0] = '\0';
    }

    // Decode image if path provided
    bool use_hw_jpeg = false;
    bool use_mjpeg = false;
    static char lvgl_path[140] = {0};  // Extra space for "S:" prefix
    lvgl_path[0] = '\0';

    if (s_clock_bg_path[0] != '\0') {
        if (is_mjpeg_file(s_clock_bg_path)) {
            // Use MJPEG player for video backgrounds
            use_mjpeg = true;
            ESP_LOGI(TAG, "Clock: using MJPEG player");
        } else if (is_jpeg_file(s_clock_bg_path)) {
            // Use hardware JPEG decoder
            esp_err_t ret = hw_jpeg_decode_file(s_clock_bg_path, &s_clock_image_dsc, &s_clock_image_data);
            if (ret == ESP_OK) {
                use_hw_jpeg = true;
                ESP_LOGI(TAG, "Clock: using HW JPEG decoder");
            } else {
                ESP_LOGW(TAG, "HW JPEG decode failed, falling back to LVGL loader");
                posix_to_lvgl_path(s_clock_bg_path, lvgl_path, sizeof(lvgl_path));
            }
        } else {
            // Use LVGL file loader for PNG/BMP
            posix_to_lvgl_path(s_clock_bg_path, lvgl_path, sizeof(lvgl_path));
            ESP_LOGI(TAG, "Clock: using LVGL loader for %s", lvgl_path);
        }
    }

    if (bsp_display_lock(100)) {
        // Apply background to ui_Panel41 (full-screen panel behind clock)
        if (ui_Panel41) {
            if (use_mjpeg) {
                // Clear any existing background
                lv_obj_set_style_bg_image_src(ui_Panel41, NULL,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);

                // Create lv_image widget for MJPEG playback
                s_clock_mjpeg_image = lv_image_create(ui_Panel41);
                if (s_clock_mjpeg_image != NULL) {
                    lv_obj_set_size(s_clock_mjpeg_image, LV_PCT(100), LV_PCT(100));
                    lv_obj_center(s_clock_mjpeg_image);
                    lv_image_set_inner_align(s_clock_mjpeg_image, LV_IMAGE_ALIGN_STRETCH);
                    // Send to back so clock face is on top
                    lv_obj_move_to_index(s_clock_mjpeg_image, 0);

                    bsp_display_unlock();

                    // Create MJPEG player (this may take time for indexing)
                    mjpeg_player_config_t cfg = {
                        .file_path = s_clock_bg_path,
                        .target_image = s_clock_mjpeg_image,
                        .target_width = 480,
                        .target_height = 800,
                        .fps = 24,
                        .loop = true,
                        .cancel_check = NULL,
                        .cancel_user_data = NULL,
                    };
                    esp_err_t ret = mjpeg_player_create(&cfg, &s_clock_mjpeg_player);
                    if (ret == ESP_OK) {
                        mjpeg_player_start(s_clock_mjpeg_player);
                        ESP_LOGI(TAG, "Clock MJPEG player started");
                    } else {
                        ESP_LOGE(TAG, "Failed to create MJPEG player: %s", esp_err_to_name(ret));
                        // Clean up on failure
                        if (bsp_display_lock(100)) {
                            lv_obj_delete(s_clock_mjpeg_image);
                            s_clock_mjpeg_image = NULL;
                            bsp_display_unlock();
                        }
                    }

                    // Re-acquire lock for rest of function
                    if (!bsp_display_lock(100)) {
                        goto clock_done;
                    }
                }
            } else if (use_hw_jpeg) {
                lv_obj_set_style_bg_image_src(ui_Panel41, &s_clock_image_dsc,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_image_opa(ui_Panel41, LV_OPA_COVER,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);
            } else if (lvgl_path[0] != '\0') {
                lv_obj_set_style_bg_image_src(ui_Panel41, lvgl_path,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_image_opa(ui_Panel41, LV_OPA_COVER,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                // Clear background
                lv_obj_set_style_bg_image_src(ui_Panel41, NULL,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            s_last_clock_panel = ui_Panel41;
        }

        // Keep clock face fully opaque (not semi-transparent)
        if (ui_AnalogClockContainer) {
            // Clock face image stays opaque regardless of background
            lv_obj_set_style_bg_image_src(ui_AnalogClockContainer, &ui_img_717184261,
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_image_opa(ui_AnalogClockContainer, LV_OPA_COVER,
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            s_last_clock_container = ui_AnalogClockContainer;
        }
        bsp_display_unlock();
    }

clock_done:
    ESP_LOGI(TAG, "Clock background: %s", s_clock_bg_path[0] ? s_clock_bg_path : "(default)");
    return ESP_OK;
}

esp_err_t ui_bg_clear_touchpad(void)
{
    return ui_bg_apply_to_touchpad(NULL);
}

esp_err_t ui_bg_clear_clock(void)
{
    return ui_bg_apply_to_clock(NULL);
}

void ui_bg_load_saved_settings(void)
{
    char path[128];

    ESP_LOGI(TAG, "Loading saved background settings...");

    // Load touchpad background
    if (sdcard_get_touchpad_bg(path, sizeof(path)) == ESP_OK && path[0] != '\0') {
        ESP_LOGI(TAG, "Restoring touchpad background: %s", path);
        ui_bg_apply_to_touchpad(path);
    }

    // Load clock background
    if (sdcard_get_clock_bg(path, sizeof(path)) == ESP_OK && path[0] != '\0') {
        ESP_LOGI(TAG, "Restoring clock background: %s", path);
        ui_bg_apply_to_clock(path);
    }
}

void ui_bg_refresh_if_needed(void)
{
    // Check if screen objects have been recreated (different pointers)
    // If so, reapply the cached backgrounds

    bool need_touchpad_refresh = false;
    bool need_clock_refresh = false;

    // Check touchpads
    if (ui_TouchAndScrollPanel && ui_TouchAndScrollPanel != s_last_touchpad1) {
        need_touchpad_refresh = true;
    }
    if (ui_TouchAndScrollPanel1 && ui_TouchAndScrollPanel1 != s_last_touchpad2) {
        need_touchpad_refresh = true;
    }
    if (ui_TouchAndScrollPanel2 && ui_TouchAndScrollPanel2 != s_last_touchpad3) {
        need_touchpad_refresh = true;
    }

    // Check clock (both panel and container)
    if (ui_Panel41 && ui_Panel41 != s_last_clock_panel) {
        need_clock_refresh = true;
    }
    if (ui_AnalogClockContainer && ui_AnalogClockContainer != s_last_clock_container) {
        need_clock_refresh = true;
    }

    // Reapply touchpad if needed (no lock needed - called from LVGL timer context)
    if (need_touchpad_refresh && s_touchpad_bg_path[0] != '\0') {
        // Use decoded image if available, otherwise use LVGL path
        if (s_touchpad_image_data != NULL) {
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel, &s_touchpad_image_dsc);
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel1, &s_touchpad_image_dsc);
            apply_bg_to_panel_with_dsc(ui_TouchAndScrollPanel2, &s_touchpad_image_dsc);
        } else {
            char lvgl_path[140];
            posix_to_lvgl_path(s_touchpad_bg_path, lvgl_path, sizeof(lvgl_path));
            apply_bg_to_panel_with_path(ui_TouchAndScrollPanel, lvgl_path);
            apply_bg_to_panel_with_path(ui_TouchAndScrollPanel1, lvgl_path);
            apply_bg_to_panel_with_path(ui_TouchAndScrollPanel2, lvgl_path);
        }
        s_last_touchpad1 = ui_TouchAndScrollPanel;
        s_last_touchpad2 = ui_TouchAndScrollPanel1;
        s_last_touchpad3 = ui_TouchAndScrollPanel2;
        ESP_LOGD(TAG, "Refreshed touchpad backgrounds");
    }

    // Reapply clock if needed
    if (need_clock_refresh && s_clock_bg_path[0] != '\0') {
        // Apply background to Panel41
        if (ui_Panel41) {
            if (s_clock_image_data != NULL) {
                lv_obj_set_style_bg_image_src(ui_Panel41, &s_clock_image_dsc,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                char lvgl_path[140];
                posix_to_lvgl_path(s_clock_bg_path, lvgl_path, sizeof(lvgl_path));
                lv_obj_set_style_bg_image_src(ui_Panel41, lvgl_path,
                                              LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            lv_obj_set_style_bg_image_opa(ui_Panel41, LV_OPA_COVER,
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            s_last_clock_panel = ui_Panel41;
        }
        // Keep clock face fully opaque
        if (ui_AnalogClockContainer) {
            lv_obj_set_style_bg_image_src(ui_AnalogClockContainer, &ui_img_717184261,
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_image_opa(ui_AnalogClockContainer, LV_OPA_COVER,
                                          LV_PART_MAIN | LV_STATE_DEFAULT);
            s_last_clock_container = ui_AnalogClockContainer;
        }
        ESP_LOGD(TAG, "Refreshed clock background");
    }
}
