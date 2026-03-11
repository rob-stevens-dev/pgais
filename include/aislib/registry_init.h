#pragma once

namespace aislib {

/**
 * @file registry_init.h
 * @brief Library initialisation — registers all message decoders.
 *
 * Calling register_all_decoders() installs one decoder function per
 * supported message type into the MessageRegistry singleton.  The function
 * is idempotent: calling it more than once replaces each decoder with the
 * same function pointer, which is a safe no-op.
 *
 * Callers must invoke register_all_decoders() exactly once before any call
 * to MessageRegistry::instance().decode().  The typical placement is at
 * application start-up, before any NMEA input is processed.
 *
 * When aislib is embedded as a PostgreSQL extension, register_all_decoders()
 * should be called from the extension's _PG_init() function, which is
 * guaranteed to run in a single-threaded context before any backend uses
 * the extension.
 *
 * Thread safety: register_all_decoders() is not thread-safe.  All calls to
 * it must complete before any concurrent calls to
 * MessageRegistry::instance().decode() are made.  Post-registration,
 * decode() is safe for concurrent use.
 */

/**
 * @brief Registers all supported AIS message type decoders with the
 *        MessageRegistry singleton.
 *
 * After this call, MessageRegistry::instance().decode() will dispatch
 * correctly for message type IDs registered in this phase.  Message types
 * not yet implemented return ErrorCode::MessageTypeUnsupported as before.
 *
 * Phase 2 registers decoders for the following type IDs:
 *   1, 2, 3  — Class A position report (Msg1_3PositionReportClassA)
 *   4, 11    — Base station report / UTC date response (Msg4_11BaseStationReport)
 *   5        — Static and voyage data (Msg5StaticAndVoyageData)
 *   18       — Standard Class B CS position report (Msg18ClassBPositionReport)
 *   24       — Class B CS static data report (Msg24StaticDataReport)
 */
void register_all_decoders();

} // namespace aislib
