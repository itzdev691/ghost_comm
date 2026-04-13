#include "Network/mesh.h"
#include "Status/status_ping.h"


void sendConfirmationStatusPing() {
  sendMessage(BROADCAST_MAC, MessageType::Status, "Confirmed");
}