#include "aislib/registry_init.h"
#include "aislib/message.h"
#include "aislib/messages/msg1_3.h"
#include "aislib/messages/msg4_11.h"
#include "aislib/messages/msg05.h"
#include "aislib/messages/msg09.h"
#include "aislib/messages/msg10.h"
#include "aislib/messages/msg18.h"
#include "aislib/messages/msg19.h"
#include "aislib/messages/msg24.h"
#include "aislib/messages/msg27.h"

namespace aislib {

/**
 * @brief Registers all implemented decoder functions with the MessageRegistry.
 *
 * Each call to register_decoder() installs a function pointer.  The call is
 * idempotent because registering the same pointer a second time is a silent
 * overwrite with an identical value.
 *
 * Consult the CHANGELOG for the current list of registered type IDs.
 */
void register_all_decoders()
{
    MessageRegistry& reg = MessageRegistry::instance();

    // Types 1, 2, 3 — Class A position report (identical payload layout)
    reg.register_decoder(1u,  decode_msg1_3);
    reg.register_decoder(2u,  decode_msg1_3);
    reg.register_decoder(3u,  decode_msg1_3);

    // Types 4, 11 — Base station report / UTC date response
    reg.register_decoder(4u,  decode_msg4_11);
    reg.register_decoder(11u, decode_msg4_11);

    // Type 5 — Static and voyage related data (two-part)
    reg.register_decoder(5u,  decode_msg5);

    // Type 9 — Standard SAR aircraft position report
    reg.register_decoder(9u,  decode_msg9);

    // Type 10 — UTC/date inquiry
    reg.register_decoder(10u, decode_msg10);

    // Type 18 — Standard Class B CS position report
    reg.register_decoder(18u, decode_msg18);

    // Type 19 — Extended Class B CS position report
    reg.register_decoder(19u, decode_msg19);

    // Type 24 — Class B CS static data report (Part A and Part B)
    reg.register_decoder(24u, decode_msg24);

    // Type 27 — Long-range AIS broadcast message
    reg.register_decoder(27u, decode_msg27);
}

} // namespace aislib