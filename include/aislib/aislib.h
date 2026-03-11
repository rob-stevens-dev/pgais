#pragma once

/**
 * @file aislib.h
 * @brief Umbrella header for the aislib AIS message parsing library.
 *
 * Including this header brings in all public aislib interfaces: error
 * handling, bit-field primitives, payload codec, sentence framing,
 * message base class and type registry, all implemented message types,
 * and the registry initialisation function.
 *
 * For applications that include many aislib headers, this umbrella header
 * is the recommended inclusion point.  For minimal compilation units that
 * use only a subset of the library, including individual headers directly
 * reduces build times.
 *
 * Usage pattern:
 * @code
 *   #include <aislib/aislib.h>
 *
 *   // At application start-up:
 *   aislib::register_all_decoders();
 *
 *   // Decode a sentence:
 *   auto sr = aislib::Sentence::parse("!AIVDM,...");
 *   aislib::SentenceAssembler assembler;
 *   auto ar = assembler.feed(sr.value());
 *   if (ar && ar.value()) {
 *       const auto& msg = ar.value().value();
 *       auto dr = aislib::MessageRegistry::instance().decode(msg.payload, msg.fill_bits);
 *       if (dr) {
 *           // dr.value() is std::unique_ptr<aislib::Message>
 *       }
 *   }
 * @endcode
 */

#include "aislib/error.h"
#include "aislib/bitfield.h"
#include "aislib/payload.h"
#include "aislib/sentence.h"
#include "aislib/message.h"
#include "aislib/registry_init.h"

#include "aislib/messages/msg1_3.h"
#include "aislib/messages/msg4_11.h"
#include "aislib/messages/msg5.h"
#include "aislib/messages/msg18.h"
#include "aislib/messages/msg24.h"
