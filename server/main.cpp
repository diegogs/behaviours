/**
 * Behaviours - UML-like graphic programming language
 * Copyright (C) 2013 Coralbits SL & AISoy Robotics.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <onion/onion.hpp>
#include <onion/url.hpp>
#include <onion/extrahandlers.hpp>

extern "C"{
  #include <onion/handlers/webdav.h>
}
#include <ab/manager.h>
#include <ab/lua.h>
#include <ab/log.h>
#include <ab/factory.h>
#include <ab/events/start.h>
#include <ab/pluginloader.h>
#include <signal.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "nodemanager.hpp"

namespace AB{
	std::string static_dir="static";
	std::string data_dir="data";
}

using namespace ABServer;
using namespace std;
using namespace AB;

Onion::Onion *o;
 
AB::Manager manager;
   
NodeManager nodeManager(&manager);
  
static void node_notify_enter(AB::Node *n) {
  nodeManager.activateNode(n);
}

static void node_notify_exit(AB::Node *n) {
  nodeManager.deactivateNode(n);
}

static void lua_output_update(const std::string &str) {
  if(!str.empty()) {
    WARNING("got: %s from Lua",str.c_str());
    nodeManager.updateLuaOutput(str);
  }
}

static void on_SIGINT(int){
	static bool twice=false;
	if (twice){
		INFO("Exiting on second SIGINT.");
		exit(1);
	}
	twice=true;
	o->listenStop();
}

int main(void){
	if (getenv("DIAPATH")){
		WARNING("Setting default path to %s", getenv("DIAPATH"));
		static_dir=std::string(getenv("DIAPATH")) + "/static/";
		data_dir=std::string(getenv("DIAPATH")) + "/data/";
	}
	else{
		struct stat sb;
		if (stat(static_dir.c_str(), &sb)<0){
			ERROR("Can't open data dir. Set it with environment variable DIAPATH to the parent of static and data dirs. Alternatively run dia at the source directory of dia.");
			exit(1);
		}
	}
	
  ABServer::init();
  //DIA::init(&manager);

  AB::lua_ab_print_real = lua_output_update;
  AB::manager_notify_node_enter = node_notify_enter;
  AB::manager_notify_node_exit = node_notify_exit;
  
  manager.loadBehaviour("data/current.dia");
  o=new Onion::Onion(O_POOL);
  
  signal(SIGINT, on_SIGINT);
  signal(SIGTERM, on_SIGINT);
  
  o->setHostname("0.0.0.0");
  o->setPort("8081");
  
  Onion::Url url(o);
	
  url.add("", new Onion::RedirectHandler("/static/index.html"));
  url.add("^manager/", &nodeManager, &NodeManager::manager);
  url.add("^node/", &nodeManager, &NodeManager::node);
  url.add("^connections/", &nodeManager, &NodeManager::connections);
  url.add("^lua/", &nodeManager, &NodeManager::lua);
  url.add("^update/", &nodeManager, &NodeManager::update);
  url.add("^upload/", &nodeManager, &NodeManager::uploadXML);
  url.add("^wavload/", &nodeManager, &NodeManager::uploadWAV);
  url.add("^static", new Onion::StaticHandler(static_dir));
  url.add("^data", new Onion::StaticHandler(data_dir));
  
	
	PluginLoader::loadPath(".");
	
//  onion_handler *w=onion_handler_webdav("data/files",NULL);
//  onion_url_add_handler(url.c_ptr(), "^webdav/", w);
	    
  nodeManager.mimeFill();

	INFO("Listening at 127.0.0.1:8081");
	
  o->listen();
  
  delete o;
  std::cout<<"closed"<<endl;
}
