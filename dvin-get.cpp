/*
 *   Copyright (c) 2022 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <opendht.h>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <condition_variable>

#include "DVINDataTypes.h"

static bool running = true;
static std::condition_variable cv;
static std::mutex mtx;

int main(int argc, char *argv[])
{
	if (3 != argc)
	{
		std::cout << "usage: " << argv[0] << " bootstrap key" << std::endl;
		return EXIT_FAILURE;
	}

	const std::string key(argv[2]);

	std::string name("TestGet");
	name += std::to_string(getpid());
	dht::DhtRunner node;
	node.run(17171, dht::crypto::generateIdentity(name));
	node.bootstrap(argv[1], "17171");
	std::cout << "Joined the DHT at " << argv[1] << " using an id of " << name << std::endl;
	std::cout << "Getting Data for " << key << " ..." << std::endl;
	auto starttime = std::chrono::steady_clock::now();
	node.get(
		dht::InfoHash::get(key),
		[](const std::shared_ptr<dht::Value> &v) {
			if (v->checkSignature())
			{
				std::cout << "Value is signed" << std::endl;
				std::cout << "User Type: " << v->user_type << std::endl;
				if (0 == v->user_type.compare("reflector-mrefd-0"))
				{
					auto rdat = dht::Value::unpack<SReflectorData0>(*v);
					std::cout << "Callsign: '" << rdat.cs << "'" << std::endl;
					std::cout << "Modules: '" << rdat.mods << "'" << std::endl;
					std::cout << "IPv4 Address: '" << rdat.ipv4 << "'" << std::endl;
					std::cout << "IPv6 Address: '" << rdat.ipv6 << "'" << std::endl;
					std::cout << "Port: " << rdat.port << std::endl;
					std::cout << "URL: '" << rdat.url << "'" << std::endl;
					std::cout << "Email Address: '" << rdat.email << "'" << std::endl;
					if (rdat.peers.size())
					{
						std::cout << "Peer\tModule(s):" << std::endl;
						for (const auto &p : rdat.peers)
						{
							std::cout << p.first << '\t' << p.second << std::endl;
						}
					}
					else
					{
						std::cout << "Peers: none" << std::endl;
					}
				}
				else if (0 == v->user_type.compare("reflector-mrefd-1"))
				{
					auto rdat = dht::Value::unpack<SReflectorData1>(*v);
					std::cout << "Callsign: '" << rdat.cs << "'" << std::endl;
					std::cout << "Modules: '" << rdat.mods << "'" << std::endl;
					std::cout << "Encryption Enabled Modules: " << rdat.emods << std::endl;
					std::cout << "IPv4 Address: '" << rdat.ipv4 << "'" << std::endl;
					std::cout << "IPv6 Address: '" << rdat.ipv6 << "'" << std::endl;
					std::cout << "Port: " << rdat.port << std::endl;
					std::cout << "URL: '" << rdat.url << "'" << std::endl;
					std::cout << "Email Address: '" << rdat.email << "'" << std::endl;
					std::cout << "Sponsor: '" << rdat.sponsor << "'" << std::endl;
					std::cout << "Country: '" << rdat.country << "'" << std::endl;
					if (rdat.peers.size())
					{
						std::cout << "Peer\tModule(s):" << std::endl;
						for (const auto &p : rdat.peers)
						{
							std::cout << p.first << '\t' << p.second << std::endl;
						}
					}
					else
					{
						std::cout << "Peers: none" << std::endl;
					}
				}
				else
					std::cout << "Unknown User Type: " << v->user_type << std::endl;
			}
			else
				std::cout << "Value signature failed!" << std::endl;
			return false;
		},
		[](bool success) {
			if (! success)
				std::cerr << "get failed!" << std::endl;
			std::unique_lock<std::mutex> lck(mtx);
			running = false;
			cv.notify_all();
		}
	);

	std::unique_lock<std::mutex> lck(mtx);
	while (running)
		cv.wait(lck);

	std::chrono::duration<double> elapsed(std::chrono::steady_clock::now() - starttime);
	std::cout << "time: " << std::setprecision(3) << elapsed.count() << " seconds" << std::endl;

	node.join();

	return EXIT_SUCCESS;
}
