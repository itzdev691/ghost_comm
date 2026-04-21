// src/node_identity.cpp
#include "Identity/node_identity.h"

// Edit this file to set a unique name for this node. This is used in the display and can be helpful when debugging larger networks to identify which node is which. It has no effect on the network protocol or connectivity, so it doesn't need to be unique across nodes, but it can be helpful to avoid confusion.

static constexpr char NODE_NAME[] = "spectre";

const char* getNodeName() {
  return NODE_NAME;
}
