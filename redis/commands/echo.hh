/*
 * Copyright (C) 2019 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "redis/request.hh"
#include "redis/abstract_command.hh"

namespace redis {

namespace commands {

class echo : public abstract_command {
    bytes _str;
public:
    static shared_ptr<abstract_command> prepare(service::storage_proxy& proxy, request&& req);
    echo(bytes&& name, bytes&& str) : abstract_command(std::move(name)) , _str(std::move(str)) {}
    virtual future<redis_message> execute(service::storage_proxy&, redis_options&, service_permit) override;
};

}

}
