#include "aislib/registry_init.h"
#include "aislib/message.h"
#include "aislib/messages/msg1_3.h"
#include "aislib/messages/msg4_11.h"
#include "aislib/messages/msg5.h"
#include "aislib/messages/msg18.h"
#include "aislib/messages/msg24.h"

namespace aislib {

/**
 * @brief Registers all Phase 2 decoder functions with the MessageRegistry.
 *
 * Each call to register_decoder() installs a function pointer.  The call is
 * idempotent because registering the same pointer a second time is a
 * silent overwrite with an identical value.
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

    // Type 5 — Static and voyage related data
    reg.register_decoder(5u,  decode_msg5);

    // Type 18 — Standard Class B CS position report
    reg.register_decoder(18u, decode_msg18);

    // Type 24 — Class B CS static data report
    reg.register_decoder(24u, decode_msg24);
}

} // namespace aislib
