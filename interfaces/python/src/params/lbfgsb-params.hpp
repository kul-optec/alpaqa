#pragma once

#include <alpaqa/lbfgsb/lbfgsb-adapter.hpp>
#include <dict/kwargs-to-struct.hpp>

template <>
PARAMS_TABLE_DECL(alpaqa::lbfgsb::LBFGSBParams);

extern PARAMS_TABLE_INST(alpaqa::lbfgsb::LBFGSBParams);
