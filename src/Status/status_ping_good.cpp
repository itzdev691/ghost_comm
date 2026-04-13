#include "Network/mesh.h"
#include "Status/status_ping.h"

void sendGoodStatusPing() {
  sendMessage(BROADCAST_MAC, MessageType::Status, "Good");
}
