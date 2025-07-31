#ifndef KAGOME_C_API_H
#define KAGOME_C_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Copy the exact types from rspamd_tokenizer_types.h */
typedef struct rspamd_ftok {
	size_t len;
	const char *begin;
} rspamd_ftok_t;

typedef struct rspamd_ftok_unicode {
	size_t len;
	const uint32_t *begin;
} rspamd_ftok_unicode_t;

/* Word flags */
#define RSPAMD_WORD_FLAG_TEXT (1u << 0u)
#define RSPAMD_WORD_FLAG_META (1u << 1u)
#define RSPAMD_WORD_FLAG_LUA_META (1u << 2u)
#define RSPAMD_WORD_FLAG_EXCEPTION (1u << 3u)
#define RSPAMD_WORD_FLAG_HEADER (1u << 4u)
#define RSPAMD_WORD_FLAG_UNIGRAM (1u << 5u)
#define RSPAMD_WORD_FLAG_UTF (1u << 6u)
#define RSPAMD_WORD_FLAG_NORMALISED (1u << 7u)
#define RSPAMD_WORD_FLAG_STEMMED (1u << 8u)
#define RSPAMD_WORD_FLAG_BROKEN_UNICODE (1u << 9u)
#define RSPAMD_WORD_FLAG_STOP_WORD (1u << 10u)
#define RSPAMD_WORD_FLAG_SKIPPED (1u << 11u)
#define RSPAMD_WORD_FLAG_INVISIBLE_SPACES (1u << 12u)
#define RSPAMD_WORD_FLAG_EMOJI (1u << 13u)

typedef struct rspamd_word {
	rspamd_ftok_t original;
	rspamd_ftok_unicode_t unicode;
	rspamd_ftok_t normalized;
	rspamd_ftok_t stemmed;
	unsigned int flags;
} rspamd_word_t;

typedef struct rspamd_words_s {
	size_t n;
	size_t m;
	rspamd_word_t *a;
} rspamd_words_t;

/* Forward declarations */
typedef struct ucl_object_s ucl_object_t;

/* Opaque handle for the tokenizer instance */
typedef struct kagome_tokenizer_handle kagome_tokenizer_handle_t;

/* C API functions */

/**
 * Initialize the kagome tokenizer
 * @param config UCL configuration object (can be NULL)
 * @param error_buf Buffer for error messages
 * @param error_buf_size Size of error buffer
 * @return 0 on success, non-zero on failure
 */
int kagome_init(const ucl_object_t *config, char *error_buf, size_t error_buf_size);

/**
 * Cleanup the kagome tokenizer
 */
void kagome_deinit(void);

/**
 * Detect if text is Japanese
 * @param text UTF-8 text to analyze
 * @param len Length of text in bytes
 * @return Confidence score 0.0-1.0, or -1.0 if cannot handle
 */
double kagome_detect_language(const char *text, size_t len);

/**
 * Tokenize Japanese text
 * @param text UTF-8 text to tokenize
 * @param len Length of text in bytes
 * @param result Output kvec to fill with rspamd_word_t elements
 * @return 0 on success, non-zero on failure
 */
int kagome_tokenize(const char *text, size_t len, rspamd_words_t *result);

/**
 * Cleanup tokenization result
 * @param result Result kvec from kagome_tokenize
 */
void kagome_cleanup_result(rspamd_words_t *result);

/**
 * Get language hint
 * @return Language code "ja" for Japanese
 */
const char *kagome_get_language_hint(void);

/**
 * Get minimum confidence threshold
 * @return Minimum confidence for Japanese detection
 */
double kagome_get_min_confidence(void);

#ifdef __cplusplus
}
#endif

#endif /* KAGOME_C_API_H */