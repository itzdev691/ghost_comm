#include "Network/mesh.h"
#include "Status/status_ping.h"

void sendOkStatusPing() {
  sendMessage(BROADCAST_MAC, MessageType::Status, "OK");
}