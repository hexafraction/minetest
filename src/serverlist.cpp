/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <iostream>
#include <sstream>
#include <algorithm>

#include "main.h" // for g_settings
#include "settings.h"
#include "serverlist.h"
#include "filesys.h"
#include "porting.h"
#include "log.h"
#include "json/json.h"
#if USE_CURL
#include <curl/curl.h>
#endif

namespace ServerList
{
std::string getFilePath()
{
	std::string serverlist_file = g_settings->get("serverlist_file");

	std::string rel_path = std::string("client") + DIR_DELIM
		+ "serverlist" + DIR_DELIM
		+ serverlist_file;
	std::string path = porting::path_share + DIR_DELIM + rel_path;
	return path;
}

std::vector<ServerListSpec> getLocal()
{
	std::string path = ServerList::getFilePath();
	std::string liststring;
	if(fs::PathExists(path))
	{
		std::ifstream istream(path.c_str(), std::ios::binary);
		if(istream.is_open())
		{
			std::ostringstream ostream;
			ostream << istream.rdbuf();
			liststring = ostream.str();
			istream.close();
		}
	}

	return ServerList::deSerialize(liststring);
}


#if USE_CURL

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


std::vector<ServerListSpec> getOnline()
{
	std::string liststring;
	CURL *curl;

	curl = curl_easy_init();
	if (curl)
	{
		CURLcode res;

		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_URL, (g_settings->get("serverlist_url")+"/list").c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ServerList::WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &liststring);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			errorstream<<"Serverlist at url "<<g_settings->get("serverlist_url")<<" not found (internet connection?)"<<std::endl;
		curl_easy_cleanup(curl);
	}
	return ServerList::deSerializeJson(liststring);
}

#endif

/*
	Delete a server fromt he local favorites list
*/
bool deleteEntry (ServerListSpec server)
{
	std::vector<ServerListSpec> serverlist = ServerList::getLocal();
	for(unsigned i = 0; i < serverlist.size(); i++)
	{
		if  (serverlist[i]["address"] == server["address"]
		&&   serverlist[i]["port"]    == server["port"])
		{
			serverlist.erase(serverlist.begin() + i);
		}
	}

	std::string path = ServerList::getFilePath();
	std::ofstream stream (path.c_str());
	if (stream.is_open())
	{
		stream<<ServerList::serialize(serverlist);
		return true;
	}
	return false;
}

/*
	Insert a server to the local favorites list
*/
bool insert (ServerListSpec server)
{
	// Remove duplicates
	ServerList::deleteEntry(server);

	std::vector<ServerListSpec> serverlist = ServerList::getLocal();

	// Insert new server at the top of the list
	serverlist.insert(serverlist.begin(), server);

	std::string path = ServerList::getFilePath();
	std::ofstream stream (path.c_str());
	if (stream.is_open())
	{
		stream<<ServerList::serialize(serverlist);
	}

	return false;
}

std::vector<ServerListSpec> deSerialize(std::string liststring)
{
	std::vector<ServerListSpec> serverlist;
	std::istringstream stream(liststring);
	std::string line, tmp;
	while (std::getline(stream, line))
	{
		std::transform(line.begin(), line.end(),line.begin(), ::toupper);
		if (line == "[SERVER]")
		{
			ServerListSpec thisserver;
			std::getline(stream, tmp);
			thisserver["name"] = tmp;
			std::getline(stream, tmp);
			thisserver["address"] = tmp;
			std::getline(stream, tmp);
			thisserver["port"] = tmp;
			std::getline(stream, tmp);
			thisserver["description"] = tmp;
			serverlist.push_back(thisserver);
		}
	}
	return serverlist;
}

std::string serialize(std::vector<ServerListSpec> serverlist)
{
	std::string liststring;
	for(std::vector<ServerListSpec>::iterator i = serverlist.begin(); i != serverlist.end(); i++)
	{
		liststring += "[server]\n";
		liststring += (*i)["name"].asString() + "\n";
		liststring += (*i)["address"].asString() + "\n";
		liststring += (*i)["port"].asString() + "\n";
		liststring += (*i)["description"].asString() + "\n";
		liststring += "\n";
	}
	return liststring;
}

std::vector<ServerListSpec> deSerializeJson(std::string liststring)
{
	std::vector<ServerListSpec> serverlist;
	Json::Value root;
	Json::Reader reader;
	std::istringstream stream(liststring);
	if (!liststring.size()) {
		return serverlist;
	}
	if (!reader.parse( stream, root ) )
	{
		errorstream  << "Failed to parse server list " << reader.getFormattedErrorMessages();
		return serverlist;
	}
	if (root["list"].isArray())
	    for (unsigned int i = 0; i < root["list"].size(); i++)
	{
		if (root["list"][i].isObject()) {
			serverlist.push_back(root["list"][i]);
		}
	}
	return serverlist;
}

std::string serializeJson(std::vector<ServerListSpec> serverlist)
{
	Json::Value root;
	Json::Value list(Json::arrayValue);
	for(std::vector<ServerListSpec>::iterator i = serverlist.begin(); i != serverlist.end(); i++)
	{
		list.append(*i);
	}
	root["list"] = list;
	Json::StyledWriter writer;
	return writer.write( root );
}


#if USE_CURL
static size_t ServerAnnounceCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    return 0;
    //((std::string*)userp)->append((char*)contents, size * nmemb);
    //return size * nmemb;
}
void sendAnnounce(std::string action, u16 clients) {
	Json::Value server;
	if (action.size())
		server["action"]	= action;
	server["port"] = g_settings->get("port");
        if (action != "del") {
		server["name"]		= g_settings->get("server_name");
		server["description"]	= g_settings->get("server_description");
		server["address"]	= g_settings->get("server_address");
		server["version"]	= VERSION_STRING;
		server["url"]		= g_settings->get("server_url");
		server["creative"]	= g_settings->get("creative_mode");
		server["damage"]	= g_settings->get("enable_damage");
		server["dedicated"]	= g_settings->get("server_dedicated");
		server["password"]	= g_settings->getBool("disallow_empty_password");
		server["pvp"]		= g_settings->getBool("enable_pvp");
		server["clients"]	= clients;
		server["clients_max"]	= g_settings->get("max_users");
	}
	if(server["action"] == "start")
		actionstream << "announcing to " << g_settings->get("serverlist_url") << std::endl;
	Json::StyledWriter writer;
	CURL *curl;
	curl = curl_easy_init();
	if (curl)
	{
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_URL, (g_settings->get("serverlist_url")+std::string("/announce?json=")+curl_easy_escape(curl, writer.write( server ).c_str(), 0)).c_str());
		//curl_easy_setopt(curl, CURLOPT_USERAGENT, "minetest");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ServerList::ServerAnnounceCallback);
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &liststring);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
		res = curl_easy_perform(curl);
		//if (res != CURLE_OK)
		//	errorstream<<"Serverlist at url "<<g_settings->get("serverlist_url")<<" not found (internet connection?)"<<std::endl;
		curl_easy_cleanup(curl);
	}

}
#endif

} //namespace ServerList
