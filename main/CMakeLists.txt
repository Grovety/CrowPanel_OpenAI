set(APP_DIR "${PROJECT_DIR}/main/app")
set(APP_SRC 
    "${APP_DIR}/audio/audio_codec_data_dummy.c"
    "${APP_DIR}/audio/esp_capture_audio_afe_src.c"
    "${APP_DIR}/audio/dummy_codec.c"
    "${APP_DIR}/audio/limiter.c"
    "${APP_DIR}/app.c"
    "${APP_DIR}/board.c"
    "${APP_DIR}/media_sys.c"
    "${APP_DIR}/network.c"
    "${APP_DIR}/nvs.c"
    "${APP_DIR}/openai.c"
    "${APP_DIR}/openai_webrtc.c"
    "${APP_DIR}/openai_signaling.c"
    "${APP_DIR}/openai_chat.c"
)
set(APP_INC "${APP_DIR}/include" "${APP_DIR}/audio/include")

set(UI_DIR "${PROJECT_DIR}/main/ui")
set(UI_SRC 
    "${UI_DIR}/assets/icons.c"
    "${UI_DIR}/assets/noto_sans_18.c"
    "${UI_DIR}/auth_window.c"
    "${UI_DIR}/common.c"
    "${UI_DIR}/popup.c"
    "${UI_DIR}/settings_window.c"
    "${UI_DIR}/status_bar.c"
    "${UI_DIR}/terminal.c"
    "${UI_DIR}/ui.c"
    "${UI_DIR}/wifi_window.c"
)
set(UI_INC "${UI_DIR}/include")

idf_component_register(
    SRCS
    "main.cpp"
    "lvgl_v8_port.cpp"
    "${APP_SRC}"
    "${UI_SRC}"
    INCLUDE_DIRS .
    "${APP_INC}"
    "${UI_INC}"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)

# The following code is to avoid the error:
# lvgl_v8_port/managed_components/lvgl__lvgl/demos/stress/lv_demo_stress.c:92:29: error: format '%d' expects argument of
# type 'int', but argument 6 has type 'uint32_t' {aka 'long unsigned int'} [-Werror=format=]

# Get the exact component name
idf_build_get_property(build_components BUILD_COMPONENTS)
foreach(COMPONENT ${build_components})
    if(COMPONENT MATCHES "lvgl" OR COMPONENT MATCHES "lvgl__lvgl")
        set(TARGET_COMPONENT ${COMPONENT})
        break()
    endif()
endforeach()
# Get the component library
if(TARGET_COMPONENT STREQUAL "")
    message(FATAL_ERROR "Component 'lvgl' not found.")
else()
    idf_component_get_property(LVGL_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
endif()
target_compile_options(${LVGL_LIB} PRIVATE "-Wno-format")
set(TARGET_COMPONENT "")
