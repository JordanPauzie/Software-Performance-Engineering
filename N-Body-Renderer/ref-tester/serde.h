#ifndef REF_TEST_SERDE_H
#define REF_TEST_SERDE_H

#include "../serde.h" // used for error types
#include "./types.h"

extern const uint64_t REF_OUT_HEADER;

/**
 * @brief Deserialize the ref_out_t contained within `file` into `out`.
 * Returns a non-zero value in the case of an error.
 *
 * @param[out] out item to deserialize into
 * @param[in] file file containing exactly one ref_out_t
 * @return non-zero in case of error
 */
deser_error_e deser_ref_out(ref_out_t *out, FILE *file);

/**
 * @brief Serialize `out` and write it to `file`. Returns a non-zero value in
 * the case of an error.
 *
 * @param[out] file a file pointer
 * @param[in] out item to serialize
 * @return non-zero in case of error
 */
ser_error_e ser_ref_out(FILE *file, const ref_out_t *out);

#endif // REF_TEST_SERDE_H