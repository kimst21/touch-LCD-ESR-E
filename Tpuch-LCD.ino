#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// 터치스크린 핀 설정
#define XPT2046_IRQ 18   // 터치 인터럽트 핀
#define XPT2046_MOSI 6   // SPI MOSI 핀
#define XPT2046_MISO 1   // SPI MISO 핀
#define XPT2046_CLK 36   // SPI 클럭 핀
#define XPT2046_CS 4     // SPI 칩 선택 핀

// SPI 통신 초기화
SPIClass touchscreenSPI = SPIClass(VSPI);
// XPT2046 터치스크린 객체 생성
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240    // 화면 가로 크기
#define SCREEN_HEIGHT 320   // 화면 세로 크기

// 터치 입력 데이터 변수
int x, y, z;  // x, y 좌표 및 z 압력 값

// LVGL 그리기 버퍼 설정
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// LVGL 디버그 로깅 콜백 함수
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);  // 로그 출력
  Serial.flush();
}

// 터치 입력 읽기 콜백 함수
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    // 터치 포인트 데이터 읽기
    TS_Point p = touchscreen.getPoint();
    // 터치 좌표 보정
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;  // 압력 값

    // 터치 상태 업데이트
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    // 터치 상태 업데이트 (터치되지 않음)
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// 버튼 클릭 횟수 변수
int btn1_count = 0;

// 일반 버튼 클릭 이벤트 처리 함수
static void event_handler_btn1(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    btn1_count++;  // 클릭 횟수 증가
    LV_LOG_USER("Button clicked %d", (int)btn1_count);
  }
}

// 토글 버튼 상태 변경 이벤트 처리 함수
static void event_handler_btn2(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = (lv_obj_t*) lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    // 상태 출력 (ON/OFF)
    LV_LOG_USER("Toggled %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "on" : "off");
  }
}

// 슬라이더 값 표시를 위한 레이블 객체
static lv_obj_t * slider_label;

// 슬라이더 이벤트 처리 함수
static void slider_event_callback(lv_event_t * e) {
  lv_obj_t * slider = (lv_obj_t*) lv_event_get_target(e);
  char buf[8];
  lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));  // 슬라이더 값 포맷
  lv_label_set_text(slider_label, buf);  // 레이블 업데이트
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
  LV_LOG_USER("Slider changed to %d%%", (int)lv_slider_get_value(slider));  // 디버그 출력
}

// GUI 생성 함수
void lv_create_main_gui(void) {
  // "Hello, All-In-One ESR Board" 텍스트 레이블 생성
  lv_obj_t * text_label = lv_label_create(lv_scr_act());
  lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);  // 텍스트 줄바꿈
  lv_label_set_text(text_label, "Hello, All-In-One ESR Board");
  lv_obj_set_width(text_label, 150);  // 텍스트 너비 설정
  lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label, LV_ALIGN_CENTER, 0, -90);  // 중앙 위쪽 정렬

  lv_obj_t * btn_label;

  // 일반 버튼 생성
  lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
  lv_obj_add_event_cb(btn1, event_handler_btn1, LV_EVENT_ALL, NULL);  // 이벤트 콜백 연결
  lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -50);
  lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

  btn_label = lv_label_create(btn1);
  lv_label_set_text(btn_label, "Button");
  lv_obj_center(btn_label);

  // 토글 버튼 생성
  lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
  lv_obj_add_event_cb(btn2, event_handler_btn2, LV_EVENT_ALL, NULL);
  lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);

  btn_label = lv_label_create(btn2);
  lv_label_set_text(btn_label, "Toggle");
  lv_obj_center(btn_label);

  // 슬라이더 생성
  lv_obj_t * slider = lv_slider_create(lv_scr_act());
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 60);
  lv_obj_add_event_cb(slider, slider_event_callback, LV_EVENT_VALUE_CHANGED, NULL);
  lv_slider_set_range(slider, 0, 100);  // 슬라이더 값 범위 설정
  lv_obj_set_style_anim_time(slider, 2000, 0);

  // 슬라이더 레이블 생성
  slider_label = lv_label_create(lv_scr_act());
  lv_label_set_text(slider_label, "0%");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

// 초기화 함수
void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);  // 시리얼 통신 시작
  Serial.println(LVGL_Arduino);

  lv_init();  // LVGL 초기화
  lv_log_register_print_cb(log_print);  // 디버깅 로깅 설정

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(2);  // 터치스크린 회전 설정

  lv_disp_t * disp;
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));  // 디스플레이 초기화
  lv_disp_set_rotation(disp, LV_DISP_ROTATION_270);

  lv_indev_t * indev = lv_indev_create();  // 입력 장치 생성
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);  // 입력 장치 유형 설정
  lv_indev_set_read_cb(indev, touchscreen_read);  // 터치 입력 콜백 설정

  lv_create_main_gui();  // GUI 생성
}

// 메인 루프
void loop() {
  lv_task_handler();  // LVGL 작업 처리
  lv_tick_inc(5);  // 시간 갱신
  delay(5);  // 실행 간격
}
