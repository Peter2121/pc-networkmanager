#pragma once

#include <string>
#include "json.hpp"

const std::string JSON_PARAM_SCOPE = "SCOPE";
const std::string JSON_PARAM_CMD = "CMD";
const std::string JSON_PARAM_DATA = "DATA";
const std::string JSON_PARAM_RESULT = "RESULT";
const std::string JSON_PARAM_SUCC = "SUCCESS";
const std::string JSON_PARAM_ERR = "ERROR";
const std::string JSON_DATA_ERR_INVALID_REQUEST = "INVALID REQUEST";
const std::string JSON_DATA_ERR_NOT_IMPLEMENTED = "NOT IMPLEMENTED";
const std::string JSON_DATA_ERR_INVALID_COMMAND = "INVALID COMMAND";
const std::string JSON_DATA_ERR_INTERNAL_ERROR = "INTERNAL ERROR";
const std::string JSON_PARAM_ADDR_TYPE = "ADDRESS TYPE";
const std::string JSON_PARAM_IPV4_ADDR = "IPV4 ADDRESS";
const std::string JSON_PARAM_IPV6_ADDR = "IPV6 ADDRESS";
const std::string JSON_PARAM_LINK_ADDR = "LINK ADDRESS";
const std::string JSON_PARAM_IPV4_MASK = "IPV4 SUBNET MASK";
const std::string JSON_PARAM_IPV6_MASK = "IPV6 SUBNET MASK";
const std::string JSON_PARAM_IPV4_BCAST = "IPV4 BROADCAST ADDRESS";
const std::string JSON_PARAM_IPV6_BCAST = "IPV6 BROADCAST ADDRESS";
const std::string JSON_PARAM_IPV4_PPP_GW = "IPV4 PPP GATEWAY";
const std::string JSON_PARAM_IPV6_PPP_GW = "IPV6 PPP GATEWAY";
const std::string JSON_PARAM_IF_NAME = "INTERFACE NAME";
const std::string JSON_PARAM_ADDRESSES = "ADDRESSES";
const std::string JSON_PARAM_INTERFACES = "INTERFACES";

const nlohmann::json JSON_RESULT_SUCCESS = { { JSON_PARAM_RESULT, JSON_PARAM_SUCC } };
const nlohmann::json JSON_RESULT_ERR = { { JSON_PARAM_RESULT, JSON_PARAM_ERR } };


