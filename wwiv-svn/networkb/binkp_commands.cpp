#include "networkb/binkp_commands.h"

#include <map>
#include <string>

using std::map;
using std::string;

namespace wwiv {
namespace net {

const int BinkpCommands::M_NUL;
const int BinkpCommands::M_ADR;
const int BinkpCommands::M_PWD;
const int BinkpCommands::M_FILE;
const int BinkpCommands::M_OK;
const int BinkpCommands::M_EOB;
const int BinkpCommands::M_GOT;
const int BinkpCommands::M_ERR;
const int BinkpCommands::M_BSY;
const int BinkpCommands::M_GET;
const int BinkpCommands::M_SKIP;

// static
string BinkpCommands::command_id_to_name(int command_id) {
  static const map<int, string> map = {
    {M_NUL, "M_NUL"},
    {M_ADR, "M_ADR"},
    {M_PWD, "M_PWD"},
    {M_FILE, "M_FILE"},
    {M_OK, "M_OK"},
    {M_EOB, "M_EOB"},
    {M_GOT, "M_GOT"},
    {M_ERR, "M_ERR"},
    {M_BSY, "M_BSY"},
    {M_GET, "M_GET"},
    {M_SKIP, "M_SKIP"},
  };
  return map.at(command_id);
}


}  // namespace net
} // namespace wwiv
