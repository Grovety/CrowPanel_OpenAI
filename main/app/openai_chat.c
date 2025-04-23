#include "openai.h"

#include "esp_http_client.h"
#include "esp_log.h"
#include "https_client.h"

#define TAG "OPENAI_CHAT"

static void process_response(http_resp_t *resp, void *ctx);

typedef void (*sentence_callback_t)(const char *sentence_start,
                                    size_t sentence_length, void *user_data);

static int is_sentence_end(char c) {
    return (c == '.' || c == '!' || c == '?' || c == '\n');
}

static const char *skip_leading_whitespace(const char *str) {
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

static void split_by_senteces(const char *text, sentence_callback_t callback,
                              void *user_data) {
    if (!text || !callback) {
        return; // Nothing to do or nowhere to send results
    }

    const char *start = text;
    const char *ptr = start;
    int in_quote = 0;
    int sentence_found = 0;

    while (*ptr) {
        sentence_found = 0;
        const char *sentence_end =
            NULL; // Pointer to *after* the last char of the sentence segment

        if (*ptr == '"') {
            in_quote = !in_quote;
            // Check if punctuation occurred right before this closing quote
            if (!in_quote && ptr > start && is_sentence_end(*(ptr - 1))) {
                // Sentence ends *including* the quote
                sentence_end = ptr + 1;
                sentence_found = 1;
            }
        } else if (!in_quote && is_sentence_end(*ptr)) {
            // Sentence ends *including* the punctuation
            sentence_end = ptr + 1;
            sentence_found = 1;
        }

        // If a sentence boundary is found, process it
        if (sentence_found && sentence_end) {
            const char *next_actual_sentence_start =
                skip_leading_whitespace(sentence_end);

            // Calculate length
            size_t len = next_actual_sentence_start - start;

            // Call the callback with the raw segment
            if (len > 0) {
                callback(start, len, user_data);
            }

            // Update start for the next potential sentence
            start = next_actual_sentence_start;
            ptr =
                next_actual_sentence_start; // Continue loop from the new start

            // If skipping spaces took us to the end, break the loop
            if (!*ptr) {
                break;
            }
            continue; // Skip the loop's default increment for this iteration
        }

        ptr++; // Move to the next character
    }          // End of while loop

    // Handle any remaining text after the last found sentence boundary
    // No need to skip leading whitespace here, as 'start' should already be
    // correct
    if (*start) {                   // If there's non-whitespace remaining text
        size_t len = strlen(start); // The rest of the string
        if (len > 0) {
            callback(start, len, user_data);
        }
    }
}

static void send_sentence_callback(const char *sentence_start,
                                   size_t sentence_length, void *user_data) {
    openai_ctx_t *openai_ctx = (openai_ctx_t *)user_data;
    ESP_LOGD(TAG, "sentence_length=%u:^%.*s$", sentence_length,
             (int)sentence_length, sentence_start);
    xMessageBufferSend(openai_ctx->messages_buffer, sentence_start,
                       sentence_length, pdMS_TO_TICKS(1000));
}

static void process_response(http_resp_t *resp, void *ctx) {
    ESP_LOG_BUFFER_HEXDUMP(TAG, resp->data, resp->size, ESP_LOG_DEBUG);
    openai_ctx_t *openai_ctx = (openai_ctx_t *)ctx;

    openai_event_t ev = {
        .type = OPENAI_EVENT_RESPONSE_DONE,
        .user_data = NULL,
    };
    xQueueSend(openai_ctx->openai_event_queue, &ev, 0);

    cJSON *root = cJSON_Parse((const char *)resp->data);
    if (!root) {
        ESP_LOGW(TAG, "Unable to parse response");
        return;
    }
    char *text = cJSON_Print(root);
    if (text) {
        ESP_LOGD(__FUNCTION__, "recv:\n%s", text);
        free(text);
    }

    do {
        cJSON *status = cJSON_GetObjectItemCaseSensitive(root, "status");
        if (status && cJSON_IsString(status)) {
            if (strcmp(status->valuestring, "completed") != 0) {
                cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
                ESP_LOGW(TAG, "Status: %s, error: %s", status->valuestring,
                         error->valuestring);
                break;
            }
        }

        cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
        if (error) {
            cJSON *message = cJSON_GetObjectItemCaseSensitive(error, "message");
            if (message && cJSON_IsString(message)) {
                ESP_LOGW(TAG, "Error: %s", message->valuestring);
                xMessageBufferSend(openai_ctx->messages_buffer,
                                   message->valuestring,
                                   strlen(message->valuestring), 0);
                xMessageBufferSend(openai_ctx->messages_buffer, "\n", 1, 0);
                break;
            }
        }

        cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
        if (id && cJSON_IsString(id)) {
            strncpy(openai_ctx->session.previous_response_id, id->valuestring,
                    sizeof(openai_ctx->session.previous_response_id));
        }

        cJSON *output = cJSON_GetObjectItemCaseSensitive(root, "output");
        cJSON *output_item;
        cJSON_ArrayForEach(output_item, output) {
            cJSON *type = cJSON_GetObjectItemCaseSensitive(output_item, "type");
            if (cJSON_IsString(type) &&
                strcmp(type->valuestring, "message") == 0) {
                cJSON *content =
                    cJSON_GetObjectItemCaseSensitive(output_item, "content");
                cJSON *status =
                    cJSON_GetObjectItemCaseSensitive(content, "status");
                if (cJSON_IsString(status) &&
                    strcmp(status->valuestring, "completed") != 0) {
                    ESP_LOGE(TAG, "output status: %s", status->valuestring);
                    return;
                }
                cJSON *content_item;
                cJSON_ArrayForEach(content_item, content) {
                    cJSON *text =
                        cJSON_GetObjectItemCaseSensitive(content_item, "text");
                    if (text && cJSON_IsString(text)) {
                        ESP_LOGI(TAG, "Extracted Message:\n%s",
                                 text->valuestring);
                        split_by_senteces(text->valuestring,
                                          send_sentence_callback, openai_ctx);
                        xMessageBufferSend(openai_ctx->messages_buffer, "\n", 1,
                                           0);
                    }
                }
            } else {
                ESP_LOGW(TAG, "Unknown output type %s", type->valuestring);
            }
        }
    } while (false);
    cJSON_Delete(root);
}

int openai_send_text(openai_ctx_t *openai_ctx, char *text) {
    const int len =
        sizeof(OPENAI_AUTH_PREFIX) + strlen(openai_ctx->session.api_key) + 1;
    char auth[len];
    snprintf(auth, len, OPENAI_AUTH_PREFIX "%s", openai_ctx->session.api_key);
    char *header[] = {
        "Content-Type: application/json",
        auth,
        NULL,
    };
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", OPENAI_CHAT_MODEL);
    cJSON_AddStringToObject(root, "input", text);
    if (strlen(openai_ctx->session.previous_response_id)) {
        cJSON_AddStringToObject(root, "previous_response_id",
                                openai_ctx->session.previous_response_id);
    }
    cJSON_AddStringToObject(
        root, "instructions",
        "Be precise, try to fit the response in three paragraphs");
    char *json_string = cJSON_Print(root);
    if (json_string) {
        ESP_LOGD(TAG, "post to url=%s:\n%s", OPENAI_API_URL, json_string);
        openai_event_t ev = {
            .type = OPENAI_EVENT_REQUEST_SENT,
            .user_data = NULL,
        };
        xQueueSend(openai_ctx->openai_event_queue, &ev, 0);
        int result = https_post(OPENAI_API_URL, header, json_string, NULL,
                                process_response, openai_ctx);
        if (result == ESP_ERR_HTTP_EAGAIN) {
            openai_event_t ev = {
                .type = OPENAI_EVENT_REQUEST_TIMEOUT,
                .user_data = NULL,
            };
            xQueueSend(openai_ctx->openai_event_queue, &ev, 0);
        }
        free(json_string);
    }
    cJSON_Delete(root);
    return 0;
}
