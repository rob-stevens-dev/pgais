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
 * MessageRegistry::instance().decode() are made.  Once registration is
 * complete, decode() is safe for concurrent read access.
 */

/**
 * @brief Registers all currently implemented AIS message type decoders with
 *        the MessageRegistry singleton.
 *
 * After this call, MessageRegistry::instance().decode() will dispatch
 * correctly for all message type IDs that have a concrete implementation.
 * Type IDs without a registered decoder continue to return
 * ErrorCode::MessageTypeUnsupported.
 *
 * The set of registered types grows with each development phase.  Consult
 * the CHANGELOG for the current list of supported type IDs.
 */
void register_all_decoders();

} // namespace aislib