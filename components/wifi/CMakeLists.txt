idf_component_register(
	SRCS
	"wifi.c"
	INCLUDE_DIRS
	"."
	REQUIRES
	"esp_wifi"
	"nvs_flash"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
