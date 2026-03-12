#pragma once

/**
 * @file aislib.h
 * @brief Umbrella header for the aislib AIS parsing and generation library.
 *
 * Including this header provides access to all public library interfaces:
 * the core infrastructure (error handling, bit engine, payload codec, sentence
 * framer, message base), and all message type decoders implemented to date.
 *
 * For applications that need fine-grained control over compile dependencies,
 * include the individual component headers directly.
 *
 * Library initialisation must be performed before decoding any messages:
 *
 * @code
 *   #include <aislib/aislib.h>
 *
 *   aislib::register_all_decoders();   // call once at startup
 * @endcode
 *
 * Refer to the README for full usage examples and architecture documentation.
 */

// Core infrastructure
#include "aislib/error.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"
#include "aislib/sentence.h"
#include "aislib/message.h"
#include "aislib/registry_init.h"

// Message type decoders — Phase 2
#include "aislib/messages/msg1_3.h"
#include "aislib/messages/msg4_11.h"
#include "aislib/messages/msg05.h"
#include "aislib/messages/msg18.h"
#include "aislib/messages/msg24.h"

// Message type decoders — Phase 3
#include "aislib/messages/msg09.h"
#include "aislib/messages/msg10.h"
#include "aislib/messages/msg19.h"
#include "aislib/messages/msg27.h"