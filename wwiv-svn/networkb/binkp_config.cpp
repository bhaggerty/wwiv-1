#include "networkb/binkp_config.h"

#include <iostream>
#include <memory>
#include <map>
#include <sstream>
#include <string>

#include "core/strings.h"
#include "core/inifile.h"
#include "core/file.h"
#include "core/textfile.h"
#include "sdk/networks.h"

using std::endl;
using std::map;
using std::string;
using std::stringstream;
using std::unique_ptr;
using std::vector;
using wwiv::core::IniFile;
using namespace wwiv::strings;
using namespace wwiv::sdk;

namespace wwiv {
namespace net {

// [[ VisibleForTesting ]]
bool ParseBinkConfigLine(const string& line, uint16_t* node, BinkNodeConfig* config) {
  // A line will be of the format @node host:port
  if (line.empty() || line[0] != '@') {
    // skip empty lines and those not starting with @.
    return false;
  }
  
  stringstream stream(line);
  string node_str;
  stream >> node_str;
  *node = StringToUnsignedShort(node_str.substr(1));
  string host_port_str;
  stream >> host_port_str;
  
  string host = host_port_str;
  uint16_t port = 24554;  // default port
  if (host_port_str.find(':') != string::npos) {
    vector<string> host_port = SplitString(host_port_str, ":");
    host = host_port[0];
    port = StringToUnsignedShort(host_port[1]);
  }
  
  config->host = host;
  config->port = port;

  return true;
}

static bool ParseAddressesFile(std::map<uint16_t, BinkNodeConfig>* node_config_map, const string network_dir) {
  TextFile node_config_file(network_dir, "binkp.net", "rt");
  if (node_config_file.IsOpen()) {
    // Only load the configuration file if it exists.
    string line;
    while (node_config_file.ReadLine(&line)) {
      uint16_t node_number;
      BinkNodeConfig node_config;
      if (ParseBinkConfigLine(line, &node_number, &node_config)) {
        // Parsed a line correctly.
        node_config_map->emplace(node_number, node_config);
      }
    }
  }
  return true;
}

BinkConfig::BinkConfig(const std::string& callout_network_name, const Config& config, const Networks& networks)
    : callout_network_name_(callout_network_name), networks_(networks) {
  // network names will alwyas be compared lower case.
  StringLowerCase(&callout_network_name_);
  system_name_ = config.config()->systemname;
  if (system_name_.empty()) {
    system_name_ = "Unnamed WWIV BBS";
  }

  if (networks.contains(callout_network_name)) {
    const net_networks_rec& net = networks[callout_network_name];
    callout_node_ = net.sysnum;
    if (callout_node_ == 0) {
      throw config_error(StringPrintf("NODE not specified for network: '%s'", callout_network_name.c_str()));
    }

    ParseAddressesFile(&node_config_, net.dir);
  }
}

const std::string BinkConfig::network_dir(const std::string& network_name) const {
  return networks_[network_name].dir;
}

static net_networks_rec test_net(const string& network_dir) {
  net_networks_rec net;
  net.sysnum = 1;
  strcpy(net.name, "wwivnet");
  net.type = net_type_wwivnet;
  strcpy(net.dir, network_dir.c_str());
  return net;
}

// For testing
BinkConfig::BinkConfig(int callout_node_number, const string& system_name, const string& network_dir) 
  : callout_network_name_("wwivnet"), callout_node_(callout_node_number), system_name_(system_name),
    networks_({ test_net(network_dir) }) {
  ParseAddressesFile(&node_config_, network_dir);
}

BinkConfig::~BinkConfig() {}

const BinkNodeConfig* BinkConfig::node_config_for(int node) const {
  auto iter = node_config_.find(node);
  if (iter != end(node_config_)) {
    return &iter->second;
  }
  return nullptr;
}

}  // namespace net
}  // namespace wwiv

