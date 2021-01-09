#ifndef NMCOMMAND_DATA_H
#define NMCOMMAND_DATA_H

#include "json.hpp"
#include "nmcommand.h"
#include "nmjsonconst.h"
#include "magic_enum.hpp"

using json = nlohmann::json;

class nmcommand_data
{
protected:
    nmcommand command;
    json json_data;
public:
    nmcommand_data(std::string str_data);
    bool isValid();
    json getJsonData();
    nmcommand getCommand();
};

#endif // NMCOMMAND_DATA_H
