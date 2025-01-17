// WARNING: This file is autogenerated by cbindgen, don't modify it manually.
// clang-format off

#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>
#include <QtCore>
#define NONNULL

namespace librepcb {
namespace rs {

/**
 * Wrapper type for [Archive]
 */
struct ZipArchive;

/**
 * Wrapper type for [Writer]
 */
struct ZipWriter;

extern "C" {

extern size_t ffi_qbytearray_len(const QByteArray * NONNULL obj);

extern const uint8_t *ffi_qbytearray_data(const QByteArray * NONNULL obj);

extern uint8_t *ffi_qbytearray_data_mut(QByteArray * NONNULL obj);

extern void ffi_qbytearray_resize(QByteArray * NONNULL obj,
                                  size_t len,
                                  uint8_t value);

extern size_t ffi_qstring_len(const QString * NONNULL obj);

extern const uint16_t *ffi_qstring_utf16(const QString * NONNULL obj);

extern void ffi_qstring_set(QString * NONNULL obj, const char *s, size_t len);

/**
 * Wrapper for [increment_number_in_string]
 */
void ffi_increment_number_in_string(QString * NONNULL s);

/**
 * Create a new [ZipArchive] object from file path
 */
ZipArchive *ffi_ziparchive_new_from_file(const QString * NONNULL path,
                                         QString * NONNULL err);

/**
 * Create a new [ZipArchive] object from memory
 */
ZipArchive *ffi_ziparchive_new_from_mem(const QByteArray * NONNULL data,
                                        QString * NONNULL err);

/**
 * Delete [ZipArchive] object
 */
void ffi_ziparchive_delete(ZipArchive *obj);

/**
 * Get number of files in [ZipArchive]
 */
size_t ffi_ziparchive_len(const ZipArchive * NONNULL obj);

/**
 * Get name of a file in [ZipArchive]
 */
bool ffi_ziparchive_name_for_index(ZipArchive * NONNULL obj,
                                   size_t index,
                                   QString * NONNULL name,
                                   QString * NONNULL err);

/**
 * Read a file from [ZipArchive]
 */
bool ffi_ziparchive_read_by_index(ZipArchive * NONNULL obj,
                                  size_t index,
                                  QByteArray * NONNULL buf,
                                  QString * NONNULL err);

/**
 * Extract [ZipArchive] to directory
 */
bool ffi_ziparchive_extract(ZipArchive * NONNULL obj,
                            const QString * NONNULL dir);

/**
 * Create a new [ZipWriter] object writing to a file
 */
ZipWriter *ffi_zipwriter_new_to_file(const QString * NONNULL path,
                                     QString * NONNULL err);

/**
 * Create a new [ZipWriter] object writing to memory
 */
ZipWriter *ffi_zipwriter_new_to_mem(QByteArray * NONNULL data);

/**
 * Delete [ZipWriter] object
 */
void ffi_zipwriter_delete(ZipWriter *obj);

/**
 * Write a file to [ZipWriter]
 */
bool ffi_zipwriter_write_file(ZipWriter * NONNULL obj,
                              const QString * NONNULL name,
                              const QByteArray * NONNULL data,
                              uint32_t mode,
                              QString * NONNULL err);

/**
 * Finish writing to [ZipWriter]
 */
bool ffi_zipwriter_finish(ZipWriter * NONNULL obj, QString * NONNULL err);

}  // extern "C"

}  // namespace rs
}  // namespace librepcb

// clang-format on
