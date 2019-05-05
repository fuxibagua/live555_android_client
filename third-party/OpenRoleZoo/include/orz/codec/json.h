//
// Created by lby on 2018/1/10.
//

#ifndef ORZ_CODEC_JSON_H
#define ORZ_CODEC_JSON_H

#include "orz/io/jug/jug.h"

namespace orz
{
    orz::jug json2jug(const std::string &json);
    std::string jug2json(const orz::jug &obj);
    std::string form_encode(const orz::jug &obj);
}


#endif //ORZ_CODEC_JSON_H
