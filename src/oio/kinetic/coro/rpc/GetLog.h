/**
 * This file is part of the OpenIO client libraries
 * Copyright (C) 2016 OpenIO SAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */

#ifndef SRC_OIO_KINETIC_CORO_RPC_GETLOG_H_
#define SRC_OIO_KINETIC_CORO_RPC_GETLOG_H_

#include <memory>
#include "Exchange.h"

namespace oio {
namespace kinetic {
namespace rpc {

class GetLog : public oio::kinetic::rpc::Exchange {
 public:
    GetLog();

    FORBID_MOVE_CTOR(GetLog);
    FORBID_COPY_CTOR(GetLog);

    virtual ~GetLog();

    void ManageReply(oio::kinetic::rpc::Request *rep) override;

    double getCpu() const;

    double getTemp() const;

    double getSpace() const;

    double getIo() const;

 private:
    double cpu, temp, space, io;
};

}  // namespace rpc
}  // namespace kinetic
}  // namespace oio

#endif  // SRC_OIO_KINETIC_CORO_RPC_GETLOG_H_
