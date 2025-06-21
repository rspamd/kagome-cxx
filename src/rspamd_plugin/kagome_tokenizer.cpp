#include "kagome/c_api/kagome_c_api.h"

/* Forward declaration for UCL object */
typedef struct ucl_object_s ucl_object_t;

/* Rspamd custom tokenizer API */
#define RSPAMD_CUSTOM_TOKENIZER_API_VERSION 1

typedef rspamd_words_t rspamd_tokenizer_result_t;

typedef struct rspamd_custom_tokenizer_api {
    unsigned int api_version;
    const char *name;
    
    int (*init)(const ucl_object_t *config, char *error_buf, size_t error_buf_size);
    void (*deinit)(void);
    double (*detect_language)(const char *text, size_t len);
    int (*tokenize)(const char *text, size_t len, rspamd_tokenizer_result_t *result);
    void (*cleanup_result)(rspamd_tokenizer_result_t *result);
    const char *(*get_language_hint)(void);
    double (*get_min_confidence)(void);
} rspamd_custom_tokenizer_api_t;

/* Implementation of the tokenizer API */
static int kagome_rspamd_init(const ucl_object_t *config, char *error_buf, size_t error_buf_size) {
    return kagome_init(config, error_buf, error_buf_size);
}

static void kagome_rspamd_deinit(void) {
    kagome_deinit();
}

static double kagome_rspamd_detect_language(const char *text, size_t len) {
    return kagome_detect_language(text, len);
}

static int kagome_rspamd_tokenize(const char *text, size_t len, rspamd_tokenizer_result_t *result) {
    return kagome_tokenize(text, len, result);
}

static void kagome_rspamd_cleanup_result(rspamd_tokenizer_result_t *result) {
    kagome_cleanup_result(result);
}

static const char *kagome_rspamd_get_language_hint(void) {
    return kagome_get_language_hint();
}

static double kagome_rspamd_get_min_confidence(void) {
    return kagome_get_min_confidence();
}

/* API structure */
static const rspamd_custom_tokenizer_api_t kagome_api = {
    .api_version = RSPAMD_CUSTOM_TOKENIZER_API_VERSION,
    .name = "japanese_kagome",
    .init = kagome_rspamd_init,
    .deinit = kagome_rspamd_deinit,
    .detect_language = kagome_rspamd_detect_language,
    .tokenize = kagome_rspamd_tokenize,
    .cleanup_result = kagome_rspamd_cleanup_result,
    .get_language_hint = kagome_rspamd_get_language_hint,
    .get_min_confidence = kagome_rspamd_get_min_confidence
};

/* Entry point function that Rspamd will call */
extern "C" {

const rspamd_custom_tokenizer_api_t *rspamd_tokenizer_get_api(void) {
    return &kagome_api;
}

} // extern "C" 