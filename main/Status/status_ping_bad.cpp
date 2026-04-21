#include "Network/mesh.h"
#include "Status/status_ping.h"

void sendBadStatusPing() {
  sendMessage(BROADCAST_MAC, MessageType::Status, "Bad");
}
